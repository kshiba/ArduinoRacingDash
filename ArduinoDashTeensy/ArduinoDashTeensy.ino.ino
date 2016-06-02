#include <avr/pgmspace.h>
#include <Adafruit_NeoPixel.h>
#include "SPI.h"
#include "ILI9341_t3.h"
//#include <Fonts/FreeMonoBold18pt7b.h>

#if defined(__SAM3X8E__)
#undef __FlashStringHelper::F(string_literal)
#define F(string_literal) string_literal
#endif

//COLOR CODE
#define LIGHTRED 0xF8A0
#define LIGHTGREEN 0xAFEC
#define GREEN 0xFF00
#define RED 0xFF00

//For Teensy 3.2 SPI pin
#define _sclk 13
#define _miso 12
#define _mosi 11
#define _cs 10
#define _dc 15
#define _rst 14

//Neopixel pin Setting
#define LEDmax 8
#define LEDpin 16
boolean ltype = true;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDmax, LEDpin, NEO_RGB + NEO_KHZ800);
ILI9341_t3 tft = ILI9341_t3(_cs, _dc, _rst);

byte LEDposition;
int LEDcolor, LEDcolorR, LEDcolorG, LEDcolorB;
boolean LEDflag;

byte colR[11] = {0, 0, 24, 24, 24, 24, 24,24};
byte colG[11] = {0, 0, 24, 24, 24, 0, 0, 0};
byte colB[11] = {24, 24, 0, 0, 0, 0, 0, 0};
word leds [17] = { 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,255};


PROGMEM  prog_uchar VERSION[] = {2, 1, 0, 3};
PROGMEM  prog_uint16_t ledsLong[2][17] = {{0, 1, 3, 7, 15, 31, 63, 127, 255, 256, 768, 1792, 3840, 7936, 7968, 8032, 8160}, {0, 1, 3, 7, 15, 31, 63, 127, 255, 1, 3, 7, 15, 31, 8223, 24607, 57375}};
PROGMEM  prog_uint16_t ledsShort[2][9] = {{0, 256, 768, 1792, 3840, 7936, 7968, 8032, 8160}, {0, 1, 3, 7, 15, 31, 8223, 24607, 57375}};
//PROGMEM  prog_uint16_t ledsShort[2][9] = {{0, 255, 6375, 15555, 15555, 32385, 32385, 65280, 65280}, {0, 1, 3, 7, 15, 31, 8223, 24607, 57375}};

byte bsettings, base, buttons, oldbuttons, page, oldpage, e2, lapComplete, lapActive, lapReset;
int intensity, oldintensity, ledNum, pitLimiterColor, deltaneg, delta, blinkVal, lowFuel;
byte gear, spd_h, spd_l, shift, rpm_h, rpm_l, delta_h, delta_l, tm, engine, lap, invert, ledCRL, f1, f2;

byte ogear,ospd,oshift,oengine,olap;
int ofuel;
word orpm;

char st1;
char st2[5];
char st3[10];
String boost;
int fuel,spd, brk, mins, secs, milsecs;
word rpm,maxrpm;
boolean changed, blinkrpm, ledOff, firstLap;
unsigned long milstart, milstart2 = 0, mils, lapmils, cnctGap;

void setup() {
    tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  strip.begin();
  tft.setRotation(3);
  
  tft.fillScreen(ILI9341_BLACK);
  InitLCD();
  orpm=1;
  ospd=1;
  ogear=0;
  ofuel=1;
  olap=1;
 LCDFuel(0);
 LCDGear(7);
 LCDRPM(0);
 LCDSPD(0);
 LEDLight(7);
 LCDLAP(0);
 displayLapTime();
  delay(3000);
  LCDGear(1);
 LEDLight(0);
        Serial.begin(9600);
                  
	      oldbuttons = 0;
	      page = 0;
        oldpage = 0;
        changed = false;
        blinkrpm = false;
        ledOff = false;
        intensity = 0;
        oldintensity = 0;
        pitLimiterColor = GREEN;
        maxrpm=0;
        delay(3000);
}

