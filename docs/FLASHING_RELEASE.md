# Flashing Release Binaries (ESP32)

For a full release flash with Web UI, `firmware.bin` alone is not enough.
You need firmware + bootloader + partition table + filesystem image.

Release artifacts now include:

- `bootloader.bin`
- `partitions.bin`
- `boot_app0.bin` (if present for the platform package)
- `firmware.bin`
- `littlefs.bin`
- `flash_macos_linux.sh`
- `flash_windows.ps1`
- `FLASHING.md` (bundle-specific instructions with exact FS offset)

## macOS

1. Extract the artifact zip.
2. Find serial port (for example `/dev/tty.usbserial-0001`).
3. Run:

```bash
chmod +x flash_macos_linux.sh
./flash_macos_linux.sh /dev/tty.usbserial-0001 921600
```

## Linux

1. Extract the artifact zip.
2. Find serial port (for example `/dev/ttyUSB0`).
3. Run:

```bash
chmod +x flash_macos_linux.sh
./flash_macos_linux.sh /dev/ttyUSB0 921600
```

## Windows (PowerShell)

1. Extract the artifact zip.
2. Find COM port in Device Manager (for example `COM3`).
3. Run:

```powershell
./flash_windows.ps1 -Port COM3 -Baud 921600
```

## Notes

- The release workflow computes filesystem offset from `partitions.bin`, so scripts use correct address for that build.
- If needed, open `FLASHING.md` inside the artifact for the exact manual `esptool` command.
