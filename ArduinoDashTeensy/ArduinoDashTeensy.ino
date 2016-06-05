#include <avr/pgmspace.h>
#include <Adafruit_NeoPixel.h>
#include "SPI.h"
#include "ILI9341_t3.h"
//#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>

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

//joystick Setting;
#define BTNNum 17
#define BTNRefleshRate 15  //msec
#define BTN_OFF 0
#define BTN_ON 1
#define MAXBTN 32
#define NBTNNum 11
#define ENCNum 3
#define ENC_CW 0
#define ENC_CCW 1
#define ENC_BTN_ON 30

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDmax, LEDpin, NEO_RGB + NEO_KHZ800);
ILI9341_t3 tft = ILI9341_t3(_cs, _dc, _rst);

PROGMEM  prog_uchar VERSION[] = {2, 1, 0, 3};

//for neopixel control
boolean LEDflag;
boolean ltype = true;

const byte colR[11] = {0, 0, 24, 24, 24, 24, 24, 24};
const byte colG[11] = {0, 0, 24, 24, 24, 0, 0, 0};
const byte colB[11] = {24, 24, 0, 0, 0, 0, 0, 0};
const word leds [17] = { 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 255};

//joistick button control
int btnRefresh = 0;
byte btnStatus[BTNNum];
byte btnStatusOLD[BTNNum];
const int btnPin[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 23};
const int encPin1[] = {17, 19, 21};
const int encPin2[] = {18, 20, 22};
const int encJNum[2][3] = {{13, 15, 17}, {14, 16, 18}};
long encCurrntPos[3];
int encBtnTime[3];

Encoder je1(encPin1[0], encPin2[0]);
Encoder je2(encPin1[1], encPin2[1]);
Encoder je3(encPin1[2], encPin2[2]);

//LCD Control
int refleshCount = 0;
boolean enblLapUpdate = false;

//data control variable
//TODO: Ajust var
byte base, lapComplete, lapActive, lapReset;

byte gear,  shift, engine, lap;
byte ogear, oshift, oengine, olap;

int intensity, oldintensity, ledNum, deltaneg, delta, lowFuel;

String boost;
int fuel,  spd, brk, mins, secs, milsecs;
int ofuel, ospd;
word rpm, orpm;

boolean blinkrpm, ledOff, firstLap;
unsigned long milstart, milstart2 = 0, mils, lapmils, cnctGap;


void setup() {
  //joystick initialize
  for (int i = 0; i < BTNNum; i++) {
    btnStatus[i] = BTN_OFF;
    btnStatusOLD[i] = BTN_OFF;
  }
  for (int i = 0; i < NBTNNum; i++) {
    pinMode(btnPin[i], INPUT_PULLUP);
  }
  for (int i = 0; i < ENCNum; i++) {
    encCurrntPos[i] = -999;
    encBtnTime[i] = millis();
  }
  for (int i = 1; i < MAXBTN + 1; i++) {
    Joystick.button(i, 0);
  }

  initLCD();

  //initialize etc.
  blinkrpm = false;
  ledOff = false;
  intensity = 0;
  oldintensity = 0;

  //Start Control!
  Serial.begin(9600);
  Joystick.hat(-1);
}

void loop() {
  if ((millis() - cnctGap) > 1000) {
    lapActive = 0;
  }
  rcvChk();
  lcdUpdate();
  for (int i = 0; i < NBTNNum; i++)
    BtnChk(i);
  for (int i = 0; i < ENCNum; i++)
    EncChk(i);
}

