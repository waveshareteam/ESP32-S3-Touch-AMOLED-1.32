# Firmware

[English](README.md) | [简体中文](README_ZH.md)

Factory firmware for ESP32-S3-Touch-AMOLED-1.32. The BIN is a merged image flashed at `0x0`; the date in its file name identifies the version.

| File | Purpose |
| --- | --- |
| [ESP32-S3-Touch-AMOLED-1.32_FactoryOnly_20251103.bin](ESP32-S3-Touch-AMOLED-1.32_FactoryOnly_20251103.bin) | Factory firmware, restores the out-of-box demo |

Run these commands in this directory, replacing `<PORT>` with the board's serial port:

```bash
python -m pip install esptool
esptool.py --chip esp32s3 --port <PORT> erase_flash
esptool.py --chip esp32s3 --port <PORT> write_flash 0x0 ESP32-S3-Touch-AMOLED-1.32_FactoryOnly_20251103.bin
```

