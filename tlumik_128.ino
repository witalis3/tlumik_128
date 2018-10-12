/*
 * Sterowanie tłumikiem 0-127 dB
 * autor: Witold Buszkiewicz
 * SP3JDZ
 * ***************************
 * TODO
 * - krok 1dB i 10dB - przycisk SW1
 * - pamiętanie ostatnio nastawionego tłumienia
 * 	- po upływie 1s
 * - zmiana na wyjściu dopiero, gdy brak zmian w okresie 200ms (?) - żeby nie klepały przekaźniki w czasie kręcenia enkoderem
 *
 */
#include "Arduino.h"
#include <EEPROM.h>
#include "tlumik_128.h"
#include <TimerOne.h>
// wyświetlacz
#include <LiquidCrystal.h>
#include "Bounce2.h"

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 2, en = 3, d4 = 5, d5 = 4, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

Bounce krok = Bounce();
byte tlumienie = 127;		// tłumienie tłumika w dB
byte aktualny_krok = KROK_1dB;
boolean byla_zmiana = false;
unsigned long czas_zmiany;
int8_t enc_delta;							// -128 ... 127
void encode_read()
{
	static int8_t last;
	int8_t nowy;
	int8_t diff;
	nowy = 0;
	if (digitalRead(ENC_A) == LOW)
		nowy = 3;
	if (digitalRead(ENC_B) == LOW)
		nowy ^= 1;								// convert gray to binary
	diff = last - nowy;						// difference last - nowy
	if (diff & 1)
	{							// bit 0 = value (1)
		last = nowy;							// store nowy as next last
		enc_delta += (diff & 2) - 1;		// bit 1 = direction (+/-)
	}
}
int8_t encode_read1(void)					// read single step encoders
{
	int8_t val;
	noInterrupts();
	val = enc_delta;
	enc_delta = 0;
	interrupts();
	return val;								// counts since last call
}
int8_t encode_read2(void)					// read two step encoders
{
	int8_t val;
	noInterrupts();
	val = enc_delta;
	enc_delta &= 1;
	interrupts();
	return val >> 1;
}
int8_t encode_read4(void)// read four step encoders; funkcja dla enkodera kwadraturowego
{
	int8_t val;
	noInterrupts();
	val = enc_delta;
	enc_delta &= 3;
	interrupts();
	return val >> 2;
}
void set_tlumienie(int8_t enc)
{
	if (tlumienie + enc*aktualny_krok < 0)
	{
		tlumienie = 0;
	}
	else
	{
		tlumienie = tlumienie + enc*aktualny_krok;
		tlumienie = constrain(tlumienie, 0, 127);
	}
#if defined(DEBUG)
	Serial.print("tlumienie: ");
	Serial.println(tlumienie);
#endif
}
void set_wyjscia()
{
	digitalWrite(T_1dB, bitRead(tlumienie, 0));
	digitalWrite(T_2dB, bitRead(tlumienie, 1));
	digitalWrite(T_4dB, bitRead(tlumienie, 2));
	digitalWrite(T_8dB, bitRead(tlumienie, 3));
	digitalWrite(T_16dB, bitRead(tlumienie, 4));
	digitalWrite(T_32dB, bitRead(tlumienie, 5));
	digitalWrite(T_64dB, bitRead(tlumienie, 6));
}
void show_tlumienie()
{
	char bufor[8];
	sprintf(bufor, "%3d dB", tlumienie);
	lcd.setCursor(8, 1);
	lcd.print(bufor);
}
void show_krok()
{
	char bufor[7];
	sprintf(bufor, "Krok %2d", aktualny_krok);
	lcd.setCursor(0, 1);
	lcd.print(bufor);
}
void setup()
{
#if defined(DEBUG)
	Serial.begin(1200);
	Serial.println("tlumik startuje...");
#endif
	byte coldstart;
	  coldstart = EEPROM.read(0);              // Grab the coldstart byte indicator in EEPROM for
	                                           // comparison with the COLDSTART_REFERENCE
	  // Initialize frequency and position memories if first upload, COLDSTART_REF has been modified in ML.h
	  // since last upload OR if the "Clear All" command has been issued through the Controller Menu functions (0xfe)
	  if (coldstart != COLDSTART_REF)
	  {

	    EEPROM.write(TLUMIENIE_ADRES, tlumienie);           // write running tłumienie into eeprom
	    EEPROM.write(0, COLDSTART_REF);               // COLDSTART_REF in first byte indicates all initialized
#if defined(DEBUG)
	Serial.println("zapis do pamieci wartosci poczatkowych");
#endif
	  }
	  else                                           // EEPROM contains stored data, retrieve the data
	  {
	    //EEPROM_readAnything(TLUMIENIE_ADRES, tlumienie);            // read the current attenuation
	    tlumienie = EEPROM.read(TLUMIENIE_ADRES);
#if defined(DEBUG)
	Serial.print("odczyt z pamieci: ");
	Serial.println(tlumienie);
#endif

	  }
	// set up the LCD's number of columns and rows:
	lcd.begin(16, 2);

	pinMode(ENC_A, INPUT_PULLUP);					// wejście enkodera
	pinMode(ENC_B, INPUT_PULLUP);					// wejście enkodera
	pinMode(T_1dB, OUTPUT);
	pinMode(T_2dB, OUTPUT);
	pinMode(T_4dB, OUTPUT);
	pinMode(T_8dB, OUTPUT);
	pinMode(T_16dB, OUTPUT);
	pinMode(T_32dB, OUTPUT);
	pinMode(T_64dB, OUTPUT);

	krok.attach(SW_1, INPUT_PULLUP);

	Timer1.initialize(250); // set a timer of length 1ms - odczyt wejść enkodera będzie się odbywał co 1ms
	Timer1.attachInterrupt(encode_read); // attach the service routine here
	lcd.print("hello, SP2GSI!");
	delay(500);
	set_tlumienie(0);
	show_tlumienie();
	show_krok();
	set_wyjscia();
}

void loop()
{
	int enc = encode_read4();
	if (enc != 0)
	{
		byla_zmiana = true;
		set_tlumienie(enc);
		show_tlumienie();
		czas_zmiany = millis();
	}
	if (byla_zmiana && (millis() - czas_zmiany > CZAS_REAKCJI))
	{
		EEPROM.write(TLUMIENIE_ADRES, tlumienie);
		set_wyjscia();
		byla_zmiana = false;
#if defined(DEBUG)
		Serial.print("zmiana na wyjsciu i zapis do EEPROM: ");
		Serial.println(tlumienie);
#endif
	}
	krok.update();
	if (krok.read() == LOW)
	{
		if (aktualny_krok == KROK_1dB)
		{
			aktualny_krok = KROK_10dB;
		}
		else
		{
			aktualny_krok = KROK_1dB;
		}
		show_krok();
		delay(200);
	}
}
