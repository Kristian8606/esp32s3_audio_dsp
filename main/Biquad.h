#include <math.h>
#include <stdio.h>
#include "webserver.h"

#define SAMPLE_RATE (44100)

#define M_PI 3.14159265358979323846

int length;
/*
Filter Settings file

Room EQ V5.31.3
Dated: Jan 8, 2025 11:32:39 AM

Notes:92.7

Equaliser: miniDSP 2x4 HD
RMS Average bassMax
Filter  1: ON  PK       Fc   45.80 Hz  Gain   3.00 dB  Q  1.000
Filter  2: ON  PK       Fc   48.75 Hz  Gain  -5.50 dB  Q 22.248
Filter  3: ON  PK       Fc   58.10 Hz  Gain  -6.70 dB  Q 10.674
Filter  4: ON  PK       Fc   70.80 Hz  Gain -10.80 dB  Q  6.692
Filter  5: ON  PK       Fc   85.40 Hz  Gain  -9.30 dB  Q  5.138
Filter  6: ON  PK       Fc   136.0 Hz  Gain  -6.10 dB  Q 10.723
Filter  7: ON  PK       Fc   160.0 Hz  Gain  -6.10 dB  Q 11.092
Filter  8: ON  PK       Fc   483.0 Hz  Gain  -3.10 dB  Q  3.998
Filter  9: ON  PK       Fc    3155 Hz  Gain   3.00 dB  Q  1.812
Filter 10: ON  PK       Fc    8368 Hz  Gain  -3.50 dB  Q  1.000
Filter 11: ON  PK       Fc    38.8 Hz  Gain   3.00 dB  Q  5.00

*/
enum {
    LP = 0,
    HP,
    BP,
    Notch,
    PK,
    LS,
    HS
};

 /* In this array you need to specify the type of filter. PK , LP , HP and so on. In this case  PK - (PEAK FILTERS ) are set.*/
 /*
 type_filters[] = { PK
                      ,PK
                      ,PK
                      ,PK
                      ,PK
                      ,PK
                      ,PK
                      ,PK
                      ,PK
                      ,PK
                      ,PK
                      
      
};
 Hz[] = { 45.80
              , 48.75
              , 58.10
              , 70.80
              , 85.40
              , 136.0
              , 160.0
              , 483.0
              ,  3155
              ,  8368
              , 38.8
          
};

 dB[] = {   3.00
              ,  -5.50
              ,  -6.70
              , -10.80
              ,  -9.30
              ,  -6.10
              ,  -6.10
              ,  -3.10
              ,   3.00
              ,  -3.50
              ,   3.00
        
};

 Qd[] = {  1.000
              , 22.248
              , 10.674
              ,  6.692
              ,  5.138
              , 10.723
              , 11.092
              ,  3.998
              ,  1.812
              ,  1.000
              ,  5.0
            
};
*/

  struct iir_filt {
	float in_z1_st;
    float in_z2_st;
    float out_z1_st;
    float out_z2_st;
    float in_z1;
    float in_z2;
    float out_z1;
    float out_z2;
    float a0;
    float a1;
    float a2;
    float b1;
    float b2;
    char* type;
};

void bq_print_info(struct iir_filt* bq){
	printf("A0: %13.16f\n",bq->a0);
	printf("A1: %13.16f\n",bq->a1);
	printf("A2: %13.16f\n",bq->a2);
	printf("B1: %13.16f\n",bq->b1);
	printf("B2: %13.16f\n",bq->b2);
	printf("TYPE: %s\n",bq->type);
	printf("\n");
}



 //length = filter_count;//(int)(sizeof(Hz) / sizeof(Hz[0]));

