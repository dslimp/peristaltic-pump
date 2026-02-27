# Expansion I2C Protocol (Autodiscovery)

This protocol links the central board (I2C master) with one expansion board (I2C slave) that controls up to 4 stepper motors.

## Autodiscovery

- Central scans addresses `0x20..0x2F`.
- Discovery request frame:
  - `[cmd=0x01, crc]`
- Discovery response frame (6 bytes):
  - `magicA=0x50 ('P')`
  - `magicB=0x58 ('X')`
  - `proto=0x01`
  - `motorCount` (`1..4`)
  - `features` (reserved)
  - `crc`

CRC is XOR of all bytes before CRC.

## Framing

- Request: `[cmd, payload..., crc]`
- Response: `[payload..., crc]`
- CRC: XOR of bytes before CRC.
- Little-endian for multi-byte integers.

## Commands

- `0x01` `HELLO`
  - Req payload: none
  - Resp: 6-byte discovery response

- `0x10` `GET_STATE`
  - Req payload: `motorIdx:u8`
  - Resp payload (28 bytes + crc):
    - `mode:u8` (`0=flow`, `1=dosing`)
    - `running:u8`
    - `targetSpeedX10:i16` (rpm * 10)
    - `currentSpeedX10:i16`
    - `dosingRemainingMl:u16`
    - `uptimeSec:u32`
    - `totalPumpedMl:u32`
    - `totalHoseMl:u32`
    - `mlPerRevCwX100:u16`
    - `mlPerRevCcwX100:u16`
    - `dosingSpeedX10:u16`
    - `maxSpeedX10:u16`

- `0x20` `SET_FLOW`
  - Req payload: `motorIdx:u8, lphX10:u16, reverse:u8`

- `0x21` `START_DOSING`
  - Req payload: `motorIdx:u8, volumeMl:u16, reverse:u8`

- `0x22` `STOP`
  - Req payload: `motorIdx:u8`

- `0x23` `START`
  - Req payload: `motorIdx:u8`

- `0x24` `SET_SETTINGS`
  - Req payload:
    - `motorIdx:u8`
    - `mlPerRevCwX100:u16`
    - `mlPerRevCcwX100:u16`
    - `dosingFlowLphX10:u16`
    - `maxFlowLphX10:u16`

## Firmware environments

- Central board:
  - `pio run -e esp32s3 -t upload --upload-port <PORT_CENTRAL>`
  - `pio run -e esp32s3 -t uploadfs --upload-port <PORT_CENTRAL>`
- Expansion board:
  - `pio run -e esp32s3-expansion -t upload --upload-port <PORT_EXPANSION>`

Do not use plain `pio run -t upload` in this repo, because it may try all environments.

Default expansion board I2C address is `0x2A` in `src/expansion_main.cpp`.
