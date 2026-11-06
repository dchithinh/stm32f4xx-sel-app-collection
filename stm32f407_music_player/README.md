# STM32F407 Discovery music player

Plays `music/a.mp3` from the HAT microSD card, with an LVGL touch UI, and analog
audio on the Discovery **3.5 mm headphone jack** (CS43L22).

Firmware is linked for the **app slot** used by `stm32f4xx_sel_app_bl`
(bootloader at `0x08000000`, this app at `0x08010000`).

## Hardware

| Function | Pins / notes |
|----------|----------------|
| MCU | STM32F407VG, 168 MHz (HSE 8 MHz) |
| LCD ILI9341 | SPI2: PB13 SCK, PB15 MOSI, PC2 MISO, PB9 CS, PD10 RST, PD9 DC |
| Touch XPT2046 | SPI3: PC10/11/12, PE3 CS, PE5 IRQ |
| microSD | SPI3, PE4 CS (same HAT wiring as the bootloader) |
| CS43L22 jack | I2C PB6/PB9, I2S3 PA4 WS, PC7 MCLK, PC10 CK, PC12 SD, PD4 reset |

SPI3 is shared by SD, touch, and I2S3 audio. While a track is playing, I2S owns
SPI3: the UI still redraws on SPI2, the blue user button (PA0) toggles play/pause,
and a finger on the screen also pauses. After pause/stop, SPI3 is SPI again so
touch and SD work normally.

PB9 is LCD CS and codec SDA. The codec is initialized first; later volume writes
temporarily remap PB9 to I2C.

### Known limitation (current HAT)

On this HAT revision, **microSD** and **I2S3 (headphone codec)** share **SPI3** with **touch**. The firmware switches SPI3 between SPI (SD/touch) and I2S audio; playback must pause/stop before SD and full touch use are reliable. A **next HAT version** is planned to move **microSD and touch to SPI1**, leaving I2S3 dedicated to audio so bus sharing is no longer an issue.

## SD card layout

Format **FAT32**. Place:

```
/apps/music_player.bin     ← built image for the bootloader
/music/a.mp3               ← track to play
```

## Build

```bash
pio run
```

Outputs:

| File | Use |
|------|-----|
| `.pio/build/black_f407vg/music_player_0x08010000.bin` | App image (byte 0 = flash `0x08010000`; `PROGNAME` in `extra_pio.py`) |
| `sd_card/apps/music_player.bin` | Copy to microSD `/apps/` |

ST-Link (bootloader already present):

```bash
pio run -t upload
```

Upload address is `0x08010000`. Do not flash this `.bin` at `0x08000000`.
