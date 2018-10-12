/*
 * tlumik_128.h
 *
 *  Created on: 28.12.2017
 *      Author: witek
 */

#ifndef TLUMIK_128_H_
#define TLUMIK_128_H_

//#define DEBUG

#define ENC_A A4
#define ENC_B A5

#define T_1dB 9
#define T_2dB 10
#define T_4dB 12
#define T_8dB A2
#define T_16dB A3
#define T_32dB 8
#define T_64dB 11

#define SW_1 A0
#define KROK_1dB 1
#define KROK_10dB 10
#define COLDSTART_REF      0x12   // When started, the firmware examines this "Serial Number
                                  // and enforces factory reset to clear all
                                  // settings, as well as frequency and position memories.
                                  // To roll this value is useful if there is chance of a
                                  // mismatch due to restructuring of the EEPROM.
                                  // Else - the stepper may take on on epic journey... :)
                                  // COLDSTART_REF can be any unique number between
                                  // 0x01 and 0xfd.  0xfe is reserved for use by the firmware
                                  // to clear frequency/position memory data while retaining
                                  // controller_settings.
#define TLUMIENIE_ADRES	1
#define CZAS_REAKCJI 500		// czas [ms] po jakim następuje zmiana tłumienia i zapis do EEPROM

void set_tlumienie(int8_t enc);
void set_wyjscia();
void show_tlumienie();

#endif /* TLUMIK_128_H_ */
