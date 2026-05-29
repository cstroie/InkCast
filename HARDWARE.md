# InkCast — Bill of Materials & Wiring Guide

**Project:** e-Paper Weather Display
**MCU:** ESP32-C3 SuperMini
**Display:** GDEM029C90 2.9" B/W/R
**Revision:** A — 2026-05-29

---

## Bill of Materials

| Ref | Component | Value / Part | Package | Qty | Notes |
|-----|-----------|--------------|---------|-----|-------|
| U1 | Charger module | TP4056 + DW01A | Module (18×10mm) | 1 | Use version with DW01A protection IC and USB-C input |
| U2 | LDO voltage regulator | MCP1700-3302E | TO-92 or SOT-23 | 1 | 3.3V output, 250mA, 178mV dropout |
| U3 | Microcontroller | ESP32-C3 SuperMini | Module | 1 | USB CDC on boot must be enabled in firmware |
| D1 | ePaper display | GDEM029C90 | FPC connector | 1 | 2.9" 296×128 B/W/R, SSD1680 controller |
| BT1 | LiPo battery | 3.7V 1000mAh | JST-PH 2.0mm 2-pin | 1 | Ensure connector matches TP4056 BAT pins |
| J1 | Charge connector | USB-C receptacle | Through-hole or module | 1 | Already on TP4056 module — no separate part needed |
| C1 | Ceramic capacitor | 1µF 10V | 0402 or 0603 | 1 | LDO input bypass |
| C2 | Ceramic capacitor | 1µF 10V | 0402 or 0603 | 1 | LDO output bypass |
| C3 | Electrolytic capacitor | 220µF 6.3V low-ESR | Radial 5mm | 1 | **Must be low-ESR** — tantalum or polymer; absorbs ESP32 WiFi current peaks |

---

## Wiring

### 1. Charging circuit — USB-C → TP4056

The TP4056 module has its own USB-C port. This is the **only charging port** on the device.
The ESP32-C3 SuperMini USB-C port is **not** used for charging — only for firmware flashing and CDC serial.

| TP4056 pin | Connects to | Wire colour (suggested) |
|------------|-------------|------------------------|
| IN+ (USB-C VBUS) | USB-C connector on module (onboard) | — (already wired on module) |
| IN− (USB-C GND) | USB-C connector on module (onboard) | — (already wired on module) |

Nothing to wire here — the USB-C input is already on the TP4056 module board.

---

### 2. TP4056 → LiPo battery

| TP4056 pin | Battery pin | Notes |
|------------|-------------|-------|
| BAT+ | BT1 + (JST red wire) | Protected charge output |
| BAT− | BT1 − (JST black wire) | |

---

### 3. TP4056 → MCP1700 LDO (power path)

The MCP1700 is powered from the **OUT+ / OUT−** pins of the TP4056 (after the DW01A protection IC), not from the BAT pins directly.

| TP4056 pin | MCP1700 pin | Notes |
|------------|-------------|-------|
| OUT+ | IN (pin 1) | 3.0–4.2V battery voltage into LDO |
| OUT− | GND (pin 2) | Common ground |

---

### 4. MCP1700 LDO — bypass capacitors

All capacitors connect between the respective node and GND.

| Capacitor | Node | Notes |
|-----------|------|-------|
| C1 (1µF ceramic) | MCP1700 IN — GND | Place as close to the IN pin as possible |
| C2 (1µF ceramic) | MCP1700 OUT — GND | Place as close to the OUT pin as possible |
| C3 (220µF low-ESR) | MCP1700 OUT — GND | Place as close to the SuperMini 3V3 pin as possible |

---

### 5. MCP1700 LDO → ESP32-C3 SuperMini

> **Important:** The SuperMini 3V3 pin is used here as a **power input**, bypassing the onboard LDO. Do not connect anything to the SuperMini 5V pin in this build.

| MCP1700 pin | SuperMini pin | Notes |
|-------------|---------------|-------|
| OUT (pin 3) | 3V3 | 3.3V regulated supply |
| GND (pin 2) | GND | Common ground |

---

### 6. MCP1700 LDO → ePaper display power

The display is powered from the same 3.3V rail.

| MCP1700 OUT node | Display pin | Notes |
|------------------|-------------|-------|
| OUT (3.3V rail) | VCC | Shared with SuperMini 3V3 |
| GND | GND | Common ground |

---

### 7. SPI bus — ESP32-C3 SuperMini → GDEM029C90

Hardware SPI2 on native IO MUX pins — fastest, no GPIO matrix routing.

| SuperMini pin | Display pin | Signal | Notes |
|---------------|-------------|--------|-------|
| GPIO7 | DIN | SPI MOSI | Data to display |
| GPIO6 | CLK | SPI SCK | Clock |
| GPIO10 | CS | SPI CS | Chip select, active low |

---

### 8. Control lines — ESP32-C3 SuperMini → GDEM029C90

| SuperMini pin | Display pin | Signal | Notes |
|---------------|-------------|--------|-------|
| GPIO3 | DC | Data/Command | High = data, Low = command |
| GPIO1 | RST | Reset | Active low — safe, no boot conflict |
| GPIO0 | BUSY | Busy out | Input on MCU; wait for LOW before next command |

---

### 9. Onboard peripherals (already on SuperMini PCB — no external wiring)

| SuperMini pin | Function | Notes |
|---------------|----------|-------|
| GPIO8 | Blue LED | Active low, 10kΩ series resistor onboard |
| GPIO9 | BOOT / Button | Active low, pull-up onboard; reused by InkCast firmware as force-refresh |

---

## Ground connections summary

All the following must share a **common GND**:

- TP4056 IN− and OUT−
- BT1 −
- MCP1700 GND (pin 2)
- C1, C2, C3 negative terminals
- SuperMini GND
- Display GND

Tie all GND points together. On a perfboard, run a dedicated GND bus rail.

---

## 3.3V rail connections summary

All the following connect to the **MCP1700 OUT** node:

- C2 positive terminal
- C3 positive terminal
- SuperMini 3V3 pin
- Display VCC pin

---

## Assembly notes

1. **Solder C3 as close to the SuperMini 3V3 pin as possible.** It is the bulk reservoir for WiFi current spikes. Distance adds inductance and defeats its purpose.
2. **Do not connect the SuperMini 5V pin** to anything. It is unused and is not part of this power chain.
3. **TP4056 Rprog resistor** sets charge current. The default onboard resistor on most modules is 1.2kΩ, giving 500mA charge current — correct for a 1000mAh cell (0.5C rate). Verify your module's Rprog before use.
4. **GPIO0 (BUSY)** is an input on the MCU. The GDEM029C90 BUSY pin is an output — it is driven by the display, not the MCU. No pull-up needed; the display drives it actively.
5. **GPIO1 (RST) and GPIO3 (DC)** are outputs from the MCU. RST on the display is active-low. Ensure firmware holds it high after init.
6. **SPI.begin(6, -1, 7, 10)** must be called explicitly in firmware to lock hardware SPI2 to the native IO MUX pins. The `-1` is MISO (unused for this display).
7. **USB-C on the SuperMini** remains independent. You can plug it in for flashing or monitoring without disturbing the battery power path.
8. **Deep sleep:** in deep sleep the MCP1700 quiescent current (~1.6µA) and ESP32-C3 deep sleep current (~5µA) dominate. Total standby draw is approximately 7µA, giving theoretical runtime of several months on a 1000mAh cell before recharge.
