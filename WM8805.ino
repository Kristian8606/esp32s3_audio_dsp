/************************************************************************************

WM8805 S/PDIF Receiver
Project page: http://www.dimdim.gr/spdif-receiver-with-the-wm8805/

Based on: http://www.itsonlyaudio.com/audio-hardware/arduino-software-control-for-wm8804wm8805/

v0.82 : - Initial public release.
        - Only usable through serial port.
        - Outputs detected sampling rate.
        - Input switching only through editing the code (lines 326 - 334).
        - Constantly (every 100ms) checks the status register, so is essentially blocking.
          Could be made to check the register once every for example 500ms.

************************************************************************************/


#include <Wire.h>
int wm8805 = 58; // device wm8805, 58d = 0x3A = 0111010 (see datasheet pp. 17, table 11)
byte error, SR;
int SPDSTAT, INTSTAT;
int fs = 0;
bool toggle = 0;

void setup()
{
  Wire.begin();
  
  Serial.begin(115200);
  
  pinMode(2,OUTPUT);       // Connect to wm8805 reset pin. 
  digitalWrite(2, HIGH);   // Keep wm8805 reset pin high.
  
  delay(1000);  // for debugging, gives me time to fire up a serial terminal
  Wire.beginTransmission(wm8805);
  error = Wire.endTransmission();

  if (error == 0)
  {
    Serial.print("WM8805 found");
    Serial.println("  !");
  }
  else if (error==4) 
  {
    Serial.println("Unknown error at address 0x3A");
  }
  else
  {
    Serial.println("No response from WM8805!");
  }
  
  Serial.print("Device ID: ");
  byte c = ReadRegister(wm8805, 1);
  if (c < 10) {
    Serial.print('0');
  } 
  Serial.print(c,HEX);
  
  c = ReadRegister(wm8805, 0);
  if (c < 10) {
    Serial.print('0');
  } 
  Serial.print(c,HEX);
  
  Serial.print(" Rev. ");
  c = ReadRegister(wm8805, 2);
  Serial.println(c,HEX);

  DeviceInit(wm8805);
  
  delay(500);   
} // setup