struct iir_filt *iir_coeff;

 static void calcBiquad(int type, double Fc, double peakGain,  double Q, struct iir_filt* config) {

  //   double a0 = 1.0;
   //  double a1 = 0.0, a2 = 0.0, b1 = 0.0, b2 = 0.0;
     
		 config->in_z1 = 0.0;
		 config->in_z2 = 0.0;
		 config->out_z1 = 0.0;
		 config->out_z2 = 0.0;
		 
		 config->in_z1_st = 0.0;
		 config->in_z2_st = 0.0;
		 config->out_z1_st = 0.0;
		 config->out_z2_st = 0.0;



     double norm;
     double V = pow(10, fabs(peakGain) / 20.0);
     double K = tan(M_PI * Fc);
     switch (type) {
     case LP:
         norm = 1 / (1 + K / Q + K * K);
         config->a0 = K * K * norm;
         config->a1 = 2 * config->a0;
         config->a2 = config->a0;
         config->b1 = 2 * (K * K - 1) * norm;
         config->b2 = (1 - K / Q + K * K) * norm;
         config->type = "LOWPASS";
         break;

     case HP:
         norm = 1 / (1 + K / Q + K * K);
        config->a0 = 1 * norm;
        config->a1 = -2 * config->a0;
        config->a2 = config->a0;
        config->b1 = 2 * (K * K - 1) * norm;
        config->b2 = (1 - K / Q + K * K) * norm;
         config->type = "HIGHPASS";
         break;

     case BP:
         norm = 1 / (1 + K / Q + K * K);
        config->a0 = K / Q * norm;
        config->a1 = 0;
        config->a2 = - config->a0;
        config->b1 = 2 * (K * K - 1) * norm;
        config->b2 = (1 - K / Q + K * K) * norm;
         config->type = "BANDPASS";
         break;

     case Notch:
         norm = 1 / (1 + K / Q + K * K);
         config->a0 = (1 + K * K) * norm;
         config->a1 = 2 * (K * K - 1) * norm;
         config->a2 = config->a0;
         config->b1 = config->a1;
         config->b2 = (1 - K / Q + K * K) * norm;
         config->type = "NOTCH";
         break;

     case PK:
         if (peakGain >= 0) {    // boost
             norm = 1 / (1 + 1 / Q * K + K * K);
             config->a0 = (1 + V / Q * K + K * K) * norm;
             config->a1 = 2 * (K * K - 1) * norm;
             config->a2 = (1 - V / Q * K + K * K) * norm;
             config->b1 = config->a1;
             config->b2 = (1 - 1 / Q * K + K * K) * norm;
         }
         else {    // cut
             norm = 1 / (1 + V / Q * K + K * K);
             config->a0 = (1 + 1 / Q * K + K * K) * norm;
             config->a1 = 2 * (K * K - 1) * norm;
             config->a2 = (1 - 1 / Q * K + K * K) * norm;
             config->b1 = config->a1;
             config->b2 = (1 - V / Q * K + K * K) * norm;
         }
         config->type = "PEAK";
         break;
     case LS:
         if (peakGain >= 0) {    // boost
             norm = 1 / (1 + sqrt(2) * K + K * K);
            config->a0 = (1 + sqrt(2 * V) * K + V * K * K) * norm;
            config->a1 = 2 * (V * K * K - 1) * norm;
            config->a2 = (1 - sqrt(2 * V) * K + V * K * K) * norm;
            config->b1 = 2 * (K * K - 1) * norm;
            config->b2 = (1 - sqrt(2) * K + K * K) * norm;
         }
         else {    // cut
             norm = 1 / (1 + sqrt(2 * V) * K + V * K * K);
            config->a0 = (1 + sqrt(2) * K + K * K) * norm;
            config->a1 = 2 * (K * K - 1) * norm;
            config->a2 = (1 - sqrt(2) * K + K * K) * norm;
            config->b1 = 2 * (V * K * K - 1) * norm;
            config->b2 = (1 - sqrt(2 * V) * K + V * K * K) * norm;
         }
         config->type = "LOWSHELF";
         break;
     case HS:
         if (peakGain >= 0) {    // boost
             norm = 1 / (1 + sqrt(2) * K + K * K);
             config->a0 = (V + sqrt(2 * V) * K + K * K) * norm;
             config->a1 = 2 * (K * K - V) * norm;
             config->a2 = (V - sqrt(2 * V) * K + K * K) * norm;
             config->b1 = 2 * (K * K - 1) * norm;
             config->b2 = (1 - sqrt(2) * K + K * K) * norm;
         }
         else {    // cut
             norm = 1 / (V + sqrt(2 * V) * K + K * K);
            config->a0 = (1 + sqrt(2) * K + K * K) * norm;
            config->a1 = 2 * (K * K - 1) * norm;
            config->a2 = (1 - sqrt(2) * K + K * K) * norm;
            config->b1 = 2 * (K * K - V) * norm;
            config->b2 = (V - sqrt(2 * V) * K + K * K) * norm;
         }
         config->type = "HIGHSHELF";
         break;
     }
 }


 static float process_iir_ch_1 (float inSampleF, struct iir_filt * config) {
         
		 
	  float outSampleF =
	config->a0 * inSampleF
	+ config->a1 * config->in_z1
	+ config->a2 * config->in_z2
	- config->b1 * config->out_z1
	- config->b2 * config->out_z2;
	config->in_z2 = config->in_z1;
	config->in_z1 = inSampleF;
	config->out_z2 = config->out_z1;
	config->out_z1 = outSampleF;
	return outSampleF;
}
 static float process_iir_ch_2 (float inSampleF, struct iir_filt * config) {
 		
	  float outSampleF =
	config->a0 * inSampleF
	+ config->a1 * config->in_z1_st
	+ config->a2 * config->in_z2_st
	- config->b1 * config->out_z1_st
	- config->b2 * config->out_z2_st;
	config->in_z2_st = config->in_z1_st;
	config->in_z1_st = inSampleF;
	config->out_z2_st = config->out_z1_st;
	config->out_z1_st = outSampleF;
	return outSampleF;
}

 static void create_biquad() {
 length = filter_count;
 iir_coeff = (struct iir_filt *)malloc(length * sizeof(struct iir_filt));
      
     for (int i = 0; i < length; i++){
       //  printf("type iir filter %d  %fHz, dB %f, Q %f\n", type_filters[i], Hz[i], dB[i], Qd[i]);
         calcBiquad(type_filters[i], Hz[i]/SAMPLE_RATE, dB[i], Qd[i], &iir_coeff[i]);
         bq_print_info(&iir_coeff[i]);
     }
   
 }
 

