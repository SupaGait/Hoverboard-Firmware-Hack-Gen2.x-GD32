# CarrierHasArrived — board B (split hoverboard)

Board A is the single mainboard, flashed from `hoverboard-firmware-hack-FOC` branch `cursor/carrier-single-board-bd10`. This tree is board B: two half-boards, one hub each. The ESP32 sends both hubs the same left/right pair. This firmware makes the split pair look like that single board.

## What the ESP32 sends

115200 8N1, little-endian, start word `0xABCD`. One UART, to the **master** half only.

| Command field | Hub |
|---------------|-----|
| `steer` | left, the master half |
| `speed` | right, the slave half |

Both fields are −1000…1000. 1000 asks for 1000 rpm. With frames arriving, speed mode holds the wheel, so 0 holds 0 rpm. Feedback is the 18-byte `VARIANT_USART` frame: `speedL_meas` and `speedR_meas` in rpm, pack volts × 100, board temperature × 10. This tree does not sample the die sensor, so `boardTemp` is 0.

A positive left command returns a positive `speedL_meas`. A positive right command returns a negative `speedR_meas`, the same sign as the single board with `INVERT_R_DIRECTION` left unset. The ESP32 inverts stay at their defaults; fix a mirrored mount on the phone.

If the ESP32 goes quiet for 800 ms, both hubs release (PWM off), including the slave. If the slave half is not answering, the master sends no feedback, so the ESP32 treats this axle as down and holds the other board at zero.

## Flash two different images

Default pinout is GD32F130 layout 2.1.1 (`LAYOUT 1` in `Inc/config.h`). USART0 on PB6/PB7 is the ESP32. USART1 on PA2/PA3 stays the 4-pin cable between the halves. Set `LAYOUT` to the number on your board before building; `Inc/defines/defines_2-target-version.h` includes the matching pin file.

```bash
cd HoverBoardGigaDevice
make test-carrier
make carrier-master    # build/carrier-master.bin  → half wired to the ESP32
make carrier-slave     # build/carrier-slave.bin   → the other half
make -C Bootloader            # master BL (USART0 + slave relay)
make -C Bootloader SLAVE=1    # slave BL (USART1 host)
```

Apps link at `0x08001000` after a 4 KiB UART IAP bootloader. Flash each half once with its bootloader at `0x08000000`, then the matching app at `0x08001000`. Later updates use Carrier SoftAP: `target=B` for the master, `target=B-slave` for the slave (master bootloader relays). Protocol: CarrierHasArrived `docs/BOARD_IAP.md`.

PlatformIO, same split:

```bash
pio run -e genericGD32F130C8          # master
pio run -e carrier_slave_GD32F130     # slave
```

Wiring to the ESP32-S3, 3.3 V, shared ground. Do not connect a sensor-cable 15 V wire. ESP32 GPIO15 (TX) goes to the master RX, GPIO16 (RX) to the master TX. Measure the idle level on PB6/PB7 and level-shift if that header sits at 5 V.

Current limit is 15 A, the same class as the single board. 10S pack. The reverse beep is off, and the board does not latch itself off after sitting still.