void loop()                            // is quasi-interrupt routine
{
  INTSTAT = 0;
  while (INTSTAT == 0)
    {
      delay(100);
      INTSTAT = ReadRegister(wm8805, 11); // poll (and clear) interrupt register every 100ms.
    }
  
  // decode why interrupt was triggered
  // most of these shouldn't happen as they are masked off
  // but still useful for debugging
 
  if (bitRead(INTSTAT, 0)) {                                         // UPD_UNLOCK
    SPDSTAT = ReadRegister(wm8805, 12);
    Serial.print("UPD_UNLOCK: ");
    if (bitRead(SPDSTAT,6)) {
      delay(500);
 
      // try again to try to reject transient errors
      SPDSTAT = ReadRegister(wm8805, 12);
      if (bitRead(SPDSTAT,6)) {
      
        Serial.println("S/PDIF PLL unlocked");
        
        // switch PLL coeffs around to try to find stable setting
        
        if (toggle) {
          Serial.println("trying 192 kHz mode...");
          fs = 192;
          WriteRegister(wm8805, 6, 8);                  // set PLL_N to 8
          WriteRegister(wm8805, 5, 12);                 // set PLL_K to 0C49BA (0C)
          WriteRegister(wm8805, 4, 73);                 // set PLL_K to 0C49BA (49)
          WriteRegister(wm8805, 3, 186);                // set PLL_K to 0C49BA (BA)
          toggle = 0;
          delay(500);
        }
        else {
          //fs = 0;
          Serial.println("trying normal mode...");
          WriteRegister(wm8805, 6, 7);                  // set PLL_N to 7
          WriteRegister(wm8805, 5, 54);                 // set PLL_K to 36FD21 (36)
          WriteRegister(wm8805, 4, 253);                // set PLL_K to 36FD21 (FD)
          WriteRegister(wm8805, 3, 33);                 // set PLL_K to 36FD21 (21)
          toggle = 1;
          delay(500);
        } // if toggle


      } // if (bitRead(SPDSTAT,6))

    }
    else {
      Serial.println("S/PDIF PLL locked");
    }
    delay(50); // sort of debounce
  }
  
  if (bitRead(INTSTAT, 1)) {                                         // INT_INVALID
    Serial.println("INT_INVALID");
  }
  
  if (bitRead(INTSTAT, 2)) {                                         // INT_CSUD
    Serial.println("INT_CSUD");
  }
  
  if (bitRead(INTSTAT, 3)) {                                         // INT_TRANS_ERR
    Serial.println("INT_TRANS_ERR");
  }
  
  if (bitRead(INTSTAT, 4)) {                                         // UPD_NON_AUDIO
    Serial.println("UPD_NON_AUDIO");
  }
  
  if (bitRead(INTSTAT, 5)) {                                         // UPD_CPY_N
    Serial.println("UPD_CPY_N");
  }
  
  if (bitRead(INTSTAT, 6)) {                                         // UPD_DEEMPH
    Serial.println("UPD_DEEMPH");
  }
    
  if (bitRead(INTSTAT, 7)) {                                         // UPD_REC_FREQ
    
    /*
    // clear serial terminal (in case of using PuTTY)
    Serial.write(27);       // ESC command
    Serial.print("[2J");    // clear screen command
    Serial.write(27);
    Serial.print("[H");     // cursor to home command
    */
    SPDSTAT = ReadRegister(wm8805, 12);
    int samplerate = 2*bitRead(SPDSTAT,5) + bitRead(SPDSTAT,4);      // calculate indicated rate
    Serial.print("UPD_REC_FREQ: ");
    Serial.print("Sample rate: ");

    switch (samplerate) {
      case 3:
      Serial.println("32 kHz");
      fs = 32;
      WriteRegister(wm8805, 29, 0);                 // set SPD_192K_EN to 0
      delay(500);
      break;

      case 2:
      Serial.println("44 / 48 kHz");
      fs = 48;
      WriteRegister(wm8805, 29, 0);                 // set SPD_192K_EN to 0
      delay(500);
      break;

      case 1:
      Serial.println("88 / 96 kHz");
      fs = 96;
      WriteRegister(wm8805, 29, 0);                 // set SPD_192K_EN to 0
      delay(500);
      break;

      case 0:
      Serial.println("192 kHz");
      fs = 192;
      WriteRegister(wm8805, 29, 128);                 // set SPD_192K_EN to 1
      delay(500);
      break;
    } // switch samplerate
  
  } // if (bitRead(INTSTAT, 7))

    // Detect sampling rate by reading status register 16.
    SR = ReadRegister(wm8805, 16);
    Serial.print("Detected SR: ");
    switch (SR) {
      
      case B0001:
      Serial.println("not indicated");
      break;

      case B0011:
      Serial.println("32 kHz");
      break;

      case B1110:
      Serial.println("192 kHz");
      break;

      case B1010:
      Serial.println("96 kHz");
      break;

      case B0010:
      Serial.println("48 kHz");
      break;

      case B0110:
      Serial.println("24 kHz");
      break;
      
      case B1100:
      Serial.println("176.4 kHz");
      break;

      case B1000:
      Serial.println("88.2 kHz");
      break;

      case B0000:
      if (fs == 192) {
          Serial.println("192 kHz");
        }
        else
        Serial.println("44.1 kHz");
      break;

      case B0100:
      Serial.println("22.05 kHz");
      break;
    } // switch samplerate

} // loop



/****************************************************************************************/
// Functions go below main loop

