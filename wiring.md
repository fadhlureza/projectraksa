## Project wiring table

The table below is transcribed from the project's current wiring data. Component labels are used where available.

### AC / mains-side wiring

| # | From | To | Function |
| ---: | --- | --- | --- |
| 1 | CABLE AC 3 PIN - `GND` | Single Socket - `GND` | Protective earth connection |
| 2 | CABLE AC 3 PIN - `L` | MCB 1 Phase - `Vin` | Incoming line to breaker |
| 3 | MCB 1 Phase - `Vout` | Relay Module - `COM` | Breaker output to relay common |
| 4 | Relay Module - `NO` | Single Socket - `L` | Switched line to socket |
| 5 | CABLE AC 3 PIN - `N` | Single Socket - `N` | Neutral to socket |
| 6 | PZEM-004T module - `+ve` | MCB 1 Phase - `Vout` | PZEM line-side connection as drawn |
| 7 | PZEM-004T module - `-ve` | CABLE AC 3 PIN - `N` | PZEM neutral-side connection as drawn |
| 8 | CT PZEM004T - `+` | PZEM-004T module - `+ve` | CT positive connection |
| 9 | CT PZEM004T - `-` | PZEM-004T module - `-ve` | CT negative connection |

### PZEM power and relay control

| # | From | To | Function |
| ---: | --- | --- | --- |
| 10 | PZEM-004T module - `5V` | `5V` connector - `out` | 5 V distribution |
| 11 | Relay Module - `VCC 5V` | `5V` connector - `out` | Relay module 5 V supply |
| 12 | `GND` connector - `out` | Relay Module - `GND` | Relay ground |
| 13 | `GND` connector - `out` | PZEM-004T module - `GND` | PZEM ground |
| 14 | PZEM-004T module - `RX` | RX connector - `Out` | PZEM UART receive-side connection |
| 15 | PZEM-004T module - `TX` | TX connector - `Out` | PZEM UART transmit-side connection |
| 16 | RELAY connector - `Out` | Relay Module - `CH1 / IN1` | Relay control input |
| 17 | RELAY connector - `In` | ESP32 S3 - GPIO `5` | ESP32 relay-control output |
| 18 | ESP32 S3 - `RX` | TX connector - `In` | ESP32 UART receive connection |
| 19 | ESP32 S3 - `TX` | RX connector - `In` | ESP32 UART transmit connection |

The connector labels imply the intended UART crossover:
* ESP32 `TX` -> PZEM `RX`
* ESP32 `RX` -> PZEM `TX`

However, the project's direct wire records route these through connectors rather than directly between the two modules.

### ESP32, power distribution, and ground

| # | From | To | Function |
| ---: | --- | --- | --- |
| 20 | ESP32 S3 - `5V` | Battery Shield v3 - `5V` | 5 V rail connection |
| 21 | `5V` connector - `in` | Battery Shield v3 - `5V` | 5 V distribution input |
| 22 | ESP32 S3 - `G` | Battery Shield v3 - `GND` | Battery shield ground |
| 23 | `GND` connector - `in` | Battery Shield v3 - `GND` | Ground distribution input |

### 18650 Battery / Battery Shield v3 (with 100k ohm Voltage Divider)

| # | From | To | Function |
| ---: | --- | --- | --- |
| 24 | Battery Shield v3 - `4.2v` | 100k ohm resistor (R1) - `pin1` | Divider upper-side / input from battery |
| 25 | 100k ohm resistor (R1) - `pin2` | 100k ohm resistor (R2) - `pin1` | Divider midpoint connection |
| 26 | 100k ohm resistor (R2) - `pin1` | ESP32 S3 - GPIO `4` | ADC connection from divider midpoint |
| 27 | 100k ohm resistor (R2) - `pin2` | ESP32 S3 - `G` | Divider ground-side connection |

*Rangkaian voltage divider menggunakan dua resistor 100k ohm untuk menurunkan tegangan baterai (maks ~4.2 V menjadi ~2.1 V) agar aman dibaca oleh pin ADC ESP32 (GPIO 4).*

### MQ-2 gas sensor (with 4.7k ohm Voltage Divider)

| # | From | To | Function |
| ---: | --- | --- | --- |
| 28 | MQ-2 - `VCC` | ESP32 S3 - `5V` | Sensor 5 V supply |
| 29 | MQ-2 - `GND` | ESP32 S3 - `G` | Sensor ground |
| 30 | MQ-2 - `AO` | 4.7k ohm resistor (R1) - `pin1` | Analog output to divider upper-side |
| 31 | 4.7k ohm resistor (R1) - `pin2` | 4.7k ohm resistor (R2) - `pin1` | Divider midpoint connection |
| 32 | 4.7k ohm resistor (R2) - `pin1` | ESP32 S3 - GPIO `6` | ADC gas-level signal from divider midpoint |
| 33 | 4.7k ohm resistor (R2) - `pin2` | ESP32 S3 - `G` | Divider ground-side connection |