static void process_data_stereo(int32_t *data, size_t num_samples, float volume) {
    // Параметърите са:
    // `data` - указател към входните/изходните данни (32-битови цели числа)
    // `num_samples` - броят на пробите (стерео пробите се броят поотделно, т.е. ляв и десен канал са двойки)
    int32_t *input = data;    // Указател към входните данни
    int32_t *output = data;   // Указател към изходните данни (същият като входния)

    for (size_t i = 0; i < num_samples; i += 2) {
        // Вземане на текущата проба за левия и десния канал
        float sample_left = (float)(*input++);
        float sample_right = (float)(*input++);
        // Обработка на левия канал с IIR филтър
        float processed_left = process_iir_ch_1(sample_left, &iir_coeff[0]);
        	for(int z = 1; z < length ; z++){
        		processed_left = process_iir_ch_1(processed_left, &iir_coeff[z]);
        	}
			  			  
        // Обработка на десния канал с IIR филтър
        float processed_right = process_iir_ch_2(sample_right, &iir_coeff[0]);
        for(int z = 1; z < length ; z++){
        		 processed_right = process_iir_ch_2(processed_right, &iir_coeff[z]);
        	}
            
		processed_left *= volume;
    	processed_right *= volume;

	//	if (processed_left > INT32_MAX) processed_left = INT32_MAX;
	//	if (processed_left < INT32_MIN) processed_left = INT32_MIN;
        // Запис на обработените стойности обратно в изхода
        *output++ = (int32_t)(processed_left);
        *output++ = (int32_t)(processed_right);
    }
}
