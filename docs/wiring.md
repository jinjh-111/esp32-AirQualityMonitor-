# Wiring / 接线

完整的分阶段组装、上电检查和手机配网步骤见
[`assembly.md`](assembly.md)。逐孔面包板布局见
[`breadboard-layout.md`](breadboard-layout.md)，原理图见 [`schematic.svg`](schematic.svg)。

| ESP32-C3 | SHT30 | ENS160 | SSD1306 | WS2812 |
| --- | --- | --- | --- | --- |
| 3V3 | VIN/VCC | VIN/VCC | VCC | VCC when module supports 3.3 V |
| GND | GND | GND | GND | GND |
| GPIO4 | SDA | SDA | SDA | - |
| GPIO5 | SCL | SCL | SCL | - |
| GPIO8 | - | - | - | DIN through 330 ohm |

Use a 100 uF capacitor across the RGB LED power rails. Keep all modules at
common ground. Check the exact voltage requirement of your WS2812 module
before connecting it; do not feed a 3.3 V-only module with 5 V.

I2C addresses expected by the firmware:

- SHT30: `0x44`
- ENS160: `0x53`
- SSD1306: `0x3C`
