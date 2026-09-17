# 🌱 CropSentry

**A solar-powered field sensor node that watches crops for stress signals before symptoms are visible.**

**_______________________________________________________________________________________**

### 🔧 *What It Is*

**CropSentry** is a battery-and-solar sensor node that sits in a field and measures the **volatile organic compounds a plant emits**, alongside soil moisture and ambient conditions. Plants under stress change their VOC output *before* visible symptoms appear, so a node reading those compounds continuously can flag a problem earlier than a person walking rows.

Each node reports over **LoRa** to a gateway, which forwards to a server and then to a phone. LoRa is the point: the design target is **mass agriculture**, where nodes sit far outside WiFi coverage. One gateway serves many nodes across a field.

This repo covers the **hardware**, specifcally sensing, power, and enclosure. The application and server are built separately by my teammate for the Congressional App Challenge.

**--------------**

### ⚙️ *How It Works*

The node is built around a **Heltec WiFi LoRa 32 V3** (ESP32-S3 + SX1262). Four sensors hang off it:

- **SGP41** — the *primary* VOC path. A dedicated metal-oxide VOC/NOx sensor with a heated sensing element, feeding Sensirion's VOC Index algorithm. This is the sensor doing the actual stress detection.
- **BME688** — *secondary* gas sensing plus temperature, humidity, and pressure. Its MOX element responds broadly to VOCs, which makes it a useful cross-check against the SGP41 rather than a replacement for it. (We have this because VOCs are hard to track in a plastic/agricultural environment, so we reference cross-checking as the next best logical move)
- **SHT45** — the *ambient reference*. Gas readings drift with temperature and humidity, so without a clean baseline you cannot separate a real plant signal from the air simply getting more humid. This is the least glamorous sensor in the stack and arguably the one that makes the data mean anything.
- **Capacitive soil moisture probe** — soil water content on an ADC pin. **Capacitive, not resistive**: resistive probes electrolyze and corrode away within weeks in damp soil, which would end a season-long deployment early.

All three gas/climate sensors share one **I²C bus**. The soil probe is analog on **GPIO2**.

**--------------**

### 🔋 *Power System*

```
6V 3W panel ──> Schottky ──> CN3065 charger ──> protected 18650 ──> Heltec VBAT
                                                       │
                                            load runs off the same node
```

There is no switchover circuit. The battery powers the node continuously; the panel tops it up whenever there is sun. During the day the panel covers the load and the surplus charges the cell, at night the cell carries it.

**Design decisions worth explaining:**

- **The charger is a CN3065, not a TP4056.** A TP4056 expects a stiff 5V USB source. Fed from a panel, it drags the panel below its maximum power point under partial cloud and charges almost nothing. The CN3065 has input voltage regulation that holds the panel near Vmp.
- **The Heltec's onboard charger is bypassed.** It is a basic linear Li-ion charger with no MPPT and no input current limiting; the classic way a naive solar node fails.
- **Two independent cutoffs, neither hand-built.** Charge termination (CC/CV to 4.2V) lives inside the CN3065. Over-discharge protection at 2.75V lives on the cell's own BMS. Building a comparator-based cutoff was considered and rejected: a stuck or chattering cutoff puts 7V panel open-circuit into a lithium cell inside a sealed box, unattended, in a field.
- **The panel is oversized ~4×.** Estimated node draw is roughly 70 mAh/day duty-cycled, or 150–200 mAh/day if the SGP41 runs continuously to hold its VOC Index baseline. A 3W panel massively overserves that on a sunny day. It is sized for the **worst week**, not the best day — three overcast days is when nodes die.

**--------------**

### 🧩 *Build*

The current node is **handwired from through-hole breakout modules**, not a fabricated PCB. The KiCad schematic in this repo is a wiring reference that stays PCB-ready if the design moves to a board later.

Because the sensors are breakouts, they carry their own decoupling capacitors and I²C pull-ups — no discrete passives are populated in the handwired build.

- **Enclosure:** 3D printed in **PET-G** (PLA for prototyping, then PET-G for testing)
- **Soil probe interface:** 3-position screw terminal, so the probe can be swapped without desoldering
- **Bulk capacitance:** 1000 µF electrolytic at the Heltec input, to absorb the ~120 mA current burst during LoRa transmit. Without it, handwired leads develop enough voltage drop to brown out the ESP32 mid-transmit — a failure that presents as random reboots.

**--------------**

### 📐 *Schematic*

