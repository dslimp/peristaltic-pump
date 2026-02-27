from __future__ import annotations

import json
import os
import urllib.error
import urllib.request
from typing import Any

import pytest


HW_BASE = os.getenv("PERISTALTIC_HW_BASE", "").strip().rstrip("/")


def http_json(url: str, method: str = "GET", payload: dict | None = None, timeout: float = 8.0) -> tuple[int, dict]:
    body = None
    headers = {"Content-Type": "application/json"}
    if payload is not None:
        body = json.dumps(payload).encode("utf-8")

    req = urllib.request.Request(url=url, data=body, method=method, headers=headers)
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            return resp.status, json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        raw = e.read().decode("utf-8")
        return e.code, json.loads(raw)


pytestmark = pytest.mark.skipif(not HW_BASE, reason="PERISTALTIC_HW_BASE is not set")


def test_live_state_and_settings_roundtrip() -> None:
    code, state = http_json(f"{HW_BASE}/api/state")
    assert code == 200
    assert "firmware" in state
    assert "activeMotorCount" in state

    code, settings = http_json(f"{HW_BASE}/api/settings?motorId=0")
    assert code == 200
    assert "growthProgramEnabled" in settings
    assert "phRegulationEnabled" in settings

    code, updated = http_json(
        f"{HW_BASE}/api/settings",
        method="POST",
        payload={
            "motorId": 0,
            "growthProgramEnabled": bool(settings.get("growthProgramEnabled", False)),
            "phRegulationEnabled": bool(settings.get("phRegulationEnabled", False)),
        },
    )
    assert code == 200
    assert "firmware" in updated


def test_live_schedule_crud_roundtrip() -> None:
    code, original = http_json(f"{HW_BASE}/api/schedule")
    assert code == 200
    original_entries = list(original.get("entries", []))
    original_tz = int(original.get("tzOffsetMinutes", 0))

    try:
        first = {
            "name": "HW Test A",
            "motorId": 0,
            "enabled": True,
            "hour": 9,
            "minute": 5,
            "volumeMl": 11,
            "reverse": False,
            "weekdaysMask": 0b00011111,
        }
        code, out = http_json(
            f"{HW_BASE}/api/schedule",
            method="POST",
            payload={"tzOffsetMinutes": original_tz, "entries": [first]},
        )
        assert code == 200
        assert out.get("ok") is True

        code, schedule = http_json(f"{HW_BASE}/api/schedule")
        assert code == 200
        assert len(schedule.get("entries", [])) == 1
        assert schedule["entries"][0]["name"] == "HW Test A"
        assert int(schedule["entries"][0]["volumeMl"]) == 11

        first_edit = dict(first)
        first_edit["name"] = "HW Test A Edit"
        first_edit["hour"] = 10
        first_edit["minute"] = 15
        first_edit["volumeMl"] = 22
        code, _ = http_json(
            f"{HW_BASE}/api/schedule",
            method="POST",
            payload={"tzOffsetMinutes": original_tz, "entries": [first_edit]},
        )
        assert code == 200

        code, schedule = http_json(f"{HW_BASE}/api/schedule")
        assert code == 200
        assert len(schedule.get("entries", [])) == 1
        assert schedule["entries"][0]["name"] == "HW Test A Edit"
        assert int(schedule["entries"][0]["hour"]) == 10
        assert int(schedule["entries"][0]["minute"]) == 15
        assert int(schedule["entries"][0]["volumeMl"]) == 22

        second = {
            "name": "HW Test B",
            "motorId": 0,
            "enabled": True,
            "hour": 12,
            "minute": 0,
            "volumeMl": 7,
            "reverse": False,
            "weekdaysMask": 0b00101010,
        }
        code, _ = http_json(
            f"{HW_BASE}/api/schedule",
            method="POST",
            payload={"tzOffsetMinutes": original_tz, "entries": [first_edit, second]},
        )
        assert code == 200

        code, schedule = http_json(f"{HW_BASE}/api/schedule")
        assert code == 200
        assert len(schedule.get("entries", [])) == 2

        code, _ = http_json(
            f"{HW_BASE}/api/schedule",
            method="POST",
            payload={"tzOffsetMinutes": original_tz, "entries": [first_edit]},
        )
        assert code == 200
        code, schedule = http_json(f"{HW_BASE}/api/schedule")
        assert code == 200
        assert len(schedule.get("entries", [])) == 1

        code, _ = http_json(
            f"{HW_BASE}/api/schedule",
            method="POST",
            payload={"tzOffsetMinutes": original_tz, "entries": []},
        )
        assert code == 200
        code, schedule = http_json(f"{HW_BASE}/api/schedule")
        assert code == 200
        assert len(schedule.get("entries", [])) == 0
    finally:
        http_json(
            f"{HW_BASE}/api/schedule",
            method="POST",
            payload={"tzOffsetMinutes": original_tz, "entries": original_entries},
        )


