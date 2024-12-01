# ESP32_DSP
![6884FAFA-050E-4CCE-93F6-6EF75651843B](https://github.com/user-attachments/assets/f2c9d4b0-3d23-4239-8961-3bb2ab004d40)

ESP32 DSP 

Audio Processing with DIR9001 and TDA1387

Project Overview

This project demonstrates the use of the ESP32 as a Digital Signal Processor (DSP) for real-time audio processing. The system receives a digital audio signal from the DIR9001, processes it on the ESP32 using various filter types, including low-pass, high-pass, band-pass, notch, peak, low shelf, and high shelf filters, and outputs the processed data to the TDA1387, which converts it to an analog signal.

System Architecture

# DIR9001:
A digital audio receiver that converts S/PDIF input into I2S output. It serves as the audio source and clock synchronizer.
	•	ESP32:
The core DSP processor that receives the I2S signal from the DIR9001, applies a variety of digital filters, and sends the processed audio back as I2S.
	•	TDA1387:
A high-quality Digital-to-Analog Converter (DAC) that transforms the processed digital audio into an analog signal for playback.

Features

	•	Real-Time DSP:
Processes audio signals in real-time on the ESP32.
	•	Filter Options:
The system includes multiple configurable filter types:
	•	Low-Pass Filter: Attenuates frequencies above the cutoff.
	•	High-Pass Filter: Attenuates frequencies below the cutoff.
	•	Band-Pass Filter: Passes frequencies within a specific range.
	•	Notch Filter: Removes a narrow frequency band.
	•	Peak Filter: Boosts or attenuates specific frequencies with adjustable Q-factor.
	•	Low Shelf Filter: Boosts or cuts frequencies below the shelf frequency.
	•	High Shelf Filter: Boosts or cuts frequencies above the shelf frequency.
	•	Customizable Parameters:
Easily adjust filter parameters, including:
	•	Frequency cutoff or center frequency.
	•	Gain (boost or cut).
	•	Q-factor (bandwidth).
	•	Modular Design:
The architecture allows for easy addition of new filters and audio processing algorithms.

How the System Works

	1.	Audio Input:
The DIR9001 receives a digital audio signal (S/PDIF) and converts it to I2S format.
	2.	Signal Processing:
The ESP32 applies selected DSP algorithms, including the desired filter type (e.g., low-pass or peak filter).
	3.	Audio Output:
The TDA1387 converts the processed digital audio into an analog signal, ready for playback through an amplifier or headphones.

Components

	•	ESP32 microcontroller
	•	DIR9001 Digital Audio Receiver
	•	TDA1387 DAC
	•	Audio Input: S/PDIF source
	•	Audio Output: Amplifier or headphones

Hardware Connections

	•	DIR9001 I2S output → ESP32 I2S input
	•	ESP32 I2S output → TDA1387 I2S input
	•	TDA1387 analog output → Amplifier or headphones

Setup

	1.	Connect the Hardware
Ensure all components are connected as described in the Hardware Connections section.
	2.	Install the Software
	•	Install ESP-IDF or Arduino IDE.
	•	Clone the project:

git clone https://github.com/your-repo/esp32-dsp-project.git
cd esp32-dsp-project


	•	Compile and flash the ESP32 firmware:

idf.py build flash


	3.	Configure Filters
Select and configure the desired filter type and parameters in filter_config.h:

#define FILTER_TYPE PEAK       // Options: LOW_PASS, HIGH_PASS, BAND_PASS, NOTCH, PEAK, LOW_SHELF, HIGH_SHELF
#define FREQUENCY 1000         // Hz
#define GAIN 6                 // dB (for PEAK, LOW_SHELF, HIGH_SHELF)
#define Q_FACTOR 1.0           // Quality factor

Applications

	•	Audio equalization
	•	Advanced sound design and filtering
	•	Real-time audio enhancement
	•	Experimentation with DSP algorithms

Future Improvements

	•	Add more advanced filter types and processing techniques.
	•	Implement GUI controls for real-time tuning via Bluetooth or Wi-Fi.
	•	Extend support for multi-channel audio.

License

This project is licensed under the MIT License. See the LICENSE file for details.

This version provides detailed descriptions of all filter types included in your project and keeps the structure clear and concise. Let me know if you’d like any further adjustments!
