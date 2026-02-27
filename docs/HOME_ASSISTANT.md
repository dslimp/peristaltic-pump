# Home Assistant integration

This guide connects the pump firmware API to Home Assistant (`http://192.168.88.26:8123`).

## 1. Define REST commands

Add to your Home Assistant `configuration.yaml`:

```yaml
rest_command:
  peristaltic_flow:
    url: "http://<pump-ip>/api/flow"
    method: POST
    content_type: "application/json"
    payload: '{"litersPerHour": {{ lph }}, "reverse": {{ reverse | lower }}}'

  peristaltic_dosing:
    url: "http://<pump-ip>/api/dosing"
    method: POST
    content_type: "application/json"
    payload: '{"volumeMl": {{ ml }}, "reverse": {{ reverse | lower }}}'

  peristaltic_stop:
    url: "http://<pump-ip>/api/stop"
    method: POST
```

Replace `<pump-ip>` with the ESP32 address in your LAN.

## 2. Add REST sensor for state

```yaml
sensor:
  - platform: rest
    name: Peristaltic Pump State
    resource: "http://<pump-ip>/api/state"
    value_template: "{{ value_json.modeName }}"
    json_attributes:
      - running
      - direction
      - flowLph
      - totalPumpedL
      - dosingRemainingMl
    scan_interval: 5
```

## 3. Example script actions

```yaml
script:
  pump_dose_250ml:
    alias: Pump dose 250 ml
    sequence:
      - service: rest_command.peristaltic_dosing
        data:
          ml: 250
          reverse: false

  pump_stop:
    alias: Pump stop
    sequence:
      - service: rest_command.peristaltic_stop
```

## 4. Apply config

1. In Home Assistant open **Developer Tools -> YAML**.
2. Reload REST entities and scripts, or restart Home Assistant.
3. Add controls/sensors to a dashboard card.

## Notes

- If web auth is enabled on the pump UI, keep API access available from Home Assistant.
- Recommended HA host for this setup: `192.168.88.26`.
