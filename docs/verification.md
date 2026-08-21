# Verification checklist / 验证清单

## Before power-up

- [ ] All modules share GND.
- [ ] SHT30, ENS160 and OLED are powered at the voltage required by their modules.
- [ ] WS2812 data has a 330 ohm series resistor and a 100 uF rail capacitor.

## Firmware checks

- [ ] Serial I2C scan finds `0x44`, `0x53` and `0x3C`.
- [ ] `pio run -e esp32-c3` completes without errors.
- [ ] `pio test -e native` passes all history-buffer tests.
- [ ] `pio run -e esp32-c3 -t uploadfs` installs the dashboard.

## Hardware acceptance

- [ ] Temperature and humidity respond plausibly to room changes.
- [ ] ENS160 reports warming before becoming ready; AQI/TVOC then update.
- [ ] RGB is green for AQI 1-2, yellow for 3, red for 4-5, purple when unavailable.
- [ ] OLED rotates through climate, air, network and device-status pages.
- [ ] Dashboard loads from the device IP and refreshes every 5 seconds.
- [ ] `/api/history` works when empty, partially filled and full.
- [ ] Disconnecting Wi-Fi does not stop local sensor sampling or OLED updates.
- [ ] A two-hour soak test shows no crash or steadily increasing free-memory loss.