*Pin `DO` (Digital Output) pada MQ-2 tidak terhubung (unconnected).*
*Rangkaian voltage divider menggunakan dua resistor 4.7k ohm untuk menurunkan tegangan sinyal analog output MQ-2 (maks 5 V menjadi maks ~2.5 V) agar sesuai dengan batas tegangan aman ADC ESP32 (GPIO 6).*

### DS18B20 temperature sensor (with 4.7k ohm Pull-up Resistor)

| # | From | To | Function |
| ---: | --- | --- | --- |
| 34 | DS18B20 - `VCC` | ESP32 S3 - `3V3` | Sensor 3.3 V supply |
| 35 | DS18B20 - `GND` | ESP32 S3 - `G` | Sensor ground |
| 36 | 4.7k ohm resistor - `pin1` | ESP32 S3 - `3V3` | Pull-up resistor 3.3 V rail |
| 37 | 4.7k ohm resistor - `pin2` | DS18B20 - `D` | Pull-up connection to 1-Wire data line |
| 38 | DS18B20 - `D` / 4.7k ohm `pin2` | ESP32 S3 - GPIO `7` | Temperature 1-Wire data input |

*Resistor 4.7k ohm berfungsi sebagai pull-up resistor pada jalur data 1-Wire (GPIO 7) ke rail 3.3 V.*

### Micro SD Card Module

| # | From | To | Function |
| ---: | --- | --- | --- |
| 39 | ESP32 S3 - `5V` | Micro SD Card Module - `vcc` | SD module 5 V supply |
| 40 | ESP32 S3 - `G` | Micro SD Card Module - `gnd` | SD module ground |
| 41 | ESP32 S3 - GPIO `14` | SD module - `miso` | SPI MISO |
| 42 | ESP32 S3 - GPIO `13` | SD module - `mosi` | SPI MOSI |
| 43 | ESP32 S3 - GPIO `12` | SD module - `sck` | SPI clock |
| 44 | ESP32 S3 - GPIO `11` | SD module - `cs` | SPI chip select |

### DS3231 RTC

| # | From | To | Function |
| ---: | --- | --- | --- |
| 45 | ESP32 S3 - `3V3` | DS3231 - `VCC` | RTC 3.3 V supply |
| 46 | ESP32 S3 - `G` | DS3231 - `GND` | RTC ground |
| 47 | ESP32 S3 - GPIO `8` | DS3231 - `SDA` | I2C data |
| 48 | ESP32 S3 - GPIO `9` | DS3231 - `SCL` | I2C clock |

*Pin RTC `SQW` dan `32K` saat ini tidak terhubung (unconnected).*

### Door sensor (MC-38)

| # | From | To | Function |
| ---: | --- | --- | --- |
| 49 | Door MC-38 - `GND` | ESP32 S3 - `G` | Door sensor ground |
| 50 | Door MC-38 - `S` | ESP32 S3 - GPIO `15` | Door-state signal |

## Unconnected pins identified

The following component pins appear in the parts list but do not appear in the wiring records:
* **ESP32 S3:** many unused GPIO pins, plus `RST`
* **PZEM-004T module:** none of its listed pins are unused in the current model
* **MQ-2:** `DO`
* **DS3231 RTC:** `SQW`, `32K`
* **Single Socket:** all listed pins are connected
* **Relay Module:** `NC` is unused
* **Micro SD Card Module:** all listed pins are connected
* **DS18B20:** all listed pins are connected
* **Door MC-38:** all listed pins are connected

## Static review observations

* The design contains **mains-voltage wiring** involving the cable, MCB, relay, socket, and PZEM connections. The project wiring structure alone does not verify safe insulation, protective-earth continuity, fusing, terminal ratings, or correct PZEM revision-specific wiring.
* The PZEM component currently used in the circuit is represented with pins `RX`, `TX`, `5V`, `GND`, `+ve`, and `-ve`, which differs from the other PZEM metadata representation that uses `3.3V`, `LIN`, `NIN`, `LOUT`, and `NOUT`. These should not be assumed electrically interchangeable without confirming the exact hardware revision.
* The MQ-2 analog output is routed through a 4.7k ohm : 4.7k ohm voltage divider (50% attenuation), bringing the sensor 0-5 V signal into the safe ADC range (0-2.5 V) for the ESP32 ADC on GPIO 6.
* The 18650 battery voltage (max ~4.2 V) is routed through a 100k ohm : 100k ohm voltage divider (50% attenuation), bringing the voltage to ~2.1 V for the ESP32 ADC on GPIO 4.
* The DS18B20 data line uses a dedicated 4.7k ohm pull-up resistor to 3.3 V for stable 1-Wire communication.
* This is an inspection of the saved wiring records, not a simulation or electrical safety verification.