![Schematic PNG](https://github.com/CropSentry/Hardware/blob/main/CropSentry%20SCH-1.png)

![Schematic PDF Link](https://github.com/CropSentry/Hardware/blob/main/CropSentry%20SCH.pdf)


**--------------**

### 📋 *Bill of Materials*

| Part | Function | Qty | Price | Status |
|---|---|---:|---:|---|
| **SGP41** | Primary VOC/NOx index sensor | 1 | $12.79 | *To buy* |
| **BME688** | Secondary gas + temp/humidity/pressure | 1 | $20.89 | ✅ Purchased |
| **SHT45** | Ambient temp/humidity reference | 1 | $14.99 | ✅ Purchased |
| **Capacitive soil probe** (EK1940, 2pk) | Soil water content, corrosion resistant | 1 | $9.99 | ✅ Purchased |
| **Heltec ESP32 LoRa V3** (w/ 1100 mAh cell + antenna) | MCU + SX1262 LoRa radio | 1 | $26.49 | ✅ Purchased |
| **ZPSHYD 6V 3W solar panel** | Monocrystalline, 145 × 145 mm | 1 | $14.59 | *To buy* |
| **XINLANTECH 18650 2600 mAh** | Protected Li-ion, built-in BMS | 1 | $11.72 | *To buy* |
| **CN3065 solar charge module** (3pk) | 500 mA solar Li-ion charger | 1 | $6.99 | *To buy* |
| **JST PH2.0 connector kit** (30pc) | Panel / battery / load interconnect | 1 | $7.99 | *To buy* |
| **XT60 switch** | Battery disconnect | 1 | $8.99 | *To buy* |
| **Triple screw terminal** | Soil probe interface | 1 | — | 🔧 On hand |
| **Schottky diode** (40V 5A) | Reverse current blocking | 1 | — | 🔧 On hand |
| **Bulk electrolytic** (1000 µF 10V) | LoRa TX transient smoothing | 1 | — | 🔧 On hand |

| | |
|---|---:|
| Subtotal | $135.43 |
| Tax (7%) | $9.48 |
| **Total** | **$144.91** |
| Spent so far | $77.43 |
| **Remaining** | **$67.48** |

**--------------**

### ⚠️ *Known Limitations*

Stated up front rather than discovered later:

- **Firmware is untested.** Nothing in this repo has been validated against real sensor output.
- **No validation data yet.** VOC baselines, soil probe noise floor, and actual deep-sleep current have not been measured. The power budget above is estimated, not measured — the Heltec V3's real deep sleep current varies widely by board revision and by whether the OLED and power LED are disabled, and at low duty cycle that current *is* the entire budget.
- **The prototype enclosure is 3D printed, which is a known contamination risk.** Heated thermoplastic outgasses VOCs, and this node's primary sensor measures VOCs. PETG was chosen over PLA and ABS for lower residual emission, and the print is aired out before assembly, but a sealed-enclosure baseline run is required before any VOC reading can be trusted. CNC-machined parts is a legitimate possibility once prototyping is complete.
- **Duty cycle vs. sensor calibration is unresolved.** Sensirion's VOC Index algorithm builds a rolling baseline and expects regular sampling. Deep-sleeping between reads to save power may break that baseline. This is a genuine tension between the sensing approach and the power approach.
- **Radio range is unverified.** The showcase target is 0.25 miles. Not yet measured.

- **Enclosure CAD is currently in progress. The initial physical layout and sensor placement strategy is mapped out in [enclosure_sketch.png](https://github.com/CropSentry/Hardware/blob/main/enclosure-sketch.png), focusing on isolating the SHT45 ambient sensor from internal plastic outgassing**

**--------------**

### 🏆 *Credits*

- **Hardware, power system, enclosure, and node firmware:** ***[Cai Griffith](https://www.caigriffith.dev)*** — *https://github.com/CropSentry/Hardware*
- **Application and server backend/frontend:** ***Aahan Patel*** — *https://github.com/CropSentry/MobileApp*
- **ML Design and backend:** ***Jay Patel*** - *https://github.com/CropSentry/ML-Model*

CropSentry is a three-person project built for the **Congressional App Challenge**, and approved as a research project through **Middle Georgia State University CyberKnights**.

**--------------**

***This README file was generated by Claude AI, but was only generated from the context of the presentation I made entirely. All materials, ideas, concepts, and details of the CropSentry hardware project are designed by me.*** 

**Link to presentation that I created:** ***[Presentation View Link](https://github.com/CropSentry/Hardware/blob/main/CropSentry%20Materials/CropSentry/Presentation/CropSentry%20MGA%20Presentation.pdf)***

**--------------**
