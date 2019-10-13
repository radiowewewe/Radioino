// \file Radioino.ino
// \brief FM Radio implementation using RDA5807M, PAM8403, LCD 1602, 4Buttons/TTP224.
//
// \author WeWeWe - Welle West Wetterau e.V.
// \copyright Copyright (c) 2018 by WeWeWe - Welle West Wetterau e.V.
// This work is licensed under BSD 3-Clause License.
//
// \details
// This is a full function radio implementation that uses a LCD display to show the current station information.
// It's using a Arduino Nano for controling other chips via I2C. PAM8403 is used for amplification of the
// output from RDA5807M.
//
// Depencies:
// ----------
// aka the giant shoulders we stand on
// https://github.com/mathertel/Radio
// https://github.com/mathertel/OneButton
// https://www.arduino.cc/en/Reference/LiquidCrystal
//
// Wiring:
// -------
// t.b.d.
//
// Userinterface:
// --------------
// +----------------+---------------------+-----------------+---------------------+----------------------------+
// | Button \ Event |       click()       | double click()  |    long press()     |          comment           |
// +----------------+---------------------+-----------------+---------------------+----------------------------+
// | VolDwn         | -1 volume           | toggle mute     | each 500ms -1       | if vol eq 0 then mute      |
// | VolUp          | +1 volume           | toggle loudness | each 500ms +1       | if muted then first unmute |
// | R3We           | tune 87.8 MHz       | toggle presets  | scan to next sender |                            |
// | Disp           | toggle display mode |                 |                     | > freq > radio > audio >   |
// +----------------+---------------------+-----------------+---------------------+----------------------------+
//
// LCD display:
// +----------------------------------------------------+
// |  Programmname  |  (T)uned (S)tereo (M)ute (B)oost  |
// |  > RDS Text > FREQ > RDS Time > Audio > Signal >   |
// +----------------------------------------------------+
//
// Issues:
// -------
// * In the implementation of "seekUp|Down function" in <RDA5807M.cpp> search for
// "registers[RADIO_REG_CTRL] &= (~RADIO_REG_CTRL_SEEK);" and put it before if statement
// also see comments in https://github.com/mathertel/Radio/issues/5
//
// History:
// --------
// * 02.01.2018 created.
// * 28.01.2018 solution to issue with seek function
// * 08.12.2018 changed display control to direkt wiring; instead of i2c

// - - - - - - - - - - - - - - - - - - - - - - //
//  i n c l u d e s
// - - - - - - - - - - - - - - - - - - - - - - //
#include <radio.h>
#include <RDA5807M.h>
#include <RDSParser.h>
#include <OneButton.h>
#include <LiquidCrystal.h>

// - - - - - - - - - - - - - - - - - - - - - - //
//  g l o b a l s
// - - - - - - - - - - - - - - - - - - - - - - //
//Buttons
#define BUTTON_VOLDOWN 15
#define BUTTON_VOLUP   14
#define BUTTON_R3WE    16
#define BUTTON_DISP    17

// if using PULL-UPs set bool to TRUE and FALSE for PULL-DOWN resistors
// note: arudinos usaly have only bultin PULL-UPs
OneButton buttVolUp(BUTTON_VOLUP, false);
OneButton buttVolDown(BUTTON_VOLDOWN, false);
OneButton buttR3We(BUTTON_R3WE, false);
OneButton buttDisp(BUTTON_DISP, false);

// Define some stations available at your locations here:
// 87.80 MHz as 8780
RADIO_FREQ preset[] = {
  8780,  // * WeWeWe, hopefully your favorite :)
  8930,  // * hr3
  9440,  // * hr1
  10590, // * FFH
  10660  // * Radio Bob
};
byte i_sidx = 0;

// What to Display
// on any change don't forget to change ++ operator below
enum DISPLAY_STATE {
  TEXT,     // radio text (RDS)
  FREQ,     // station freqency
  TIME,     // time (RDS)
  AUDIO,    // audio info / volume
  SIG       // signalinfo SNR RSSI
} displayState = TEXT;

