from __future__ import annotations

import json
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any
from urllib.parse import parse_qs, urlparse

WEB_UI = """<!doctype html><html><body><h1>Peristaltic Pump</h1></body></html>"""


class PumpModel:
    def __init__(self) -> None:
        self.mode = 0  # 0 flow, 1 dosing
        self.target_speed = 0.0
        self.current_speed = 0.0
        self.last_manual_speed = 120.0
        self.max_speed = 450.0
        self.ml_per_rev_cw = 2.6
        self.ml_per_rev_ccw = 2.6
        self.dosing_speed = 180.0
        self.running = False
        self.dosing_remaining_ml = 0.0
        self.total_pumped_l = 0.0
        self.total_hose_l = 0.0
        self.uptime_sec = 0
        self.wifi_connected = True
        self.ssid = "TestWiFi"
        self.ip = "127.0.0.1"
        self.last_zigbee_payload = ""
        self.ntp_server = "time.google.com"
        self.tz_offset_minutes = 0
        self.schedule_entries: list[dict[str, Any]] = []
        self.selected_motor_id = 0
        self.expansion_enabled = True
        self.expansion_motor_count = 4
        self.expansion_interface = "i2c"
        self.expansion_connected = True
        self.growth_program_enabled = False
        self.ph_regulation_enabled = False
        self.firmware_repo = "dslimp/peristaltic-pump"
        self.firmware_asset_name = "firmware.bin"
        self.firmware_fs_asset_name = "littlefs.bin"
        self.firmware_releases: list[dict[str, Any]] = [
            {
                "tag": "v0.2.1",
                "publishedAt": "2026-02-20T00:00:00Z",
                "assets": [
                    {"name": "firmware.bin", "size": 1024 * 512, "downloadUrl": "https://example.com/v0.2.1/firmware.bin"},
                    {"name": "littlefs.bin", "size": 1024 * 128, "downloadUrl": "https://example.com/v0.2.1/littlefs.bin"},
                ],
            },
            {
                "tag": "v0.2.0",
                "publishedAt": "2026-02-12T00:00:00Z",
                "assets": [{"name": "firmware.bin", "size": 1024 * 500, "downloadUrl": "https://example.com/v0.2.0/firmware.bin"}],
            },
        ]

        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._thread: threading.Thread | None = None
        self._last_tick = time.monotonic()
        self._uptime_remainder_sec = 0.0

    def start(self) -> None:
        self._thread = threading.Thread(target=self._loop, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        if self._thread is not None:
            self._thread.join(timeout=1.0)

    def _loop(self) -> None:
        while not self._stop.is_set():
            time.sleep(0.01)
            now = time.monotonic()
            dt = now - self._last_tick
            self._last_tick = now
            self._tick(dt)

    def _tick(self, dt_sec: float) -> None:
        min_speed = 0.01
        accel = 50.0
        halt = 200.0

        with self._lock:
            ramp = (halt if abs(self.target_speed) < min_speed else accel) * dt_sec
            if self.current_speed < self.target_speed:
                self.current_speed = min(self.current_speed + ramp, self.target_speed)
            elif self.current_speed > self.target_speed:
                self.current_speed = max(self.current_speed - ramp, self.target_speed)

            if abs(self.current_speed) < min_speed and abs(self.target_speed) < min_speed:
                self.current_speed = 0.0
                self.running = False

            ml_per_rev = self.ml_per_rev_cw if self.current_speed >= 0 else self.ml_per_rev_ccw
            delta_ml = abs(self.current_speed) / 60.0 * ml_per_rev * dt_sec
            self.total_pumped_l += delta_ml / 1000.0
            self.total_hose_l += delta_ml / 1000.0
            if abs(self.current_speed) >= min_speed:
                self._uptime_remainder_sec += dt_sec
                added = int(self._uptime_remainder_sec)
                if added > 0:
                    self.uptime_sec += added
                    self._uptime_remainder_sec -= float(added)

            if self.mode == 1 and self.dosing_remaining_ml > 0.0:
                self.dosing_remaining_ml -= delta_ml
                if self.dosing_remaining_ml <= 0.0:
                    self.dosing_remaining_ml = 0.0
                    self.target_speed = 0.0
                    self.mode = 0
                    self.running = False

    def active_motor_count(self) -> int:
        if not self.expansion_enabled:
            return 1
        return 1 + max(0, min(4, int(self.expansion_motor_count)))

    def _read_motor_id(self, value: Any, default: int = 0) -> int | None:
        if value is None:
            return default
        if isinstance(value, bool):
            return None
        if isinstance(value, (int, float)):
            motor_id = int(value)
        elif isinstance(value, str) and value.strip():
            try:
                motor_id = int(value.strip())
            except ValueError:
                return None
        else:
            return None
        if motor_id < 0 or motor_id >= self.active_motor_count():
            return None
        return motor_id

    def to_state(self, motor_id: int = 0) -> dict[str, Any]:
        mode_name = "flow_lph" if self.mode == 0 else "dosing"

        with self._lock:
            flow_ml_min = self.current_speed * (self.ml_per_rev_cw if self.current_speed >= 0 else self.ml_per_rev_ccw)
            target_flow_ml_min = self.target_speed * (self.ml_per_rev_cw if self.target_speed >= 0 else self.ml_per_rev_ccw)
            base = {
                "firmware": "3.1.0-esp32-sim",
                "motorId": motor_id,
                "selectedMotorId": self.selected_motor_id,
                "activeMotorCount": self.active_motor_count(),
                "expansion": {
                    "enabled": self.expansion_enabled,
                    "interface": self.expansion_interface,
                    "motorCount": self.expansion_motor_count,
                    "connected": self.expansion_connected,
                    "address": 0x2A if self.expansion_connected else 0,
                },
                "mode": self.mode,
                "modeName": mode_name,
                "running": self.running,
                "flowMlMin": flow_ml_min,
                "flowLph": flow_ml_min * 0.06,
                "targetFlowMlMin": target_flow_ml_min,
                "targetFlowLph": target_flow_ml_min * 0.06,
                "direction": "forward" if self.target_speed >= 0 else "reverse",
                "mlPerRevCw": self.ml_per_rev_cw,
                "mlPerRevCcw": self.ml_per_rev_ccw,
                "dosingFlowLph": abs(self.dosing_speed * self.ml_per_rev_cw * 0.06),
                "dosingRemainingMl": self.dosing_remaining_ml,
                "uptimeSec": self.uptime_sec,
                "totalPumpedL": self.total_pumped_l,
                "totalHoseL": self.total_hose_l,
                "wifiConnected": self.wifi_connected,
                "ssid": self.ssid,
                "ip": self.ip,
                "preferredReverse": self.target_speed < 0,
            }
            base["motors"] = []
            for idx in range(self.active_motor_count()):
                m = dict(base)
                m["motorId"] = idx
                base["motors"].append(
                    {
                        "motorId": idx,
                        "mode": m["mode"],
                        "modeName": m["modeName"],
                        "running": m["running"],
                        "flowMlMin": m["flowMlMin"],
                        "flowLph": m["flowLph"],
                        "targetFlowMlMin": m["targetFlowMlMin"],
                        "targetFlowLph": m["targetFlowLph"],
                        "dosingFlowLph": m["dosingFlowLph"],
                        "direction": m["direction"],
                        "preferredReverse": m["preferredReverse"],
                        "mlPerRevCw": m["mlPerRevCw"],
                        "mlPerRevCcw": m["mlPerRevCcw"],
                        "dosingRemainingMl": m["dosingRemainingMl"],
                        "uptimeSec": m["uptimeSec"],
                        "totalPumpedL": m["totalPumpedL"],
                        "totalHoseL": m["totalHoseL"],
                    }
                )
            return base


class FirmwareApiServer:
    def __init__(self, host: str = "127.0.0.1", port: int = 0) -> None:
        self.model = PumpModel()
        model = self.model

        class Handler(BaseHTTPRequestHandler):
            def _parsed(self) -> tuple[str, dict[str, list[str]]]:
                parsed = urlparse(self.path)
                return parsed.path, parse_qs(parsed.query)

            def _motor_id_from_query(self, query: dict[str, list[str]], default: int = 0) -> int | None:
                value = query.get("motorId", [default])[0]
                return model._read_motor_id(value, default=default)

            def _motor_id_from_body(self, body: dict[str, Any] | None, default: int = 0) -> int | None:
                if body is None:
                    return model._read_motor_id(default, default=default)
                return model._read_motor_id(body.get("motorId"), default=default)

            def _json_response(self, code: int, payload: dict[str, Any]) -> None:
                body = json.dumps(payload).encode("utf-8")
                self.send_response(code)
                self.send_header("Content-Type", "application/json")
                self.send_header("Access-Control-Allow-Origin", "*")
                self.send_header("Access-Control-Allow-Headers", "Content-Type")
                self.send_header("Access-Control-Allow-Methods", "GET,POST,OPTIONS")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)

            def _read_body(self) -> dict[str, Any] | None:
                try:
                    length = int(self.headers.get("Content-Length", "0"))
                except ValueError:
                    return None
                raw = self.rfile.read(length) if length > 0 else b"{}"
                try:
                    return json.loads(raw.decode("utf-8"))
                except json.JSONDecodeError:
                    return None

            def do_OPTIONS(self) -> None:  # noqa: N802
                self._json_response(200, {"ok": True})

            def do_GET(self) -> None:  # noqa: N802
                path, query = self._parsed()

                if path == "/":
                    body = WEB_UI.encode("utf-8")
                    self.send_response(200)
                    self.send_header("Content-Type", "text/html")
                    self.send_header("Content-Length", str(len(body)))
                    self.end_headers()
                    self.wfile.write(body)
                    return

                if path == "/api/state":
                    motor_id = self._motor_id_from_query(query, default=model.selected_motor_id)
                    if motor_id is None:
                        self._json_response(400, {"error": "invalid motorId"})
                        return
                    self._json_response(200, model.to_state(motor_id))
                    return

                if path == "/api/settings":
                    motor_id = self._motor_id_from_query(query, default=model.selected_motor_id)
                    if motor_id is None:
                        self._json_response(400, {"error": "invalid motorId"})
                        return
                    self._json_response(
                        200,
                        {
                            "motorId": motor_id,
                            "mlPerRevCw": model.ml_per_rev_cw,
                            "mlPerRevCcw": model.ml_per_rev_ccw,
                            "dosingFlowLph": abs(model.dosing_speed * model.ml_per_rev_cw * 0.06),
                            "maxFlowLph": model.max_speed * model.ml_per_rev_cw * 0.06,
                            "ntpServer": model.ntp_server,
                            "growthProgramEnabled": model.growth_program_enabled,
                            "phRegulationEnabled": model.ph_regulation_enabled,
                            "firmwareUpdate": {
                                "repo": model.firmware_repo,
                                "assetName": model.firmware_asset_name,
                                "filesystemAssetName": model.firmware_fs_asset_name,
                                "currentVersion": "3.1.0-esp32-sim",
                            },
                            "expansion": {
                                "enabled": model.expansion_enabled,
                                "interface": model.expansion_interface,
                                "motorCount": model.expansion_motor_count,
                                "connected": model.expansion_connected,
                                "address": 0x2A if model.expansion_connected else 0,
                            },
                        },
                    )
                    return

                if path == "/api/wifi":
                    self._json_response(
                        200,
                        {
                            "wifiConnected": model.wifi_connected,
                            "ssid": model.ssid,
                            "ip": model.ip,
                            "configPortalSsid": "PeristalticPump-Setup",
                        },
                    )
                    return

                if path == "/api/firmware/config":
                    self._json_response(
                        200,
                        {
                            "repo": model.firmware_repo,
                            "assetName": model.firmware_asset_name,
                            "filesystemAssetName": model.firmware_fs_asset_name,
                            "currentVersion": "3.1.0-esp32-sim",
                        },
                    )
                    return

                if path == "/api/firmware/probe":
                    url = str(query.get("url", [""])[0]).strip()
                    if not url:
                        self._json_response(400, {"error": "url query arg is required"})
                        return
                    if not (url.startswith("http://") or url.startswith("https://")):
                        self._json_response(400, {"error": "url must be valid http(s) url"})
                        return
                    self._json_response(
                        200,
                        {
                            "ok": True,
                            "url": url,
                            "statusCode": 200,
                            "contentLength": 1024,
                        },
                    )
                    return

                if path == "/api/firmware/releases":
                    self._json_response(
                        200,
                        {
                            "repo": model.firmware_repo,
                            "assetName": model.firmware_asset_name,
                            "filesystemAssetName": model.firmware_fs_asset_name,
                            "currentVersion": "3.1.0-esp32-sim",
                            "releases": model.firmware_releases,
                        },
                    )
                    return

                if path == "/api/schedule":
                    self._json_response(
                        200,
                        {
                            "tzOffsetMinutes": model.tz_offset_minutes,
                            "entries": model.schedule_entries,
                        },
                    )
                    return

                self._json_response(404, {"error": "not found"})

            def do_POST(self) -> None:  # noqa: N802
                body = self._read_body()
                path, _ = self._parsed()

                if path == "/api/start":
                    motor_id = self._motor_id_from_body(body, default=0)
                    if motor_id is None:
                        self._json_response(400, {"error": "invalid motorId"})
                        return
                    with model._lock:
                        if abs(model.last_manual_speed) < 0.01:
                            model.last_manual_speed = 120.0
                        model.mode = 0
                        model.target_speed = model.last_manual_speed
                        model.running = True
                    self._json_response(200, model.to_state(motor_id))
                    return

                if path == "/api/stop":
                    motor_id = self._motor_id_from_body(body, default=0)
                    if motor_id is None:
                        self._json_response(400, {"error": "invalid motorId"})
                        return
                    with model._lock:
                        model.target_speed = 0.0
                        model.running = False
                        model.mode = 0
                    self._json_response(200, model.to_state(motor_id))
                    return

                if path == "/api/flow":
                    if body is None or not isinstance(body.get("litersPerHour"), (int, float)):
                        self._json_response(400, {"error": "litersPerHour is required"})
                        return
                    motor_id = self._motor_id_from_body(body, default=0)
                    if motor_id is None:
                        self._json_response(400, {"error": "invalid motorId"})
                        return
                    lph = float(body["litersPerHour"])
                    if lph < 0:
                        self._json_response(400, {"error": "litersPerHour must be >= 0"})
                        return
                    reverse = bool(body.get("reverse", False))

                    with model._lock:
                        ml_per_rev = model.ml_per_rev_ccw if reverse else model.ml_per_rev_cw
                        speed = (lph * 1000.0 / 60.0) / ml_per_rev
                        if reverse:
                            speed *= -1.0
                        model.mode = 0
                        model.target_speed = max(-model.max_speed, min(model.max_speed, speed))
                        model.last_manual_speed = model.target_speed
                        model.running = abs(model.target_speed) >= 0.01
                    self._json_response(200, model.to_state(motor_id))
                    return

                if path == "/api/dosing":
                    if body is None or not isinstance(body.get("volumeMl"), (int, float)):
                        self._json_response(400, {"error": "volumeMl is required"})
                        return
                    motor_id = self._motor_id_from_body(body, default=0)
                    if motor_id is None:
                        self._json_response(400, {"error": "invalid motorId"})
                        return
                    volume = float(body["volumeMl"])
                    if volume <= 0:
                        self._json_response(400, {"error": "volumeMl must be > 0; use reverse=true"})
                        return
                    reverse = bool(body.get("reverse", False))

                    with model._lock:
                        model.mode = 1
                        model.dosing_remaining_ml = float(abs(volume))
                        model.target_speed = -model.dosing_speed if reverse else model.dosing_speed
                        model.running = True
                    self._json_response(200, model.to_state(motor_id))
                    return

                if path == "/api/settings":
                    if body is None:
                        self._json_response(400, {"error": "invalid json"})
                        return
                    motor_id = self._motor_id_from_body(body, default=0)
                    if motor_id is None:
                        self._json_response(400, {"error": "invalid motorId"})
                        return
                    with model._lock:
                        if isinstance(body.get("mlPerRevCw"), (int, float)) and body["mlPerRevCw"] > 0:
                            model.ml_per_rev_cw = float(body["mlPerRevCw"])
                        if isinstance(body.get("mlPerRevCcw"), (int, float)) and body["mlPerRevCcw"] > 0:
                            model.ml_per_rev_ccw = float(body["mlPerRevCcw"])
                        if isinstance(body.get("dosingFlowLph"), (int, float)) and body["dosingFlowLph"] > 0:
                            model.dosing_speed = (float(body["dosingFlowLph"]) * 1000.0 / 60.0) / model.ml_per_rev_cw
                        if isinstance(body.get("maxFlowLph"), (int, float)):
                            max_flow = float(body["maxFlowLph"])
                            if max_flow <= 0:
                                self._json_response(400, {"error": "maxFlowLph must be > 0"})
                                return
                            model.max_speed = (max_flow * 1000.0 / 60.0) / model.ml_per_rev_cw
                        if isinstance(body.get("ntpServer"), str):
                            if not body["ntpServer"]:
                                self._json_response(400, {"error": "ntpServer cannot be empty"})
                                return
                            model.ntp_server = body["ntpServer"]
                        if isinstance(body.get("growthProgramEnabled"), bool):
                            model.growth_program_enabled = body["growthProgramEnabled"]
                        if isinstance(body.get("phRegulationEnabled"), bool):
                            model.ph_regulation_enabled = body["phRegulationEnabled"]

                        if isinstance(body.get("expansion"), dict):
                            exp = body["expansion"]
                            if isinstance(exp.get("enabled"), bool):
                                model.expansion_enabled = exp["enabled"]
                            if isinstance(exp.get("motorCount"), (int, float)):
                                model.expansion_motor_count = max(0, min(4, int(exp["motorCount"])))
                            if isinstance(exp.get("interface"), str) and exp["interface"] in {"i2c", "rs485", "uart"}:
                                model.expansion_interface = exp["interface"]
                        model.target_speed = max(-model.max_speed, min(model.max_speed, model.target_speed))
                        model.current_speed = max(-model.max_speed, min(model.max_speed, model.current_speed))
                        model.last_manual_speed = max(-model.max_speed, min(model.max_speed, model.last_manual_speed))
                        model.dosing_speed = min(abs(model.dosing_speed), model.max_speed)
                    self._json_response(200, model.to_state(motor_id))
                    return

                if path == "/api/calibration/run":
                    if body is None or not isinstance(body.get("direction"), str):
                        self._json_response(400, {"error": "direction is required (cw/ccw)"})
                        return
                    motor_id = self._motor_id_from_body(body, default=0)
                    if motor_id is None:
                        self._json_response(400, {"error": "invalid motorId"})
                        return
                    revs = int(body.get("revolutions", 200))
                    if revs <= 0:
                        self._json_response(400, {"error": "revolutions must be > 0"})
                        return
                    direction = body["direction"]
                    with model._lock:
                        ml_per_rev = model.ml_per_rev_ccw if direction == "ccw" else model.ml_per_rev_cw
                        volume = ml_per_rev * revs
                        model.mode = 1
                        model.dosing_remaining_ml = abs(volume)
                        model.target_speed = -model.dosing_speed if direction == "ccw" else model.dosing_speed
                        model.running = True
                    self._json_response(200, model.to_state(motor_id))
                    return

                if path == "/api/calibration/apply":
                    if body is None or not isinstance(body.get("direction"), str):
                        self._json_response(400, {"error": "direction is required (cw/ccw)"})
                        return
                    motor_id = self._motor_id_from_body(body, default=0)
                    if motor_id is None:
                        self._json_response(400, {"error": "invalid motorId"})
                        return
                    measured = float(body.get("measuredMl", -1))
                    revs = float(body.get("revolutions", -1))
                    if measured <= 0 or revs <= 0:
                        self._json_response(400, {"error": "measuredMl and revolutions must be > 0"})
                        return
                    calibrated = measured / revs
                    with model._lock:
                        if body["direction"] == "ccw":
                            model.ml_per_rev_ccw = calibrated
                        else:
                            model.ml_per_rev_cw = calibrated
                    self._json_response(200, model.to_state(motor_id))
                    return

                if path == "/api/wifi/reset":
                    with model._lock:
                        model.wifi_connected = False
                    self._json_response(200, {"ok": True})
                    return

                if path == "/api/firmware/config":
                    if body is None:
                        self._json_response(400, {"error": "invalid json"})
                        return
                    repo = body.get("repo")
                    asset_name = body.get("assetName")
                    fs_asset_name = body.get("filesystemAssetName")
                    with model._lock:
                        if isinstance(repo, str):
                            model.firmware_repo = repo.strip()
                        if isinstance(asset_name, str):
                            model.firmware_asset_name = asset_name.strip() or "firmware.bin"
                        if isinstance(fs_asset_name, str):
                            model.firmware_fs_asset_name = fs_asset_name.strip() or "littlefs.bin"
                    self._json_response(
                        200,
                        {
                            "repo": model.firmware_repo,
                            "assetName": model.firmware_asset_name,
                            "filesystemAssetName": model.firmware_fs_asset_name,
                            "currentVersion": "3.1.0-esp32-sim",
                        },
                    )
                    return

                if path == "/api/firmware/upload/filesystem":
                    self._json_response(200, {"ok": True, "message": "filesystem uploaded"})
                    return

                if path == "/api/firmware/upload/firmware":
                    self._json_response(200, {"ok": True, "message": "firmware uploaded, restarting"})
                    return

                if path == "/api/firmware/update":
                    if body is None:
                        self._json_response(400, {"error": "invalid json"})
                        return
                    mode = str(body.get("mode", "latest"))
                    tag = str(body.get("tag", ""))
                    direct_url = str(body.get("url", ""))
                    direct_fs_url = str(body.get("filesystemUrl", body.get("fsUrl", "")))
                    with model._lock:
                        if mode == "latest":
                            selected = model.firmware_releases[0]
                        elif mode == "tag":
                            selected = next((r for r in model.firmware_releases if r.get("tag") == tag), None)
                            if selected is None:
                                self._json_response(404, {"error": "github release request failed"})
                                return
                        elif mode == "url":
                            if not direct_url.startswith(("http://", "https://")) or not direct_fs_url.startswith(("http://", "https://")):
                                self._json_response(400, {"error": "url and filesystemUrl must be valid http(s) urls when mode=url"})
                                return
                            selected = {"tag": "local"}
                        else:
                            self._json_response(400, {"error": "mode must be latest, tag or url"})
                            return
                    response = {
                        "ok": True,
                        "tag": selected["tag"],
                        "assetName": model.firmware_asset_name,
                        "filesystemAssetName": model.firmware_fs_asset_name,
                        "message": "firmware and filesystem updated, restarting",
                    }
                    if mode == "url":
                        response["url"] = direct_url
                        response["filesystemUrl"] = direct_fs_url
                    self._json_response(200, response)
                    return

                if path == "/api/zigbee/send":
                    if body is None or not isinstance(body.get("payload"), str):
                        self._json_response(400, {"error": "payload is required"})
                        return
                    with model._lock:
                        model.last_zigbee_payload = body["payload"]
                    self._json_response(200, {"sent": True})
                    return

                if path == "/api/schedule":
                    if body is None:
                        self._json_response(400, {"error": "invalid json"})
                        return
                    with model._lock:
                        model.tz_offset_minutes = int(body.get("tzOffsetMinutes", model.tz_offset_minutes))
                        model.schedule_entries = list(body.get("entries", []))
                    self._json_response(200, {"ok": True})
                    return

                self._json_response(404, {"error": "not found"})

            def log_message(self, format: str, *args: Any) -> None:  # noqa: A003
                return

        self._server = ThreadingHTTPServer((host, port), Handler)
        self.host, self.port = self._server.server_address
        self._server_thread: threading.Thread | None = None

    def start(self) -> None:
        self.model.start()
        self._server_thread = threading.Thread(target=self._server.serve_forever, daemon=True)
        self._server_thread.start()

    def stop(self) -> None:
        self._server.shutdown()
        self._server.server_close()
        if self._server_thread is not None:
            self._server_thread.join(timeout=1.0)
        self.model.stop()
