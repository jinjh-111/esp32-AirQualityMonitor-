# ESP32 Desktop Air Monitor

一个可公开复现的 ESP32-C3 桌面环境监测器。它读取 SHT30 温湿度和
ENS160 空气质量数据，在 OLED 上本地显示，用 WS2812 指示空气质量等级，
并提供一个不依赖云服务的局域网网页仪表盘。

网页仪表盘默认使用简体中文。OLED 为节省固件空间使用内置 ASCII 字体，
因此设备端保留英文缩写。

An open, reproducible desktop monitor built with an ESP32-C3. It reads SHT30
temperature/humidity and ENS160 air-quality data, provides OLED and RGB
feedback, and serves a local dashboard with a 24-hour in-memory trend.

See the [architecture diagram](docs/architecture.md),
[assembly guide](docs/assembly.md), and [schematic](docs/schematic.svg).

## Features / 功能

- 2-second sensor sampling / 每 2 秒采样
- 5-minute aggregation with 288-point ring buffer / 5 分钟聚合，保留 24 小时
- OLED pages for climate, air quality, network and health / OLED 轮播页面
- AQI 1-2 green, 3 yellow, 4-5 red / AQI 分级 RGB 提示
- Wi-FiManager captive portal / 首次启动网页配网
- `GET /api/status` and `GET /api/history` / JSON API
- Offline-first local dashboard; no cloud account required / 局域网运行

> ENS160 的 eCO₂ 是等效 CO₂ 估算值，不是 NDIR 真 CO₂ 浓度。若需要真实
> CO₂，请替换或增加 SCD40/SCD41 等 NDIR 传感器。

## Materials / 物料

| Item | Qty | Notes |
| --- | ---: | --- |
| ESP32-C3-DevKitM-1 | 1 | Main controller and Wi-Fi |
| SHT30 module | 1 | Temperature and humidity |
| ENS160 module | 1 | AQI, TVOC, eCO₂ estimate |
| SSD1306 128x64 I2C OLED | 1 | Local display |
| WS2812B single LED | 1 | Status indicator |
| 330 ohm resistor, 100 uF capacitor | 1 each | Data and power protection |
| Breadboard, jumper wires, USB-C cable | 1 set | Prototype assembly |

See [wiring.md](docs/wiring.md) for the pin map and voltage notes, then follow
the Chinese [assembly guide](docs/assembly.md) and [schematic](docs/schematic.svg).

## Build and flash / 编译烧录

Install [PlatformIO](https://platformio.org/) and connect the ESP32-C3.

```bash
pio run -e esp32-c3
pio run -e esp32-c3 -t upload
pio run -e esp32-c3 -t uploadfs
pio device monitor -b 115200
```

The `uploadfs` command is required for the dashboard HTML in `data/`.

## Wi-Fi setup / 配网

On first boot, the device tries saved Wi-Fi credentials. If it cannot connect,
join the `ESP32-AirMonitor` access point (password `airmonitor`) and complete
the captive-portal form. The device then reboots and prints its local IP over
serial. Open that IP or `http://air-monitor.local/` from the same LAN.

No Wi-Fi password belongs in this repository. Reset saved credentials by
calling `wifiManager.resetSettings()` temporarily in a development build, or
erase the board's flash before provisioning it again.

## API / 接口

`GET /api/status` returns current readings, sensor health, uptime and network
state. `GET /api/history` returns the volatile 5-minute points oldest-first.

```json
{
  "readings": { "temperatureC": 23.4, "humidityPercent": 46.2,
    "aqi": 2, "tvocPpb": 85, "eco2Ppm": 612 },
  "historyPoints": 12,
  "wifi": { "connected": true, "ip": "192.168.1.42" }
}
```

## Test / 测试

```bash
pio test -e native
pio run -e esp32-c3
```

The native tests cover the history buffer's empty, chronological and
overwrite behavior. Hardware acceptance checks are documented in
[`docs/verification.md`](docs/verification.md).

## GitHub release / 发布

Create a public repository named `esp32-air-quality-monitor`, then push:

```bash
git add .
git commit -m "feat: initial ESP32 desktop air monitor"
git branch -M main
git remote add origin https://github.com/<your-user>/esp32-air-quality-monitor.git
git push -u origin main
```

The workflow in `.github/workflows/platformio.yml` builds the firmware and runs
the native tests on every push and pull request. Tag a passing commit as
`v0.1.0` for the first release. Photos, a wiring image and a short demo video
can be added under `docs/` after the physical prototype is assembled.

## Roadmap / 后续计划

- NDIR CO₂ sensor support
- Optional flash-backed history
- OTA firmware updates
- MQTT and Home Assistant integration

Released under the [MIT License](LICENSE).