// Overload ++ operator, for cylcling trough
inline DISPLAY_STATE& operator++(DISPLAY_STATE& state, int) {
  const int i = static_cast<int>(state) + 1;
  state = static_cast<DISPLAY_STATE>((i) % 5); //Need to be changed if enum type is changed
  return state;
}

// RDS Time container
struct {
  uint8_t hour = NULL;
  uint8_t minute = NULL;
} rdsTime;

// RDS Text container
String rdsText = "";

// Create an instance of a RDA5807 chip radio
RDA5807M radio;

// Create an instance for RDS info parsing
RDSParser rds;

// Create an instance for LCD 16 chars and 2 line display
// lcd (rs, enable, d0, d1, d2,d3)
LiquidCrystal lcd(7, 6, 2, 3, 4, 5);

// - - - - - - - - - - - - - - - - - - - - - - //
//  functions and callbacks
// - - - - - - - - - - - - - - - - - - - - - - //
// display volume
void volume() {
  //lcd.setCursor(0, 1); for (byte i = 0; i < 16; i++) lcd.print(" ");
  lcd.setCursor(0, 1);
  lcd.print("Vol: ");
  if (radio.getMute()) {
    lcd.print("muted    ");
  }
  else {
    for (uint8_t i = 0; i < ((radio.getVolume() + 1) >> 1); i++) lcd.write((uint8_t) 0xFF);
    if (!(radio.getVolume() & 0x01)) lcd.write((uint8_t) 0x06);
    lcd.print(" ");
    lcd.print(radio.getVolume());
    lcd.print("  ");
  }
} //volume

// callback for RDS parser by radio
void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  rds.processData(block1, block2, block3, block4);
} //RDS_process

// callback display the ServiceName text on the LCD display
void UpdateServiceName(char *name) {
  lcd.setCursor(0, 0); for (byte i = 0; i < 8; i++) lcd.print(" ");
  lcd.setCursor(0, 0); lcd.print(name);
} //UpdateServiceName

// callback update on RDS time
void UpdateRDSTime(uint8_t h, uint8_t m) {
  rdsTime.hour   = h;
  rdsTime.minute = m;
} //UpdateRDSTime

// callback for update on RDS text
void UpdateRDSText(char *text) {
  if (text && text[0]) {
    rdsText = String(text) + "   ";
    lcd.setCursor(0, 1);
    for (byte i = 0; i < 16; i++) lcd.print(" ");
  }
} // UpdateRDSText

// callback for volume down
void VolDown() {
  uint8_t v = radio.getVolume();
  if (v > 0) radio.setVolume(--v);
  else if (v == 0) radio.setMute(true);
  volume(); delay(500);
} //VolDown

// callback for volume up
void VolUp() {
  uint8_t v = radio.getVolume();
  if (radio.getMute()) radio.setMute(false);
  else if (v < 15) radio.setVolume(++v);
  volume(); delay(500);
} //VolUp

// callback for toggle mute
void Mute() {
  radio.setMute(!radio.getMute());
} //Mute

// callback for toggle boost
void Boost() {
  radio.setBassBoost(!radio.getBassBoost());
} //Boost

// callback for toggle display
void Display() {
  displayState++;
} //Display

// callback for R3We Button
void R3We() {
  i_sidx = 0;
  radio.setFrequency(preset[i_sidx]);
} //R3We

// callback for SeekUp
void SeekUp() {
  lcd.setCursor(0, 0); for (byte i = 0; i < 8; i++) lcd.print(" ");
  lcd.setCursor(0, 0); lcd.print("Suche...");
  radio.seekUp(true);
} //SeekUp

// callback for SeekUp futher while button is still pressed
void SeekUp2() {
  RADIO_INFO info;
  radio.getRadioInfo(&info);
  if (info.tuned) radio.seekUp(true);
} //SeekUp

