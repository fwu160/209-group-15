# ELECTENG/COMPSYS 209 Team 15: Smart Energy Monitor
*ELECTENG 209: Analog & Embedded Software Design - The University of Auckland*

## Overview
This repository contains Team 15's Smart Energy Monitor, a hardware-software co-design project developed for ELECTENG 209. The system accurately measures, calculates, and displays the power and energy consumption of a simulated household appliance (AC load) to support decentralized energy management.

## Key Features
* **Analog Signal Conditioning:** Custom circuitry to sense and filter 14V RMS, 500Hz AC voltage and current.
* **Embedded Processing:** Uses an ATmega328P/PB microcontroller to digitize signals (ADC) and compute RMS Voltage, Peak Current, Power, and Energy.
* **User Interface:** Real-time data visualization on a local LCD display.
* **Wireless Connectivity:** Transmits telemetry data to a smart device via UART over a Bluetooth (BLE) module.

## Specifications Summary
* **Input Source:** 14 V (RMS) at 500 Hz
* **Load Range:** 2.5 VA to 7.5 VA (Power Factor: 0.6 to 0.99)
* **Accuracy:** 5% of full-scale reading
* **PCB Design:** Altium Designer, Double Layer PTH, max size 20000 mm²

Our group will get A+ for sure 