void displayLapTime(){
        if (lapActive == 0){
          mins = 0;
          secs = 0;
          milsecs = 0;  
        }
        
        if (mins > 99){
          mins = 99; 
        }
        
        sprintf(st3,"%02d:%02d.%03d",mins,secs,milsecs);
  const int left = 120;
  const int top = 205;

  tft.fillRect(left, top, 180, 25, ILI9341_BLACK);
  tft.setCursor(left, top);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.print(st3);
          
}

void update() {
        if ((millis() - cnctGap) > 1000) {
          lapActive = 0;  
        }
        if (Serial.available() > 0) {
          if (Serial.available() > 17) {
            if (Serial.read() == 255) {
              bsettings = Serial.read();                                
      				gear = Serial.read();
      				spd_h = Serial.read();
              spd_l = Serial.read();
      				rpm_h = Serial.read();
      				rpm_l = Serial.read();
      				f1 = Serial.read();
              f2 = Serial.read();
              shift = Serial.read();
      				e2 = Serial.read();
              lap = Serial.read();
              boost = String(Serial.read());
              delta_h = Serial.read();
      				delta_l = Serial.read();
              tm = Serial.read();
                                
              base = bsettings & 15;
              intensity = (bsettings & 112) >> 4;
              deltaneg = (bsettings & 128) >> 7;
                                
              if  ((e2 & 1) == 1)
                engine = 0x10;
              else
                engine = 0x00;
              
              lowFuel = (e2 & 7) >> 1;
              brk = (e2 & 15) >> 3;
              lapActive = (e2 & 31) >> 4;
              lapReset = (e2 & 63) >> 5;
              lapComplete = (e2 & 127) >> 6;
                                
              spd = (spd_h << 8) | spd_l;
				      rpm = (rpm_h << 8) | rpm_l;
              fuel =(f1 << 8) | f2;
                                
              if (lapReset == 1)
                mils = millis(); 
                                
              if (lapComplete == 1) {
                                  lapmils = millis();                                  
                                  int tmp = (delta_h << 8)| delta_l;                                  
                                  milsecs = tmp & 1023;
                                  secs = (tmp & 65024) >> 9;
                                  mins = tm;
                                  displayLapTime();
                                  
              } else {
                                  delta = (delta_h << 8)| delta_l;
              }
                                                                
              cnctGap = millis();
              LCDGear(gear);
              LCDRPM(rpm);
              LCDSPD(spd);
              LCDFuel(fuel);
              LCDLAP(lapComplete);
              setShiftLight(engine,shift);
                                
            }
         } else if ((Serial.available() > 1) && (Serial.available() < 4)) {
                    bsettings = Serial.read();
                    Serial.print(".");
                    Serial.print(pgm_read_byte_near(VERSION + 0));
                    Serial.print(pgm_read_byte_near(VERSION + 1));
                    Serial.print(pgm_read_byte_near(VERSION + 2));
                    Serial.print(pgm_read_byte_near(VERSION + 3));
        }
                        
	}

}

void loop() {
  update();

  //if (intensity != oldintensity) {   
    //modules[0]->setupDisplay(true, intensity); 
    //modules[1]->setupDisplay(true, intensity);  
  //}
}
void LEDLight(short num) {
  short num2;
  for (int i = 0; i < LEDmax; i++) {
    if (i < num) {
      LEDcolorR = colR[i];
      LEDcolorG = colG[i];
      LEDcolorB = colB[i];
    } else {
      LEDcolor = 0;
      LEDcolorR = LEDcolorG = LEDcolorB = LEDcolor;
    }
    strip.setPixelColor(i, strip.Color(LEDcolorG, LEDcolorR, LEDcolorB));
    strip.show();
  }
}