// callback cycling presets
void nextPreset() {
  i_sidx += 1;
  i_sidx %= sizeof(preset) / sizeof (RADIO_FREQ);
  radio.setFrequency(preset[i_sidx]);
} //nextPreset


// - - - - - - - - - - - - - - - - - - - - - - //
//  d i s p l a y   u p d a t e r
// - - - - - - - - - - - - - - - - - - - - - - //
// Update LCD based on displayState
void updateLCD() {
  static RADIO_FREQ lastfreq = 0;
  static uint8_t rdsTexti = 0;
  static DISPLAY_STATE lastDisp = NULL;
  static unsigned long nextScrollTime = 0;

  lcd.setCursor(9, 0);
  RADIO_INFO rinfo;
  AUDIO_INFO ainfo;
  radio.getRadioInfo(&rinfo);
  radio.getAudioInfo(&ainfo);
  rinfo.tuned     ? lcd.print("T ") : lcd.print("  ");  // print "T" if station is tuned
  rinfo.stereo    ? lcd.print("S ") : lcd.print("  ");  // print "S" if tuned stereo
  ainfo.mute      ? lcd.print("M ") : lcd.print("  ");  // print "M" if muted
  ainfo.bassBoost ? lcd.print("B")  : lcd.print(" ");   // print "B" if loundness is ON
  if (displayState != lastDisp) {
    //erase display if state changed
    lastDisp = displayState;
    lcd.setCursor(0, 1);
    for (byte i = 0; i < 16; i++) lcd.print(" ");
  }
  RADIO_FREQ freq = radio.getFrequency();
  if (freq != lastfreq && !buttR3We.isLongPressed()) {
    lastfreq = freq;
    rdsText = "Kein RDS Text ... bitte warten  ";
    if (freq == preset[0]) {
      char *s = " WeWeWe";
      UpdateServiceName(s);
    }
    else {
      char *s = "No Name";
      UpdateServiceName(s);
    }
  }
  lcd.setCursor(0, 1);
  switch (displayState) {
    case FREQ: {
        char s[12];
        radio.formatFrequency(s, sizeof(s));
        lcd.print("Freq: "); lcd.print(s);
        break;
      }
    case TEXT: {
        if (millis() > nextScrollTime) {
          nextScrollTime = millis() + 500;
          lcd.print(rdsText.substring(rdsTexti, rdsTexti + 16));
          (rdsText.length() > rdsTexti + 15) ? rdsTexti++ : rdsTexti = 0;
        }
        break;
      }
    case TIME: {
        if (rdsTime.hour != NULL ) {
          String s;
          s = "     ";
          if (rdsTime.hour < 10) s += "0";
          s += rdsTime.hour;
          s += ":";
          if (rdsTime.minute < 10) s += "0";
          s += rdsTime.minute;
          s += "     ";
          lcd.print(s);
        }
        else {
          lcd.print("NO RDS TIME");
        }
        break;
      }
    case SIG: {
        String s;
        //if you want SNR edit the radio libary
        //s = "SNR: ";
        //s += rinfo.snr;
        s += " RSSI: ";
        s += rinfo.rssi;
        s += "      ";
        lcd.print(s);
        break;
      }
    case AUDIO: {
        volume();
        break;
      }
  } //switch
} //updateLCD

