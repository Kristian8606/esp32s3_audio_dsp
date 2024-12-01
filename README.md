# ESP32 DSP
![6884FAFA-050E-4CCE-93F6-6EF75651843B](https://github.com/user-attachments/assets/f2c9d4b0-3d23-4239-8961-3bb2ab004d40)

ESP32_DSP

Audio Processing with DIR9001 and TDA1387

Project Overview

ESP32_DSP is a project that uses the ESP32 as a Digital Signal Processor (DSP) for real-time audio processing. The system takes digital audio input from a DIR9001, processes it on the ESP32 using various filters (e.g., low-pass, high-pass, band-pass, notch, peak, low shelf, and high shelf filters), and sends the processed data to a TDA1387, which outputs an analog audio signal.

System Architecture

	•	DIR9001:
A digital audio receiver that converts S/PDIF input into I2S output. It serves as the audio source and clock synchronizer.
	•	ESP32:
The core DSP processor that applies various digital filters to the audio signal and outputs it as I2S.
	•	TDA1387:
A high-quality Digital-to-Analog Converter (DAC) that transforms the processed digital signal into an analog audio output.

Features

	•	Real-Time DSP:
Real-time audio signal processing on the ESP32.
	•	Filter Options:
The system supports multiple filter types, including:
	•	Low-Pass Filter: Attenuates frequencies above the cutoff frequency.
	•	High-Pass Filter: Attenuates frequencies below the cutoff frequency.
	•	Band-Pass Filter: Passes frequencies within a specific range.
	•	Notch Filter: Removes a narrow frequency band.
	•	Peak Filter: Boosts or attenuates a specific frequency with adjustable Q-factor.
	•	Low Shelf Filter: Boosts or cuts frequencies below a set threshold.
	•	High Shelf Filter: Boosts or cuts frequencies above a set threshold.
	•	Configurable Parameters:
Filters can be customized with the following parameters:
	•	Center or cutoff frequency
	•	Gain (boost or cut)
	•	Q-factor (bandwidth control)
	•	Modular and Scalable:
New DSP functions and filters can be easily integrated into the system.

How It Works

	1.	Audio Input:
The DIR9001 receives a digital audio signal (S/PDIF) and converts it to I2S.
	2.	DSP Processing:
The ESP32 processes the audio using the configured filters and applies real-time DSP algorithms.
	3.	Audio Output:
The TDA1387 converts the processed I2S audio data into an analog signal, ready for playback through headphones or speakers.

Hardware Components

	•	ESP32 microcontroller
	•	DIR9001 Digital Audio Receiver
	•	TDA1387 DAC
	•	Audio Input: S/PDIF source
	•	Audio Output: Amplifier or headphones

Hardware Connections

	•	DIR9001 I2S Output → ESP32 I2S Input
	•	ESP32 I2S Output → TDA1387 I2S Input
	•	TDA1387 Analog Output → Amplifier or Headphones

Setup Instructions

	1.	Connect Hardware
Wire all components according to the Hardware Connections section.
	2.	Install Software
	•	Install ESP-IDF or Arduino IDE.
	•	Clone the repository:

git clone [https://github.com/your-repo/ESP32_DSP](https://github.com/Kristian8606/ESP32_DSP).git
cd ESP32_DSP


	•	Build and flash the ESP32 firmware:

idf.py build flash


	3.	Configure Filters
Edit the filter settings in filter_config.h to customize the DSP behavior:

#define FILTER_TYPE PEAK       // Options: LOW_PASS, HIGH_PASS, BAND_PASS, NOTCH, PEAK, LOW_SHELF, HIGH_SHELF
#define FREQUENCY 1000         // Hz
#define GAIN 6                 // dB (for PEAK, LOW_SHELF, HIGH_SHELF)
#define Q_FACTOR 1.0           // Quality factor

Applications

	•	Audio equalization and tuning
	•	Sound enhancement and filtering
	•	Real-time experimentation with DSP algorithms

Future Enhancements

	•	Add support for multiple simultaneous filters.
	•	Implement audio effects like reverb or delay.
	•	Create a Bluetooth or Wi-Fi interface for real-time parameter adjustments.
	•	Extend functionality to support multi-channel audio.

License

This project is licensed under the MIT License. See the LICENSE file for more details.