void rcvChk() {
  byte bsettings, e2, f1, f2, spd_h, spd_l, rpm_h, rpm_l, delta_h, delta_l, tm;

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
        fuel = (f1 << 8) | f2;

        if (lapReset == 1)
          mils = millis();

        if (lapComplete == 1) {
          lapmils = millis();
          int tmp = (delta_h << 8) | delta_l;
          milsecs = tmp & 1023;
          secs = (tmp & 65024) >> 9;
          mins = tm;
          enblLapUpdate = true;;
        } else {
          delta = (delta_h << 8) | delta_l;
        }

        cnctGap = millis();
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

void lcdUpdate() {

  //update LCD and LED
  switch (refleshCount) {
    case 0:   //rpm fuel
      LCDRPM(rpm);
      LCDFuel(fuel);
      refleshCount++;
      break;
    case 1:   //gear Shiftlight
      LCDGear(gear);
      setShiftLight(engine, shift);
      refleshCount++;
      break;
    case 2:   //spd LapTime LAP
      LCDSPD(spd);
      if (enblLapUpdate) {
        displayLapTime();
        enblLapUpdate = false;
      }
      LCDLAP(lapComplete);
      refleshCount = 0;
      break;
    default:
      refleshCount = 0;
      break;
  }
  //update button status
  if (btnRefresh - millis() > BTNRefleshRate ) {
    btnRefresh = millis();

  }
}

void displayLapTime() {
  char st3[10];

  if (lapActive == 0) {
    mins = 0;
    secs = 0;
    milsecs = 0;
  }

  if (mins > 99)
    mins = 99;

  sprintf(st3, "%02d:%02d.%03d", mins, secs, milsecs);
  const int left = 120;
  const int top = 205;

  tft.fillRect(left, top, 180, 25, ILI9341_BLACK);
  tft.setCursor(left, top);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.print(st3);

}

void LEDLight(short num) {
  int LEDcolor, LEDcolorR, LEDcolorG, LEDcolorB;

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
  char st1;

  //tft.setFont(&FreeMonoBold18pt7b);
  tft.setTextSize(7);
  if (val == ogear) return;
  else ogear = val;

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
}

void LCDRPM(word val) {
  const int left = 117;
  const int top = 28;
  char st2[5];

  if (val == orpm) return;
  else  orpm = val;
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
  char st2[5];

  //tft.setFont();
  if (val == olap) return;
  else  olap = val;

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
  char st2[5];

  //tft.setFont();
  if (val == ofuel) return;
  else  ofuel = val;

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
  char st2[5];
  //tft.setFont();
  if (val == ospd)return;
  else ospd = val;

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

void initLCD() {
  //initialize LCD data and Test
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  strip.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);

  //Gear
  tft.drawRoundRect(110, 75, 100, 100, 10, ILI9341_WHITE);
  tft.fillRoundRect(120, 65, 80, 20, 3, ILI9341_WHITE);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(137, 68);
  tft.setTextSize(2);
  tft.print("GEAR");

  //FUEL
  tft.drawRoundRect(5, 75, 100, 100, 10, ILI9341_YELLOW);
  tft.fillRoundRect(15, 65, 80, 20, 3, ILI9341_YELLOW);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(32, 68);
  tft.setTextSize(2);
  tft.print("FUEL");

  //SPEED
  tft.drawRoundRect(215, 75, 100, 100, 10, ILI9341_CYAN);
  tft.fillRoundRect(225, 65, 80, 20, 3, ILI9341_CYAN);
  tft.setCursor(248, 68);
  tft.setTextSize(2);
  tft.print("SPD");

  //RPM
  tft.drawRoundRect(110, 10, 100, 50, 10, ILI9341_GREEN);
  tft.fillRoundRect(120, 0, 80, 20, 3, ILI9341_GREEN);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(143, 3);
  tft.setTextSize(2);
  tft.print("RPM");

  //LAP
  tft.drawRoundRect(215, 10, 100, 50, 10, LIGHTRED);
  tft.fillRoundRect(225, 0, 80, 20, 3, LIGHTRED);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(248, 3);
  tft.setTextSize(2);
  tft.print("LAP");

  //LAP TIME
  tft.drawRoundRect(5, 190, 310, 50, 10, LIGHTGREEN);
  tft.fillRoundRect(15, 180, 80, 20, 3, LIGHTGREEN);
  tft.setCursor(32, 183);
  tft.setTextSize(2);
  tft.print("TIME");

  orpm = 1;
  ospd = 1;
  ogear = 0;
  ofuel = 1;
  olap = 1;
  LCDFuel(0);
  LCDGear(7);
  LCDRPM(0);
  LCDSPD(0);
  LEDLight(7);
  LCDLAP(0);
  displayLapTime();

  delay(3000);
  gear = 1;
  LCDGear(1);
  LEDLight(0);
}

void EncChk(int num) {
  //TOD: add overflow Check
  long value;
  switch (num) {
    case 0:
      value = je1.read();
      break;
    case 1:
      value = je2.read();
      break;
    case 2:
      value = je3.read();
      break;
    default:
      return;
  }
  if (value != encCurrntPos[num]) {
    // to off if before Count is still on
    Joystick.button(encJNum[ENC_CW][num], 0);
    Joystick.button(encJNum[ENC_CCW][num], 0);
    delayMicroseconds(250);
    if (value > encCurrntPos[num])
      Joystick.button(encJNum[ENC_CW][num], 1);
    if (value < encCurrntPos[num])
      Joystick.button(encJNum[ENC_CCW][num], 1);
    encCurrntPos[num] = value;
    encBtnTime[num] = millis();
  }
  if (encBtnTime[num] - millis() > ENC_BTN_ON) {
    Joystick.button(encJNum[ENC_CW][num], 0);
    Joystick.button(encJNum[ENC_CCW][num], 0);
    encBtnTime[num] = millis();
  }
}

void BtnChk(int num) {
  //TODO: add Bounce Check
  int data = 0x01 & digitalRead(btnPin[num]);

  if (data == 1)
    Joystick.button(num + 1, 0);
  else
    Joystick.button(num + 1, 1);
}