//Display a greeting message :)
void displayGreetings() {
  byte s2[8] = { 0b00000,
                 0b00001,
                 0b00001,
                 0b00011,
                 0b00011,
                 0b00011,
                 0b00011,
                 0b00011
               };
  byte s3[8] = { 0b11111,
                 0b11111,
                 0b11111,
                 0b11110,
                 0b00000,
                 0b00000,
                 0b00000,
                 0b00000
               };
  byte s4[8] = { 0b00011,
                 0b00111,
                 0b01100,
                 0b01101,
                 0b01101,
                 0b01100,
                 0b00111,
                 0b00011
               };
  byte s5[8] = { 0b10011,
                 0b11011,
                 0b01011,
                 0b11011,
                 0b10011,
                 0b00111,
                 0b11110,
                 0b11000
               };
  byte s6[8] = { 0b11100,
                 0b11100,
                 0b11100,
                 0b11100,
                 0b11100,
                 0b11100,
                 0b11100,
                 0b11100
               };
  //lcd.createChar(0, s0);
  //lcd.createChar(1, s1);
  lcd.createChar(2, s2);
  lcd.createChar(3, s3);
  lcd.createChar(4, s4);
  lcd.createChar(5, s5);
  lcd.createChar(6, s6);
  //lcd.backlight();
  lcd.clear();
  lcd.blink();
  lcd.setCursor(1, 0); lcd.write((uint8_t) 0x02); lcd.write((uint8_t) 0x03);
  lcd.print(" *RADIOINO*");
  lcd.setCursor(0, 1); lcd.write((uint8_t) 0x04); lcd.write((uint8_t) 0x05);
  lcd.setCursor(4, 1);
  String s = "Welle West";
  for (byte i = 0; i < 11; i++) {
    lcd.print(s.substring(i, i + 1));
    delay(200);
  }
  delay(600);
  for (byte i = 13; i > 2; i--) {
    lcd.setCursor(i, 1);
    lcd.print(" ");
    delay(100);
  }
  s = "*Wetterau* ";
  for (byte i = 0; i < 12; i++) {
    lcd.print(s.substring(i, i + 1));
    delay(200);
  }
  delay(1400);
  lcd.noBlink();
  lcd.clear();
} //displayGreetings


// - - - - - - - - - - - - - - - - - - - - - - //
//  s e t u p   &   i n i t
// - - - - - - - - - - - - - - - - - - - - - - //
void setup() {
  // Initialize the Display
  lcd.begin(16,2);
//  lcd.backlight();

  // Initialize the Radio
  radio.init();
  radio.setBandFrequency(RADIO_BAND_FM, preset[i_sidx]);
  radio.setMono(false);
  radio.setMute(false);
  radio.setVolume(1);
  displayGreetings();

  // setup the information chain for RDS data.
  radio.attachReceiveRDS(RDS_process);
  rds.attachServicenNameCallback(UpdateServiceName); // callback for rds programmname
  rds.attachTextCallback(UpdateRDSText); // callback for rds radio text
  rds.attachTimeCallback(UpdateRDSTime); // callback for rds time

  // Configure our userinterface
  //attach callback for Buttons
  buttVolUp.attachClick(VolUp);
  buttVolUp.attachDoubleClick(Boost);
  buttVolUp.attachDuringLongPress(VolUp);
  buttVolDown.attachClick(VolDown);
  buttVolDown.attachDoubleClick(Mute);
  buttVolDown.attachDuringLongPress(VolDown);
  buttR3We.attachClick(R3We);
  buttR3We.attachDoubleClick(nextPreset);
  buttR3We.attachLongPressStart(SeekUp);
  buttR3We.attachDuringLongPress(SeekUp2);
  buttDisp.attachClick(Display);
  //  buttDisp.attachDoubleClick();
  //  buttDisp.attachLongPressStart();
} //setup


// - - - - - - - - - - - - - - - - - - - - - - //
//  m a i n   l o o p
// - - - - - - - - - - - - - - - - - - - - - - //
void loop() {
  unsigned long now = millis();
  static unsigned long nextDispTime = 0;
  static unsigned long nextRDSTime = 0;

  // Check Buttons
  buttVolUp.tick();
  buttVolDown.tick();
  buttR3We.tick();
  buttDisp.tick();

  // check for RDS data from time to time
  if (now > nextRDSTime) {
    radio.checkRDS();
    nextRDSTime = now + 50;
  }
  // update the display from time to time
  if (now > nextDispTime) {
    updateLCD();
    nextDispTime = now + 200;
  }
} //loop
