# cpproject-esp32

This is a PlatformIO-based ESP-IDF project for ESP32.

## Getting Started

- Build the project:
  ```bash
  pio run
  ```
- Upload to device:
  ```bash
  pio run --target upload
  ```
- Monitor serial output:
  ```bash
  pio device monitor
  ```

## Project Structure
- `src/` - Source files
- `include/` - Header files
- `lib/` - Private libraries
- `platformio.ini` - PlatformIO configuration

## Requirements
- PlatformIO (install via VS Code extension or `pip install platformio`)
- ESP32 development board

For more details, see the PlatformIO and ESP-IDF documentation.
