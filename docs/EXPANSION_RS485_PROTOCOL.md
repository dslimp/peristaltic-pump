# Expansion RS-485 Protocol (Autodiscovery)

This protocol links the central board (RS-485 master) with one expansion board (RS-485 node) that controls up to 4 stepper motors.

## Physical/link layer

- UART settings: `250000 8N1`
- Half-duplex with DE/RE direction control.
- Frame on wire:
  - TX: `[len:u8][payload:len bytes]`
  - RX: `[len:u8][payload:len bytes]`
- Payload format:
  - Request: `[cmd, payload..., crc]`
  - Response: `[payload..., crc]`
- CRC: XOR of all payload bytes before CRC.
- Integer byte order: little-endian.

## Autodiscovery

- Discovery request payload:
  - `[cmd=0x01, crc]`
- Discovery response payload (6 bytes):
  - `magicA=0x50 ('P')`
  - `magicB=0x58 ('X')`
  - `proto=0x01`
  - `motorCount` (`1..4`)
  - `features` (reserved)
  - `crc`

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
