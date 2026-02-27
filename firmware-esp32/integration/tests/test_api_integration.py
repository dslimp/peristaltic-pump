from __future__ import annotations

import json
import time
import urllib.error
import urllib.request

import pytest

from integration.firmware_api_sim import FirmwareApiServer


@pytest.fixture()
def api_server() -> tuple[FirmwareApiServer, str]:
    server = FirmwareApiServer()
    server.start()
    base = f"http://127.0.0.1:{server.port}"
    try:
        yield server, base
    finally:
        server.stop()


def http_json(url: str, method: str = "GET", payload: dict | None = None) -> tuple[int, dict]:
    body = None
    headers = {"Content-Type": "application/json"}
    if payload is not None:
        body = json.dumps(payload).encode("utf-8")

    req = urllib.request.Request(url=url, data=body, method=method, headers=headers)
    try:
        with urllib.request.urlopen(req, timeout=3.0) as resp:
            return resp.status, json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        raw = e.read().decode("utf-8")
        return e.code, json.loads(raw)


def test_flow_start_stop(api_server: tuple[FirmwareApiServer, str]) -> None:
    _, base = api_server

    code, state = http_json(f"{base}/api/flow", method="POST", payload={"motorId": 1, "litersPerHour": 6.0, "reverse": True})
    assert code == 200
    assert state["motorId"] == 1
    assert state["activeMotorCount"] >= 2
    assert isinstance(state["motors"], list) and len(state["motors"]) >= 2
    assert state["direction"] == "reverse"
    assert abs(abs(state["targetFlowLph"]) - 6.0) < 0.25

    code, state = http_json(f"{base}/api/start", method="POST", payload={"motorId": 1})
    assert code == 200
    assert state["running"] is True

    code, state = http_json(f"{base}/api/stop", method="POST", payload={"motorId": 1})
    assert code == 200
    assert state["running"] is False

    code, err = http_json(f"{base}/api/flow", method="POST", payload={"motorId": 99, "litersPerHour": 1.0})
    assert code == 400
    assert err["error"] == "invalid motorId"


def test_dosing_and_calibration(api_server: tuple[FirmwareApiServer, str]) -> None:
    _, base = api_server

    code, state = http_json(f"{base}/api/dosing", method="POST", payload={"volumeMl": 5})
    assert code == 200
    assert state["modeName"] == "dosing"

    deadline = time.time() + 5.0
    while time.time() < deadline:
        _, state = http_json(f"{base}/api/state")
        if state["modeName"] == "flow_lph" and state["running"] is False and state["dosingRemainingMl"] <= 0:
            break
        time.sleep(0.05)
    assert state["running"] is False

    code, state = http_json(
        f"{base}/api/calibration/apply",
        method="POST",
        payload={"direction": "cw", "revolutions": 200, "measuredMl": 700},
    )
    assert code == 200
    assert abs(state["mlPerRevCw"] - 3.5) < 1e-6


def test_settings_and_validation(api_server: tuple[FirmwareApiServer, str]) -> None:
    _, base = api_server

    code, settings = http_json(f"{base}/api/settings?motorId=1")
    assert code == 200
    assert settings["motorId"] == 1
    assert "expansion" in settings
    assert settings["maxFlowLph"] > 0
    assert settings["growthProgramEnabled"] is False
    assert settings["phRegulationEnabled"] is False

    code, state = http_json(
        f"{base}/api/settings",
        method="POST",
        payload={
            "motorId": 1,
            "mlPerRevCw": 3.1,
            "mlPerRevCcw": 3.2,
            "dosingFlowLph": 12.0,
            "maxFlowLph": 20.0,
            "growthProgramEnabled": True,
            "phRegulationEnabled": True,
            "expansion": {"enabled": True, "interface": "i2c", "motorCount": 4},
        },
    )
    assert code == 200
    assert state["motorId"] == 1
    assert abs(state["mlPerRevCw"] - 3.1) < 1e-6

    code, settings = http_json(f"{base}/api/settings?motorId=1")
    assert code == 200
    assert settings["growthProgramEnabled"] is True
    assert settings["phRegulationEnabled"] is True

    code, err = http_json(f"{base}/api/settings", method="POST", payload={"maxFlowLph": 0})
    assert code == 400
    assert err["error"] == "maxFlowLph must be > 0"


def test_wifi_and_schedule(api_server: tuple[FirmwareApiServer, str]) -> None:
    server, base = api_server

    code, payload = http_json(f"{base}/api/zigbee/send", method="POST", payload={"payload": "{\"cmd\":\"stop\"}"})
    assert code == 200
    assert payload["sent"] is True
    assert server.model.last_zigbee_payload == "{\"cmd\":\"stop\"}"

    code, payload = http_json(
        f"{base}/api/schedule",
        method="POST",
        payload={
            "tzOffsetMinutes": 180,
            "entries": [{"enabled": True, "hour": 9, "minute": 0, "volumeMl": 50, "reverse": False, "motorId": 0}],
        },
    )
    assert code == 200
    assert payload["ok"] is True

    code, schedule = http_json(f"{base}/api/schedule")
    assert code == 200
    assert schedule["tzOffsetMinutes"] == 180
    assert len(schedule["entries"]) == 1
    assert schedule["entries"][0]["motorId"] == 0


def test_firmware_update_endpoints(api_server: tuple[FirmwareApiServer, str]) -> None:
    _, base = api_server

    code, config = http_json(f"{base}/api/firmware/config")
    assert code == 200
    assert config["assetName"] == "firmware.bin"
    assert config["filesystemAssetName"] == "littlefs.bin"

    code, config = http_json(
        f"{base}/api/firmware/config",
        method="POST",
        payload={"repo": "acme/peristaltic", "assetName": "firmware.bin", "filesystemAssetName": "littlefs.bin"},
    )
    assert code == 200
    assert config["repo"] == "acme/peristaltic"

    code, releases = http_json(f"{base}/api/firmware/releases")
    assert code == 200
    assert isinstance(releases["releases"], list)
    assert len(releases["releases"]) >= 1

    code, payload = http_json(f"{base}/api/firmware/update", method="POST", payload={"mode": "latest"})
    assert code == 200
    assert payload["ok"] is True
    assert "tag" in payload
    assert payload["filesystemAssetName"] == "littlefs.bin"

    code, payload = http_json(
        f"{base}/api/firmware/update",
        method="POST",
        payload={
            "mode": "url",
            "url": "http://192.168.88.10:18080/firmware.bin",
            "filesystemUrl": "http://192.168.88.10:18080/littlefs.bin",
        },
    )
    assert code == 200
    assert payload["ok"] is True
    assert payload["tag"] == "local"
    assert payload["url"].endswith("/firmware.bin")
