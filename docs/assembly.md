# 组装方案 / Assembly Guide

这份方案适用于 ESP32-C3-DevKitM-1、SHT30、ENS160、SSD1306 OLED 和单颗 WS2812B 的面包板原型。

## 1. 上电前检查

1. 拔掉 USB-C 电源，确认面包板正负电源轨没有短路。
2. SHT30、ENS160 和 OLED 只接 3.3 V，不能把 5 V 接到 ESP32 的 3V3 轨。
3. 所有模块的 GND 必须连接到 ESP32 GND。
4. WS2812 的 DIN 串联 330 ohm 电阻，电源两端并联 100 uF 电容。
5. 检查 WS2812 模块丝印：若只支持 5 V，使用 USB 的 VBUS/5V 供电；若模块明确支持 3.3 V，才接 3V3。两种电源都必须与 ESP32 共地。

## 2. 面包板布局

建议把 ESP32 插在面包板中间的沟槽上，USB-C 接口朝外。左侧放 OLED，右侧依次放 SHT30 和 ENS160，WS2812 放在靠近面包板边缘的位置，避免 LED 热量直接影响温湿度传感器。

先用两根跳线把 ESP32 的 3V3 和 GND 接到面包板电源轨。I2C 三个模块共用 SDA/SCL，不要把它们串联；每个模块的 VCC 和 GND 分别接电源轨。

## 3. 接线表

| ESP32-C3 | SHT30 | ENS160 | SSD1306 OLED | WS2812B |
| --- | --- | --- | --- | --- |
| 3V3 | VIN/VCC | VIN/VCC | VCC | 仅限 3.3 V 模块 |
| GND | GND | GND | GND | GND |
| GPIO4 | SDA | SDA | SDA | - |
| GPIO5 | SCL | SCL | SCL | - |
| GPIO8 | - | - | - | DIN，经 330 ohm |
| USB VBUS/5V | - | - | - | 5 V WS2812 模块的 VCC |

I2C 地址应为：SHT30 `0x44`、ENS160 `0x53`、SSD1306 `0x3C`。如果模块地址不同，记录地址后再修改 `include/Config.h`。

## 4. 分阶段组装动作

### 阶段 A：只接 OLED

1. 连接 3V3、GND、GPIO4、GPIO5 和 OLED。
2. 烧录固件并打开串口监视器：`pio device monitor -b 115200`。
3. 确认 I2C 扫描能发现 `0x3C`，OLED 显示 `Air Monitor`。

### 阶段 B：加入 SHT30

1. 断电后把 SHT30 的 SDA、SCL 并到 GPIO4/GPIO5。
2. 重新上电，确认扫描出现 `0x44`。
3. 温度、湿度应随环境变化缓慢变化。

### 阶段 C：加入 ENS160

1. 断电后把 ENS160 并到同一组 3.3 V、GND、SDA、SCL。
2. 确认扫描出现 `0x53`。
3. ENS160 预热期间 AQI 可能不可用，OLED 和 RGB 会显示错误/紫色状态；这是正常现象。

### 阶段 D：加入 WS2812

1. 断电，把 GPIO8 接到 330 ohm 电阻，再接 WS2812 DIN。
2. 在 WS2812 VCC 与 GND 之间并联 100 uF 电容，电容正极接 VCC。
3. 确认电源规格后接 3V3 或 USB VBUS/5V，绝不要把 5 V 接到 ESP32 3V3。
4. AQI 1-2 为绿，AQI 3 为黄，AQI 4-5 为红，数据不可用时为紫色闪烁。

## 5. 首次配网和手机查看

1. 首次启动若未连上已保存 Wi-Fi，手机连接热点 `ESP32-AirMonitor`，密码 `airmonitor`。
2. 在手机浏览器完成 Wi-Fi 配置，设备随后重启。
3. 手机切回与 ESP32 相同的 Wi-Fi，打开 OLED 显示的 IP 地址，或访问 `http://air-monitor.local/`。
4. 仪表盘默认是简体中文，实时数据每 5 秒刷新；运行 5 分钟后开始出现第一个历史聚合点。

## 6. 常见问题

- **扫描不到设备**：检查 GND、SDA/SCL 是否接反，以及模块供电电压。
- **只扫描到一个设备**：逐个拔掉模块确认是否有地址冲突或模块损坏。
- **WS2812 不亮或乱闪**：检查 DIN 方向、330 ohm 电阻、共地和电源电压；5 V 供电且线较长时，建议增加 74AHCT125/74HCT14 电平转换器。
- **温度偏高**：把 SHT30/ENS160 远离 ESP32 稳压器和 WS2812，保持通风。
- **eCO2 与真实 CO2 不一致**：ENS160 的 eCO2 是算法估算值，不是真实 NDIR CO2；需要真实浓度时使用 SCD40/SCD41 等 NDIR 传感器。

