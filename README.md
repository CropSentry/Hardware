# 🌱 CropSentry

**A fully autonomous plant disease/condition detector that runs on solar power and features LoRa capability.**

--------------

### 🔧 *What It Is*

**CropSentry** is an IoT device that automatically and autonomously tracks plant conditions, including VOC, temperature, soil moisture, gas, and humidity. It sits alone in a field and can monitor multiple crop sections at once. We see that through this process, an AI/ML model can use this data to determine the plant’s in-the-moment state, as well as provide an educated and accurate summary of what can show as a potential risk. 

The data path of how info will get to a user will start from the on-board module. (Cai Griffith) From there, it sends information via a LoRa connection to another board that is connected to a Wi-Fi router. The board will then send over the Wi-Fi to a high-power server (https://gear.museumofaviation.org/tech-specs/) and will compute all readings with an AI/Ml system. (Jay Patel) After it’s been computed, the server will send the translated data to the CropSentry app. (Aahan Patel) From there, the user can read charts, numbers, and an AI summary of the condition of their crop.

**To summarize:**

- It's a plant disease/condition detector.
- It's a module that sends collected data to a powerful server.
- It's a system of sensors full of cross-checking for accuracy.


![Introduction Image 1](https://github.com/CropSentry/Hardware/blob/main/assets/CropSentry_Image_12.PNG)


--------------

### ⚙️ *How It Works*

The board chosen for the hardware design is the ***Heltec LoRa V3*** due to its high-level radio capabilities and adjustable range/power accessibility.

- **SGP41:** Detects VOC/NOx air pollution over a period of time using its metal-oxide sensor abilities. Sends its information through Sensiron’s built-in VOC translating algorithm. Primary gas sensor for accurate readings. Chosen for it's potentcy to collect accurate information under low power and time.
- **BME688:** Looks specifically for temperature and humidity, but also serves as a secondary VOC sensor. Since VOCs are hard to track accurately, the BME688 can act as another cross-check system. Chosen for its high versatility and efficiency.
- **SHT45:**  Serves as the ambient system for temperature and humidity sensing. We also see this as another cross-check system, but it still carries weight in how data is calculated for the user. Chosen due to its low power, reduced pins, and sensor cross-checking abilities.
- **Capacitive Soil Moisture Probe:**  Detects soil moisture and processes all data on an ADC pin using analog information. I chose to use capacitive so the design wouldn’t corrode or get damaged during a deployment.

Each sensor except the Soil Moisture Probe is connected via **I2C Connection**. The Soil Moisture Probe is connected to **GPIO 2** to receive analog signals.

--------------

### 🔋 *Power System*

```
6V 3W panel ──> Schottky ──> CN3065 charger ──> protected 18650 ──> Heltec VBAT
                                                       │
                                            load runs off the same node
```
--------------

### 💡 *Design Decisions*

- **Range:** This can be a concern, as we are not completely sure how far the LoRa system/data will transmit. This would mostly be because we are still working on designing the model (10/8/2026) and are not able to go into the testing phase yet.
- **Inaccurate Readings:** Specifically in the VOC/NOx sensors, where there is vulnerability due to them needing to be tested over time and for growth consistency. We are virtually trusting the sensors’ datasheets to determine our readings.
- **Water Contamination**: Rainwater (especially heavy rainwater) has a possibility of breaking through the 3D-printed layer protecting the circuits. We are actively looking for a solution to this concern and are considering using remedies such as rubber seals, CNC-milled products, and absorbent material.
- **Calibration:** We are also relying on the datasheet for accurate calibration of all sensors. Similar to the BME688, we are taking multiple readings and using the median. We also have an embedded safeguard to not send data that is unrealistic and not plausible, such as, for example, a temperature reading showing as -200 Celsius.
- **Frequency:** We decided, on much advice and legal regulations, to go with the 900-915 MHz band, which is also known as the ISM band (Industrial, Scientific, and Medical band).
- **Coding Libraries:** For the hardware coding, I used the *heltec_unofficial.h*, *WiFIManager.h*, *HTTPClient.h*, *Wire.h*, *Adafruit_BME680*, and *Adafruit_SHT4x.h* libraries.


--------------


### 📐 *Schematic Images/PDF*

![Schematic PNG](https://github.com/CropSentry/Hardware/blob/main/assets/CropSentry_Image_5.png)

[![View Schematic on KiCanvas](https://hack.club/pcb-badge)](https://github.com/CropSentry/Hardware/blob/main/sch-files/CropSentry.kicad_sch)


--------------

### 🛠️ *CAD Images/3D Models*

![CAD Image 1](https://github.com/CropSentry/Hardware/blob/main/assets/CropSentry_Image_2.png)

![3D Model Image 1](https://github.com/CropSentry/Hardware/blob/main/assets/CropSentry_Image_6.jpg)

![3D Model Image 2](https://github.com/CropSentry/Hardware/blob/main/assets/CropSentry_Image_7.jpg)


--------------


### 📋 *Bill of Materials*

| Part | Function | Qty | Price | Status |
| Part | Role | Qty | Cost | Status |
|---|---|---:|---:|---|
| **SGP41** | Primary VOC/NOx index sensor | 1 | $12.79 | *To buy* |
| **BME688** | Secondary gas + temp/humidity/pressure | 1 | $20.89 | ✅ Purchased |
| **SHT45** | Ambient temp/humidity reference | 1 | $14.99 | ✅ Purchased |
| **Capacitive soil probe** (EK1940, 2pk) | Soil water content, corrosion resistant | 1 | $9.99 | ✅ Purchased |
| **Heltec ESP32 LoRa V3** (w/ 1100 mAh cell + antenna) | MCU + SX1262 LoRa radio | 2 | $26.49 | ✅ 1 Purchased · 1 Donated |
| **ZPSHYD 6V 3W solar panel** | Monocrystalline, 145 × 145 mm | 1 | $14.59 | *To buy* |
| **XINLANTECH 18650 2600 mAh** | Protected Li-ion, built-in BMS | 1 | $11.72 | *To buy* |
| **CN3065 solar charge module** (3pk) | 500 mA solar Li-ion charger | 1 | $6.99 | *To buy* |
| **XT60 switch** | Battery disconnect | 1 | $8.99 | *To buy* |
| **Triple screw terminal** | Soil probe interface | 1 | — | 🔧 On hand |
| **Schottky diode** (40V 5A) | Reverse current blocking | 1 | — | 🔧 On hand |
| **Bulk electrolytic** (1000 µF 10V) | LoRa TX transient smoothing | 1 | — | 🔧 On hand |

### Budget

| Line | Submitted | Actual | Δ |
|---|---:|---:|---:|
| Subtotal | $135.43 | 127.44|-7.99 |
| Tax (7%) | $9.48 | 8.92|-0.56 |
| **Total** | **144.91**|**136.36** | **-$8.55** |
| Spent so far | $77.43 | $77.43 | — |
| **Remaining** | **67.48**|**58.93** | **-$8.55** |


*All on-hand pieces are CURRENTLY funded by me and others and other contributors; any money received from Stardance will go towards buying parts needed and covering part expenses. Due to shipping and time concerns, I had to purchase parts myself to be able to continue building CropSentry on a strong timeline.*


--------------

### ⚡ *Visual Data Flow*

[![Architecture diagram of cropsentry/hardware](https://gitdiagram.com/cropsentry/hardware/diagram.png)](https://gitdiagram.com/cropsentry/hardware?utm_source=readme&utm_medium=picture)

[![Architecture diagram](https://gitdiagram.com/diagram-badge.svg)](https://gitdiagram.com/cropsentry/hardware?utm_source=readme&utm_medium=badge)

![Diagram Image 1](https://github.com/CropSentry/Hardware/blob/main/assets/CropSentry_Image_9.png)
![Diagram Image 2](https://github.com/CropSentry/Hardware/blob/main/assets/CropSentry_Image_10.png)

--------------

### 🏆 *Credits*

- **Hardware, power system, enclosure, and node firmware:** ***[Cai Griffith](https://www.caigriffith.dev)*** — *https://github.com/CropSentry/Hardware*
- **Application and server backend/frontend:** ***[Aahan Patel](https://github.com/HarbingerYt)*** — *https://github.com/CropSentry/MobileApp*
- **ML Design and backend:** ***[Jay Patel](https://github.com/Jay2710-09)*** — *https://github.com/CropSentry/ML-Model*

--------------

### 📄 *Extra Information*

- All CropSentry Hardware is coded in ***C++***

- The receiver code has an example link for where the server link is supposed to go due to security concerns

- CropSentry is a three-person project built for the **Congressional App Challenge** and approved as a research project through the **Middle Georgia State University CyberKnights Program**.

- ***All work regarding software, hardware, and planning is all designed by our team (only high schoolers).*** 

- We do have mentors who only give advice, but ***all work is done by us***.


***Please also keep in mind that some of the files on here, such as code or 3D models, may have inaccuracies, as this is a work in progress. We are still in beta testing and are actively fixing these errors.***

--------------

### 🎬 *Presentation Links*

**Public link to presentation created for a research meeting:** ***[Presentation View Link](https://github.com/CropSentry/Hardware/blob/main/CropSentry%20Materials/CropSentry/Presentation/CropSentry%20MGA%20Presentation.pdf)***

**Public link to presentation created for an update at a research meeting (9-28-2026):** ***[Presentation View Link](https://canva.link/cropsentry-update-mga-presentation-9-28-2026)***

--------------

### 🖼️ *Additional Images*

![Drawing Image 1](https://github.com/CropSentry/Hardware/blob/main/assets/CropSentry_Image_4.png)
![CAD Image 1](https://github.com/CropSentry/Hardware/blob/main/assets/CropSentry_Image_3.png)
![CAD Image 1](https://github.com/CropSentry/Hardware/blob/main/assets/CropSentry_Image_1.png)