def test_live_firmware_release_list() -> None:
    code, payload = http_json(f"{HW_BASE}/api/firmware/releases?repo=dslimp/peristaltic-pump", timeout=20.0)
    assert code == 200
    assert "releases" in payload
    assert isinstance(payload["releases"], list)


def test_live_firmware_probe_endpoint() -> None:
    probe_url = "https://github.com/dslimp/peristaltic-pump/releases/latest/download/firmware.bin"
    code, payload = http_json(f"{HW_BASE}/api/firmware/probe?url={probe_url}", timeout=20.0)
    assert code == 200
    assert payload.get("ok") is True
    assert int(payload.get("statusCode", 0)) == 200


def _to_float(value: Any, fallback: float) -> float:
    try:
        return float(value)
    except (TypeError, ValueError):
        return fallback


def _to_int(value: Any, fallback: int) -> int:
    try:
        return int(value)
    except (TypeError, ValueError):
        return fallback


def test_live_full_config_roundtrip_with_restore() -> None:
    code, state = http_json(f"{HW_BASE}/api/state")
    assert code == 200
    active_motor_count = max(1, _to_int(state.get("activeMotorCount"), 1))

    code, original_settings = http_json(f"{HW_BASE}/api/settings?motorId=0")
    assert code == 200
    code, original_fw = http_json(f"{HW_BASE}/api/firmware/config")
    assert code == 200
    code, original_prefs = http_json(f"{HW_BASE}/api/ui/preferences")
    assert code == 200

    original_aliases = list(original_settings.get("motorAliases", []))
    if len(original_aliases) < active_motor_count:
        original_aliases.extend([f"Motor {i}" for i in range(len(original_aliases), active_motor_count)])
    original_aliases = original_aliases[:active_motor_count]

    updated_aliases = [f"HW cfg {i}" for i in range(active_motor_count)]
    original_tz = _to_int(original_settings.get("tzOffsetMinutes"), 0)
    updated_tz = original_tz + 30
    if updated_tz > 840:
        updated_tz = original_tz - 30
    if updated_tz < -720:
        updated_tz = 0

    updated_settings = {
        "motorId": 0,
        "mlPerRevCw": round(_to_float(original_settings.get("mlPerRevCw"), 2.6) + 0.11, 3),
        "mlPerRevCcw": round(_to_float(original_settings.get("mlPerRevCcw"), 2.6) + 0.13, 3),
        "dosingFlowLph": round(max(0.2, _to_float(original_settings.get("dosingFlowLph"), 5.0) + 0.7), 3),
        "maxFlowLph": round(max(0.2, _to_float(original_settings.get("maxFlowLph"), 10.0) + 0.9), 3),
        "ntpServer": "pool.ntp.org" if str(original_settings.get("ntpServer", "")).strip() != "pool.ntp.org" else "time.google.com",
        "tzOffsetMinutes": updated_tz,
        "growthProgramEnabled": not bool(original_settings.get("growthProgramEnabled", False)),
        "phRegulationEnabled": not bool(original_settings.get("phRegulationEnabled", False)),
        "motorAliases": updated_aliases,
        "expansion": {
            "enabled": bool(original_settings.get("expansion", {}).get("enabled", False)),
            "interface": str(original_settings.get("expansion", {}).get("interface", "rs485")),
            "motorCount": _to_int(original_settings.get("expansion", {}).get("motorCount"), 0),
        },
    }
    updated_fw = {
        "repo": "dslimp/peristaltic-pump",
        "assetName": "firmware.bin",
        "filesystemAssetName": "littlefs.bin",
    }
    updated_prefs = {
        "motorId": 0,
        "reverse": not bool(original_prefs.get("reverse", False)),
        "language": "ru" if str(original_prefs.get("language", "en")) == "en" else "en",
    }

    restore_settings = {
        "motorId": 0,
        "mlPerRevCw": _to_float(original_settings.get("mlPerRevCw"), 2.6),
        "mlPerRevCcw": _to_float(original_settings.get("mlPerRevCcw"), 2.6),
        "dosingFlowLph": _to_float(original_settings.get("dosingFlowLph"), 5.0),
        "maxFlowLph": _to_float(original_settings.get("maxFlowLph"), 10.0),
        "ntpServer": str(original_settings.get("ntpServer", "time.google.com")),
        "tzOffsetMinutes": original_tz,
        "growthProgramEnabled": bool(original_settings.get("growthProgramEnabled", False)),
        "phRegulationEnabled": bool(original_settings.get("phRegulationEnabled", False)),
        "motorAliases": original_aliases,
        "expansion": {
            "enabled": bool(original_settings.get("expansion", {}).get("enabled", False)),
            "interface": str(original_settings.get("expansion", {}).get("interface", "rs485")),
            "motorCount": _to_int(original_settings.get("expansion", {}).get("motorCount"), 0),
        },
    }
    restore_fw = {
        "repo": str(original_fw.get("repo", "dslimp/peristaltic-pump")),
        "assetName": str(original_fw.get("assetName", "firmware.bin")),
        "filesystemAssetName": str(original_fw.get("filesystemAssetName", "littlefs.bin")),
    }
    restore_prefs = {
        "motorId": _to_int(original_prefs.get("motorId"), 0),
        "reverse": bool(original_prefs.get("reverse", False)),
        "language": str(original_prefs.get("language", "en")),
    }

    try:
        code, _ = http_json(f"{HW_BASE}/api/settings", method="POST", payload=updated_settings)
        assert code == 200
        code, after_settings = http_json(f"{HW_BASE}/api/settings?motorId=0")
        assert code == 200
        assert bool(after_settings.get("growthProgramEnabled")) == bool(updated_settings["growthProgramEnabled"])
        assert bool(after_settings.get("phRegulationEnabled")) == bool(updated_settings["phRegulationEnabled"])
        assert str(after_settings.get("ntpServer")) == str(updated_settings["ntpServer"])
        assert _to_int(after_settings.get("tzOffsetMinutes"), 0) == _to_int(updated_settings["tzOffsetMinutes"], 0)
        assert list(after_settings.get("motorAliases", []))[:active_motor_count] == updated_aliases

        code, _ = http_json(f"{HW_BASE}/api/firmware/config", method="POST", payload=updated_fw)
        assert code == 200
        code, after_fw = http_json(f"{HW_BASE}/api/firmware/config")
        assert code == 200
        assert after_fw.get("repo") == updated_fw["repo"]
        assert after_fw.get("assetName") == updated_fw["assetName"]
        assert after_fw.get("filesystemAssetName") == updated_fw["filesystemAssetName"]

        code, _ = http_json(f"{HW_BASE}/api/ui/preferences", method="POST", payload=updated_prefs)
        assert code == 200
        code, after_prefs = http_json(f"{HW_BASE}/api/ui/preferences")
        assert code == 200
        assert bool(after_prefs.get("reverse")) == bool(updated_prefs["reverse"])
        assert str(after_prefs.get("language")) == str(updated_prefs["language"])
        assert _to_int(after_prefs.get("motorId"), 0) == _to_int(updated_prefs["motorId"], 0)

        code, alive = http_json(f"{HW_BASE}/api/state")
        assert code == 200
        assert "firmware" in alive
    finally:
        http_json(f"{HW_BASE}/api/settings", method="POST", payload=restore_settings)
        http_json(f"{HW_BASE}/api/firmware/config", method="POST", payload=restore_fw)
        http_json(f"{HW_BASE}/api/ui/preferences", method="POST", payload=restore_prefs)
        code, alive = http_json(f"{HW_BASE}/api/state")
        assert code == 200
        assert "firmware" in alive
