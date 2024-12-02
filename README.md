
# ESP32 DSP
![6884FAFA-050E-4CCE-93F6-6EF75651843B](https://github.com/user-attachments/assets/f2c9d4b0-3d23-4239-8961-3bb2ab004d40)

## ESP32_DSP

[Configure Filters](https://github.com/Kristian8606/ESP32_DSP/blob/main/README.md#configure-filters)
[Screenshots](https://github.com/Kristian8606/ESP32_DSP/blob/main/README.md#screenshots)

## Audio Processing with DIR9001 and TDA1387

Project Overview

ESP32_DSP is a project that uses the ESP32 as a Digital Signal Processor (DSP) for real-time audio processing. The system takes digital audio input from a DIR9001, processes it on the ESP32 using various filters (e.g., low-pass, high-pass, band-pass, notch, peak, low shelf, and high shelf filters), and sends the processed data to a TDA1387, which outputs an analog audio signal.

## System Architecture

	DIR9001:
A digital audio receiver that converts S/PDIF input into I2S output. It serves as the audio source and clock synchronizer.

    ESP32:

The core DSP processor that applies various digital filters to the audio signal and outputs it as I2S.

    TDA1387x8:

A high-quality Digital-to-Analog Converter (DAC) that transforms the processed digital signal into an analog audio output.

## Features

	Real-Time DSP:
Real-time audio signal processing on the ESP32.
* Filter Options:
The system supports multiple filter types, including:

*	Low-Pass Filter: Attenuates frequencies above the cutoff frequency.
*	High-Pass Filter: Attenuates frequencies below the cutoff frequency.
*	Band-Pass Filter: Passes frequencies within a specific range.
*	Notch Filter: Removes a narrow frequency band.
*	Peak Filter: Boosts or attenuates a specific frequency with adjustable Q-factor.
*	Low Shelf Filter: Boosts or cuts frequencies below a set threshold.
*	High Shelf Filter: Boosts or cuts frequencies above a set threshold.
## Configurable Parameters:
Filters can be customized with the following parameters:
*	Center or cutoff frequency
*	Gain (boost or cut)
*	Q-factor (bandwidth control)
	
## How It Works?

	1.	Audio Input:
The DIR9001 receives a digital audio signal (S/PDIF) and converts it to I2S.

    2.	DSP Processing:
The ESP32 processes the audio using the configured filters and applies real-time DSP algorithms.

	3.	Audio Output:
The TDA1387 converts the processed I2S audio data into an analog signal, ready for playback through headphones or speakers.

Hardware Components

*	ESP32 microcontroller
*	DIR9001 Digital Audio Receiver
*	TDA1387 DAC
*	Audio Input: S/PDIF source
*	Audio Output: Amplifier or headphones

## Hardware Connections

*	DIR9001 I2S Output → ESP32 I2S Input
*	ESP32 I2S Output → TDA1387 I2S Input
*	TDA1387 Analog Output → Amplifier or Headphones

//TODO add schematics.

Setup Instructions

    1.	Connect Hardware

Wire all components according to the Hardware Connections section.

    2.	Install Software

*	Install ESP-IDF https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html#esp-idf-programming-guide
*	Clone the repository in the i2s examples directory of the ESP-IDF:

*	Build and flash the ESP32 firmware:
```
git clone https://github.com/Kristian8606/ESP32_DSP.git
cd ESP32_DSP
idf.py build flash
```  
	
## Configure Filters
	
Edit the filter settings in ```Biquad.h``` to customize the DSP behavior:

In this array you need to specify the type of filter. PK , LP , HP and so on. In this case 6 filters PK - (PEAK FILTERS ) are set.
```C++
int type_filters[] = { PK      
                      ,PK
                      ,PK
                      ,PK
                      ,PK
                      ,PK
          
};
```
Here you set the filter frequency.
```C++
double Hz[] = { 72.50
              , 120.0
              , 224.0
              , 352.0  
              , 1279.0
              , 38.0
         

};
```
Here we set the gain in decibels.
```C++
double dB[] = { -4.80
              , -4.10
              , -2.80
              , -4.60
              , -3.30
              ,  10.0
            
};
```
And finally the Q of the filter.
```C++
double Qd[] = { 5.000
              , 5.000
              , 5.000
              , 3.066
              , 1.000
              , 7.0
              
};

```


## Future Enhancements

*	Implement audio effects like reverb or delay.
*	Create a Bluetooth or Wi-Fi interface for real-time parameter adjustments.
*	Extended functionality to support multi-channel audio via additional ESP32s for tri-amping.
  
## Screenshots
<img width="1417" alt="Screenshot 2024-11-28 at 16 16 48" src="https://github.com/user-attachments/assets/ba402a28-14ff-4e4b-aa78-98278b97d198">
<img width="1417" alt="Screenshot 2024-11-28 at 16 18 41" src="https://github.com/user-attachments/assets/08b96763-12d4-44a9-9ee3-b027bddd6cd2">
<img width="1321" alt="Screenshot 2024-11-28 at 16 35 36" src="https://github.com/user-attachments/assets/47f28925-d872-4cef-95e0-a613939db80c">
<img width="1321" alt="Screenshot 2024-11-28 at 16 34 42" src="https://github.com/user-attachments/assets/b4fa74fa-b4d4-46bf-ac98-1a26cf41fadc">
<img width="1321" alt="Screenshot 2024-11-28 at 16 32 21" src="https://github.com/user-attachments/assets/0985f260-83aa-4a5e-b48e-248b28e63498">
<img width="1321" alt="Screenshot 2024-11-28 at 16 27 04" src="https://github.com/user-attachments/assets/c76c76ec-a36e-461f-a4e6-c05a1bab222a">
<img width="1143" alt="Screenshot 2024-11-28 at 16 24 18" src="https://github.com/user-attachments/assets/4557e177-66c2-4c8f-add0-feab63a3eb60">
<img width="1143" alt="Screenshot 2024-11-28 at 16 24 00" src="https://github.com/user-attachments/assets/9e6cfa27-88a2-4cf0-a335-112577ce6d07">
<img width="1417" alt="Screenshot 2024-11-28 at 16 18 57" src="https://github.com/user-attachments/assets/d46281c0-d5c8-4189-a48c-2340f884cd4d">
<img width="1321" alt="Screenshot 2024-11-28 at 16 37 57" src="https://github.com/user-attachments/assets/0e9f059d-7686-44f6-b666-0298600b704a">
<img width="1321" alt="Screenshot 2024-11-28 at 18 26 12" src="https://github.com/user-attachments/assets/90fd3f53-0efa-447a-a56e-7ccb9a71229c">
<img width="1321" alt="Screenshot 2024-11-28 at 18 26 30" src="https://github.com/user-attachments/assets/2f31f332-6469-462e-922c-4787cf521012">


## License

This project is licensed under the GPL-3.0 License. See the LICENSE file for more details.
