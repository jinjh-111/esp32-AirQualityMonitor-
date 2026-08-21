# Architecture / 架构

```text
SHT30 + ENS160 --I2C--> ESP32-C3 --I2C--> SSD1306
                              |
                              +--GPIO8--> WS2812 indicator
                              |
                              +--Wi-Fi--> local HTTP dashboard
```

The main loop is deadline based and non-blocking. Sensor samples arrive every
2 seconds, the OLED rotates every 3 seconds, and valid samples are averaged
into one history point every 5 minutes. The history ring buffer holds 288
points (24 hours) and is intentionally volatile.

