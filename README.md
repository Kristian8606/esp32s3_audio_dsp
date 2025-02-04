# ESP32-S3 Multi-DAC Audio System
[![Donate with PayPal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](
https://www.paypal.com/donate/?business=VGY3ZM78NAL78&no_recurring=0&item_name=Thank+you+for+your+donation+%E2%80%93+your+support+helps+the+project%E2%80%99s+development+and+motivates+us+to+keep+working+hard.&currency_code=EUR)

[PCB LINK](https://easyeda.com/editor#project_id=fa41d6ad6b15413f8a76038f53fb5484)
[oshwlab]( https://oshwlab.com/kristiandimitrov/espdsp)

### Description

This project is a multi-channel audio system based on ESP32-S3 and ProtoDAC x8 with TDA1387, using DIR9001 as an SPDIF receiver. The system enables flexible audio signal processing, including IIR equalization and FIR crossover filtering, before sending the signal to the DACs.

The architecture consists of a central ESP32-S3 that processes the full-range signal and distributes it to three additional ESP32-S3 modules, each applying FIR filtering (crossover) for low, mid, and high frequencies.

## Features
	
* SPDIF input via DIR9001, or other i2s source
* DSP Processing:
* IIR Equalization (Main ESP32-S3, U1)
* FIR Crossover Filters (U2, U3, U4 - Low, Mid, High)
* Multi-ESP Architecture
* I2S signal buffering with TXS0108E or other suitable buffer.
* Separate Power Supplies for DACs and ESP32-S3 to reduce noise
* Configurable via menuconfig

## System Overview

### ESP32-S3 Assignments

* Device	Function	Processing
* U1	Main Controller	SPDIF or USB (i2s) Input, IIR EQ, Distributes Audio
* U2	Low Frequencies	FIR Low-Pass Filter
* U3	Mid Frequencies	FIR Band-Pass Filter
* U4	High Frequencies	FIR High-Pass Filter

Software Configuration (menuconfig)

### Select Device Role (U1, U2, U3, U4)
*	U1 (Main ESP32-S3) – Receives PCM data, applies IIR EQ, and distributes the signal
*	U2 (Low Frequencies) – Receives processed signal and applies FIR Low-Pass
*	U3 (Mid Frequencies) – Receives processed signal and applies FIR Band-Pass
*	U4 (High Frequencies) – Receives processed signal and applies FIR High-Pass

## DSP Processing Options
*	✅ Invert Phase
*	✅ Enable Encoder for Volume Control
*	✅ Select DSP Filters:
*	 FIR Only
*	 IIR Only
*	 FIR + IIR
*	⚠️ Passthrough Mode (No DSP Processing) – For Main device Only U1!
*	Must only be used on U1 (Main ESP32-S3)
*	Dangerous on U2, U3, or U4! – Can send incorrect frequencies to the wrong drivers

3️⃣ Save & Compile

### Hardware Details

* ESP32-S3 – Main and three processing units
* DIR9001 – SPDIF Receiver
* ProtoDAC x8 (TDA1387) – Three units for audio output
* TXS0108E – I2S Signal Buffer
* Separate Power Supplies – Reduces noise and interference

### Installation & Compilation

### Clone the Project
Clone the project into the esp-idf examples directory
```
git clone https://github.com/Kristian8606/esp32s3_audio_dsp.git
cd esp32s3_audio_dsp
git submodule add https://github.com/espressif/esp-dsp.git components/esp-dsp
git submodule update --init --recursive
idf.py set-target esp32s3

```
### Configure the Project
```
idf.py menuconfig
```
*	Select the correct Device Role (U1/U2/U3/U4)
*	Configure DSP options (FIR/IIR/Passthrough Mode)

### Compile & Flash
```
idf.py flash monitor
```
## Future Improvements

* Test different FIR and IIR filter configurations
* Optimize power supply and grounding
* Test longer I2S lines and alternative buffer chips

## Important Notes
*	Passthrough Mode should ONLY be used on U1 (Main ESP32-S3)
*	U2/U3/U4 should always apply FIR filtering to avoid incorrect frequency playback
*	Ensure proper grounding and separate power supplies for best audio quality

* Passthrough Mode is only used on the main ESP (U1), while the other ESP modules continue filtering, preventing incorrect frequency ranges from reaching the wrong speakers.
* Disclaimer: This project is in beta version and is provided 'as is'. Its use is at your own risk, and the authors are not responsible for any damages or issues arising from the use of the project. Users must ensure that all components are properly configured and that the necessary coefficients for the FIR filters are generated correctly for the corresponding speaker.
