# ELECTENG/COMPSYS 209 Team 15: Smart Energy Monitor
*ELECTENG 209: Analog & Embedded Software Design - The University of Auckland*[cite: 1]

## Overview
This repository contains Team 15's Smart Energy Monitor, a hardware-software co-design project developed for ELECTENG 209[cite: 1]. The system accurately measures, calculates, and displays the power and energy consumption of a simulated household appliance (AC load) to support decentralized energy management[cite: 1].

## Key Features
* **Analog Signal Conditioning:** Custom circuitry to sense and filter 14V RMS, 500Hz AC voltage and current[cite: 1].
* **Embedded Processing:** Uses an ATmega328P/PB microcontroller to digitize signals (ADC) and compute RMS Voltage, Peak Current, Power, and Energy[cite: 1].
* **User Interface:** Real-time data visualization on a local LCD display[cite: 1].
* **Wireless Connectivity:** Transmits telemetry data to a smart device via UART over a Bluetooth (BLE) module[cite: 1].

## Specifications Summary
* **Input Source:** 14 V (RMS) at 500 Hz[cite: 1]
* **Load Range:** 2.5 VA to 7.5 VA (Power Factor: 0.6 to 0.99)[cite: 1]
* **Accuracy:** 5% of full-scale reading[cite: 1]
* **PCB Design:** Altium Designer, Double Layer PTH, max size 20000 mm²[cite: 1]

Our group will get A+ for sure 
