from __future__ import annotations

import json
import os
import urllib.error
import urllib.request

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