void LCDGear(int val) {
  const int left = 140;
  const int top = 100;
  //tft.setFont(&FreeMonoBold18pt7b);
  tft.setTextSize(7);
  if(val==ogear) return;
  else ogear =val;
  
  if (val == 0)
    st1 = 'R';
  else if (val == 1)
    st1 = 'N';
  else
    st1 = String(val - 1, DEC).charAt(0);
  
  tft.fillRect(left, top, 40, 60, ILI9341_BLACK);
  tft.setCursor(left, top);
  tft.setTextColor(ILI9341_WHITE);
  tft.print(st1);
//  tft.setFont();
}

void LCDRPM(int val) {
  const int left = 117;
  const int top = 28;

  if(val==orpm) return;
  else  orpm=val;
//  tft.setFont();
  sprintf(st2, "%05d", val);
  tft.fillRect(left, top, 90, 25, ILI9341_BLACK);
  tft.setCursor(left, top);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.print(st2);
}

void LCDLAP(int val) {
  const int left = 240;
  const int top = 28;
//tft.setFont();
  if(val==olap) return;
  else  olap=val;

  sprintf(st2, "%03d", val);
  tft.fillRect(left, top, 50, 25, ILI9341_BLACK);
  tft.setCursor(left, top);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.print(st2);
}


void LCDFuel(int val) {
  const int left = 30;
  const int top = 120;
//tft.setFont();
  if(val==ofuel) return;
  else  ofuel=val;

  sprintf(st2, "%03d", val);
  tft.fillRect(left, top, 60, 23, ILI9341_BLACK);
  tft.setCursor(left, top);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.print(st2);
}

void LCDSPD(int val) {
  const int left = 240;
  const int top = 120;
//tft.setFont();
  if(val==ospd)return;
  else ospd=val;

  sprintf(st2, "%03d", val);
  tft.fillRect(left, top, 60, 23, ILI9341_BLACK);
  tft.setCursor(left, top);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.print(st2);
}

void setShiftLight(int eg, int sf) {
 
  if ((eg & 0x10) == 0) {
    if (sf == 16) {
      if ((millis() - milstart2) > 50) {
        if (blinkrpm == false) {
          LEDLight(0);
          blinkrpm = true;
        } else {
          LEDLight(leds[sf]);
          blinkrpm = false;
        }
        milstart2 = millis();
      }
    } else {
      LEDLight(leds[sf]);
    }
  } else {
    if ((millis() - milstart2) > 200) {
      if (blinkrpm == false) {
        LEDLight(0);
        blinkrpm = true;
      } else {
        LEDLight(LEDmax);
        blinkrpm = false;
      }
      milstart2 = millis();
    }
  }
}

void InitLCD(){
  //Gear 
  tft.drawRoundRect(110,75,100,100,10, ILI9341_WHITE);
  tft.fillRoundRect(120,65,80,20,3, ILI9341_WHITE);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(137, 68);
  tft.setTextSize(2);
  tft.print("GEAR");
  
  //FUEL
  tft.drawRoundRect(5,75,100,100,10, ILI9341_YELLOW);
  tft.fillRoundRect(15,65,80,20,3, ILI9341_YELLOW);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(32, 68);
  tft.setTextSize(2);
  tft.print("FUEL");

  //SPEED
  tft.drawRoundRect(215,75,100,100,10, ILI9341_CYAN);
  tft.fillRoundRect(225,65,80,20,3, ILI9341_CYAN);
  tft.setCursor(248, 68);
  tft.setTextSize(2);
  tft.print("SPD");

  //RPM
  tft.drawRoundRect(110,10,100,50,10, ILI9341_GREEN);
  tft.fillRoundRect(120,0,80,20,3, ILI9341_GREEN);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(143, 3);
  tft.setTextSize(2);
  tft.print("RPM");

  //LAP
  tft.drawRoundRect(215,10,100,50,10, LIGHTRED);
  tft.fillRoundRect(225,0,80,20,3, LIGHTRED);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(248, 3);
  tft.setTextSize(2);
  tft.print("LAP");

  //LAP TIME
  tft.drawRoundRect(5,190,310,50,10, LIGHTGREEN);
  tft.fillRoundRect(15,180,80,20,3, LIGHTGREEN);
  tft.setCursor(32, 183);
  tft.setTextSize(2);
  tft.print("TIME");

}