byte ReadRegister(int devaddr, int regaddr){                              // Read a data register value
  Wire.beginTransmission(devaddr);
  Wire.write(regaddr);
  Wire.endTransmission(false);                                            // repeated start condition: don't send stop condition, keeping connection alive.
  Wire.requestFrom(devaddr, 1); // only one byte
  byte data = Wire.read();
  Wire.endTransmission(true);
  return data;
}

void WriteRegister(int devaddr, int regaddr, int dataval){                // Write a data register value
  Wire.beginTransmission(devaddr); // device
  Wire.write(regaddr); // register
  Wire.write(dataval); // data
  Wire.endTransmission(true);
}

void DeviceInit(int devaddr){                                              // resets, initializes and powers a wm8805
  // reset device
  WriteRegister(devaddr, 0, 0);

  // REGISTER 7
  // bit 7:6 - always 0
  // bit 5:4 - CLKOUT divider select => 00 = 512 fs, 01 = 256 fs, 10 = 128 fs, 11 = 64 fs
  // bit 3 - MCLKDIV select => 0
  // bit 2 - FRACEN => 1
  // bit 1:0 - FREQMODE (is written by S/PDIF receiver) => 00
  WriteRegister(devaddr, 7, B00000100);

  // REGISTER 8
  // set clock outputs and turn off last data hold
  // bit 7 - MCLK output source select is CLK2 => 0
  // bit 6 - always valid => 0
  // bit 5 - fill mode select => 1 (we need to see errors when they happen)
  // bit 4 - CLKOUT pin disable => 1
  // bit 3 - CLKOUT pin select is CLK1 => 0
  // bit 2:0 - always 0
  WriteRegister(devaddr, 8, B00110000);

  // set masking for interrupts
  WriteRegister(devaddr, 10, 126);  // 1+2+3+4+5+6 => 0111 1110. We only care about unlock and rec_freq

  // set the AIF TX
  // bit 7:6 - always 0
  // bit   5 - LRCLK polarity => 0
  // bit   4 - BCLK invert => 0
  // bit 3:2 - data word length => 10 (24b) or 00 (16b)
  // bit 1:0 - format select: 11 (dsp), 10 (i2s), 01 (LJ), 00 (RJ)
  WriteRegister(devaddr, 27, B00001010);

  // set the AIF RX
  // bit   7 - SYNC => 1
  // bit   6 - master mode => 1
  // bit   5 - LRCLK polarity => 0
  // bit   4 - BCLK invert => 0
  // bit 3:2 - data word length => 10 (24b) or 00 (16b)
  // bit 1:0 - format select: 11 (dsp), 10 (i2s), 01 (LJ), 00 (RJ)
  WriteRegister(devaddr, 28, B11001010);

  // set PLL K and N factors
  // this should be sample rate dependent, but makes hardly any difference
  WriteRegister(devaddr, 6, 7);                  // set PLL_N to 7
  WriteRegister(devaddr, 5, 0x36);                 // set PLL_K to 36FD21 (36)
  WriteRegister(devaddr, 4, 0xFD);                 // set PLL_K to 36FD21 (FD)
  WriteRegister(devaddr, 3, 0x21);                 // set PLL_K to 36FD21 (21)

  // set all inputs for TTL
  WriteRegister(devaddr, 9, 0);
  
  // power up device
  WriteRegister(devaddr, 30, 0);

  // select input
  // bit   7 - MCLK Output Source Select => 0 (CLK2)
  // bit   6 - Always Valid Select => 0
  // bit   5 - Fill Mode Select => 0
  // bit   4 - CLKOUT Pin Disable => 1
  // bit   3 - CLKOUT Pin Source Select => 1)
  // bit 2:0 - S/PDIF Rx Input Select: 000 – RX0, 001 – RX1, 010 – RX2, 011 – RX3, 100 – RX4, 101 – RX5, 110 – RX6, 111 – RX7
  //WriteRegister(devaddr, 8, B00011000);           // Select Input 1 (Coax)
  WriteRegister(devaddr, 8, B00011011);           // Select Input 4 (TOSLINK)
}
