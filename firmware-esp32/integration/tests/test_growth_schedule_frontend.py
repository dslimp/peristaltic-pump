from __future__ import annotations

import json
import shutil
import subprocess
import sys
from functools import lru_cache
from pathlib import Path

import pytest


REPO_ROOT = Path(__file__).resolve().parents[3]
GROWTH_SCHEDULER_JS = REPO_ROOT / "firmware-esp32" / "data" / "growth-schedule.js"
APP_JS = REPO_ROOT / "firmware-esp32" / "data" / "app.js"
INDEX_HTML = REPO_ROOT / "firmware-esp32" / "data" / "index.html"
RELEASE_WORKFLOW = REPO_ROOT / ".github" / "workflows" / "release-artifacts.yml"


def _can_run_node() -> bool:
    if not shutil.which("node"):
        return False
    try:
        probe = subprocess.run(
            ["node", "-e", "process.stdout.write('ok')"],
            check=False,
            capture_output=True,
            text=True,
            timeout=2,
        )
    except (OSError, subprocess.TimeoutExpired):
        return False
    return probe.returncode == 0 and probe.stdout.strip() == "ok"


def _can_run_osascript() -> bool:
    if not shutil.which("osascript"):
        return False
    try:
        probe = subprocess.run(
            ["osascript", "-l", "JavaScript", "-e", "console.log('ok')"],
            check=False,
            capture_output=True,
            text=True,
            timeout=2,
        )
    except (OSError, subprocess.TimeoutExpired):
        return False
    return probe.returncode == 0 and "ok" in probe.stdout


@lru_cache(maxsize=1)
def _runtime() -> str | None:
    if sys.platform == "darwin":
        if _can_run_osascript():
            return "osascript"
        return None
    if _can_run_osascript():
        return "osascript"
    if _can_run_node():
        return "node"
    return None


def _run_growth_scheduler(payload: dict) -> dict:
    payload_json = json.dumps(payload)
    runtime = _runtime()

    if runtime == "node":
        script = (
            f"const scheduler = require({json.dumps(str(GROWTH_SCHEDULER_JS))});\n"
            f"const payload = JSON.parse({json.dumps(payload_json)});\n"
            "const result = scheduler.generateGrowthScheduleEntries(payload);\n"
            "process.stdout.write(JSON.stringify(result));\n"
        )
        out = subprocess.run(
            ["node", "-e", script],
            check=True,
            capture_output=True,
            text=True,
            timeout=8,
        )
        return json.loads(out.stdout)

    if runtime == "osascript":
        script = (
            "ObjC.import('Foundation');\n"
            f"const path = {json.dumps(str(GROWTH_SCHEDULER_JS))};\n"
            "const source = ObjC.unwrap($.NSString.stringWithContentsOfFileEncodingError(path, $.NSUTF8StringEncoding, null));\n"
            "eval(source);\n"
            f"const payload = JSON.parse({json.dumps(payload_json)});\n"
            "const result = GrowthSchedule.generateGrowthScheduleEntries(payload);\n"
            "console.log(JSON.stringify(result));\n"
        )
        out = subprocess.run(
            ["osascript", "-l", "JavaScript", "-e", script],
            check=True,
            capture_output=True,
            text=True,
            timeout=8,
        )
        lines = [line.strip() for line in out.stdout.splitlines() if line.strip()]
        return json.loads(lines[-1])

    pytest.skip("No JS runtime available (node or osascript)")


def _minutes(entry: dict) -> int:
    return int(entry["hour"]) * 60 + int(entry["minute"])


def _base_payload() -> dict:
    return {
        "programName": "Test Program",
        "activeMotorCount": 5,
        "phase": {
            "nutrients": {"a": 1.0, "b": 1.0, "c": 1.0},
            "ph": {"plus": 0.5, "minus": 0.5},
        },
        "waterL": 20,
        "phRegulationEnabled": True,
        "nutrientHour": 18,
        "nutrientMinute": 0,
        "nutrientMask": 0x7F,
        "phHour": 18,
        "phMinute": 0,
        "phMask": 0x7F,
        "pauseMinutes": 10,
    }


def test_growth_schedule_spreads_all_channels_globally() -> None:
    payload = _base_payload()

    result = _run_growth_scheduler(payload)
    entries = result["entries"]

    assert len(entries) == 5
    times = [_minutes(entry) for entry in entries]
    assert times == [1080, 1090, 1100, 1110, 1120]
    assert len(set(times)) == len(times)


def test_growth_schedule_without_ph_stays_sequential() -> None:
    payload = _base_payload()
    payload["phRegulationEnabled"] = False
    payload["pauseMinutes"] = 15
    payload["nutrientHour"] = 9
    payload["nutrientMinute"] = 5

    result = _run_growth_scheduler(payload)
    entries = result["entries"]

    assert len(entries) == 3
    times = [_minutes(entry) for entry in entries]
    assert times == [545, 560, 575]


def test_growth_schedule_reports_motor_remap_count() -> None:
    payload = _base_payload()
    payload["activeMotorCount"] = 2

    result = _run_growth_scheduler(payload)
    entries = result["entries"]

    assert result["remapped"] == 3
    assert max(int(entry["motorId"]) for entry in entries) == 1


def test_growth_schedule_name_truncation_keeps_utf8_valid() -> None:
    payload = _base_payload()
    payload["programName"] = "Aquatica TriPart Земляника Очень Длинный Профиль"

    result = _run_growth_scheduler(payload)
    name = str(result["entries"][0]["name"])

    assert "�" not in name
    assert len(name.encode("utf-8")) <= 32


def test_release_workflow_manual_flash_uses_esp32s3() -> None:
    text = RELEASE_WORKFLOW.read_text(encoding="utf-8")

    assert "esptool --chip esp32s3 --port <PORT> --baud 921600 write_flash -z" in text


def test_growth_ui_has_plant_and_fertilizer_selectors() -> None:
    html = INDEX_HTML.read_text(encoding="utf-8")

    assert 'id="growthPlant"' in html
    assert 'id="growthFertilizer"' in html


def test_growth_catalog_includes_popular_plants_and_fertilizers() -> None:
    text = APP_JS.read_text(encoding="utf-8")

    assert "id: 'gh-floraseries'" in text
    assert "id: 'an-ph-perfect'" in text
    assert "id: 'foxfarm-trio'" in text
    assert "id: 'kale'" in text
    assert "id: 'arugula'" in text
    assert "id: 'parsley'" in text
    assert "id: 'cilantro'" in text
