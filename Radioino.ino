// \file Radioino.ino
// \brief FM Radio implementation using RDA5807M, 'Verstärker', LCD 1602, 4Buttons/TTP224.
//
// \author WeWeWe - Welle West Wetterau e.V.
// \copyright Copyright (c) 2018 by WeWeWe - Welle West Wetterau e.V.
// This work is licensed under "t.b.d.".
//
// \details
// This is a full function radio implementation that uses a LCD display to show the current station information.
// Its using a Arduino Nano for controling other chips using I2C
//
// Depencies:
// ----------
// aka the giant shoulders we stand on
// https://github.com/mathertel/Radio
// https://github.com/mathertel/OneButton
// https://github.com/marcoschwartz/LiquidCrystal_I2C
//
// Wiring:
// -------
// t.b.d.
//
// Userinterface:
// --------------
// +----------------+---------------------+-----------------+---------------------+---------------------------+
// | Button \ Event |       click()       | double click()  |     long press      |          comment          |
// +----------------+---------------------+-----------------+---------------------+---------------------------+
// | VolDwn         | -1 volume           | toggle mute     | each 500ms -1       | if vol eq min then mute   |
// | VolUp          | +1 volume           | toggle loudness | each 500ms +1       | if muted the first unmute |
// | R3We           | tune 87.8 MHz       |                 | scan to next sender |                           |
// | Disp           | toggle display mode |                 |                     | > freq > radio > audio >  |
// +----------------+---------------------+-----------------+---------------------+---------------------------+
//
// LCD display:
// +---------------------------------------------------+
// |  Programmname  |  (T)uned (S)tereo (M)ute (B)oost |
// | > RDS Text > FREQ > RDS Time > Audio > Signal >   |
// +---------------------------------------------------+
//
// To Dos:
// -------
// - Button fuctions (long press f)
// - Audio Info Display
// - Cool Vol Info
//
// History:
// --------
// * 02.01.2018 created.
//


// - - - - - - - - - - - - - - - - - - - - - - //
//  i n c l u d e s
// - - - - - - - - - - - - - - - - - - - - - - //
#include <radio.h>
#include <RDA5807M.h>
#include <RDSParser.h>
#include <OneButton.h>
#include <LiquidCrystal_I2C.h>


// - - - - - - - - - - - - - - - - - - - - - - //
//  g l o b a l s
// - - - - - - - - - - - - - - - - - - - - - - //
//Buttons
#define BUTTON_VOLDOWN 2
#define BUTTON_VOLUP   3
#define BUTTON_R3WE    4
#define BUTTON_DISP    5

OneButton buttVolUp(BUTTON_VOLUP, false);
OneButton buttVolDown(BUTTON_VOLDOWN, false);
OneButton buttR3We(BUTTON_R3WE, false);
OneButton buttDisp(BUTTON_DISP, false);

// Define some stations available at your locations here:
// 87.80 MHz as 8780
RADIO_FREQ preset[] = {
  8780,  // * WeWeWe
  8930,  // * hr3
  9440,  // * hr1
  10590, // * FFH
  10660  // * Radio Bob
};
int    i_sidx = 1;

// What to Display
// on any change don't forget to change ++ operator below
enum DISPLAY_STATE {
  TEXT,     // radio text (RDS)
  FREQ,     // station freqency
  TIME,     // time (RDS)
  AUDIO,    // Volume info, muted, stereo/mono, boost
  SIG       // signalinfo SNR RSSI
} displayState = 0;

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
String rdsText = " ";

// Create an instance of a RDA5807 chip radio
RDA5807M radio;

// Create an instance for RDS info parsing
RDSParser rds;

// Create an instance for LCD 16 chars and 2 line display
// LCD address may vary
LiquidCrystal_I2C lcd(0x27, 16, 2);


// - - - - - - - - - - - - - - - - - - - - - - //
//  functions and callbacks
// - - - - - - - - - - - - - - - - - - - - - - //
// display volume on change
void volume() {
  lcd.setCursor(0, 1);
  lcd.print("Vol:" );
  lcd.print(radio.getVolume());
  delay(300);
} //volume

// callback for RDS parser by radio
void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  rds.processData(block1, block2, block3, block4);
} //RDS_process

// callback pdate the ServiceName text on the LCD display
void UpdateServiceName(char *name) {
  lcd.setCursor(0, 0); lcd.print("        "); // clear 8 chars
  lcd.setCursor(0, 0); lcd.print(name);
} // DisplayServiceName

// callback update on RDS time
void UpdateRDSTime(uint8_t h, uint8_t m) {
  rdsTime.hour = h;
  rdsTime.minute = m;
} //UpdateRDSTime

// callback for update on RDS text
void UpdateRDSText(char *text) {
  rdsText = String(text);
} // UpdateRDSText

// callback for volume down
void VolDown() {
  int v = radio.getVolume();
  if (v > 0) radio.setVolume(--v);
  else if (v == 0) radio.setSoftMute(true);
  volume();
} //VolDown

// callback for volume up
void VolUp() {
  int v = radio.getVolume();
  if (v < 15) radio.setVolume(++v);
  else if (v >= 0) radio.setSoftMute(false);
  volume();
} //VolUp

// callback for toggle mute
void Mute() {
  radio.setSoftMute(!radio.getSoftMute());
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
  radio.setFrequency(preset[0]);
} //R3We

// callback for SeekUp
void SeekUp() {
  radio.seekUp(true);
} //SeekUp


// - - - - - - - - - - - - - - - - - - - - - - //
//  d i s p l a y   u p d a t e r
// - - - - - - - - - - - - - - - - - - - - - - //
// Update LCD based on displayState
void updateLCD() {
  static RADIO_FREQ lastfreq = 0;
  static uint8_t lastmin = 0;
  static uint8_t rdsTexti = 0;
  static DISPLAY_STATE lastDisp = NULL; 
  lcd.setCursor(9, 0);
  RADIO_INFO info;
  AUDIO_INFO ainfo;
  radio.getRadioInfo(&info);
  radio.getAudioInfo(&ainfo);
  info.tuned      ? lcd.print("T ") : lcd.print("  ");  // print "T" if station is tuned
  info.stereo     ? lcd.print("S ") : lcd.print("  ");  // print "S" if tuned stereo
  ainfo.softmute  ? lcd.print("M ") : lcd.print("  ");  // print "M" if muted
  ainfo.bassBoost ? lcd.print("B")  : lcd.print(" ");   // print "B" if loundness is ON
  if (displayState != lastDisp) {
    //erase display if state changed
    lastDisp = displayState;
    lcd.setCursor(0, 1);
    lcd.print("                 ");
  }
  RADIO_FREQ freq = radio.getFrequency();
  if (freq != lastfreq) {
    lastfreq = freq;
    rdsText = "Kein RDS Text ... bitte warten";
    if (freq == preset[0]) {
      char *s = " WeWeWe";
    }
    else {
      char *s = "No Name"; 
    }
    UpdateServiceName(s);
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
        lcd.print(rdsText.substring(rdsTexti, rdsTexti + 15));
        (rdsText.length() > rdsTexti + 15) ? rdsTexti++ : rdsTexti = 0;
        break;
      }
    case AUDIO: {
        lcd.print("Audio not impl.");
      }
    case TIME: {
        if (rdsTime.hour != NULL ) {
          if (rdsTime.minute != lastmin) {
            lastmin = rdsTime.minute;
            String s;
            s = "      ";
            if (rdsTime.hour < 10) s += "0";
            s += rdsTime.hour;
            s += ":";
            if (rdsTime.minute < 10) s += "0";
            s += rdsTime.minute;
            s += "     ";
            lcd.print(s);
          }
        }
        else {
          lcd.print("NO RDS DATE/TIME");
        }
        break;
      }
    case SIG: {
        String s;
        s = "SNR: ";
        s += info.snr;
        s += " RSSI: ";
        s += info.rssi;
        s += "   ";
        lcd.print(s);
        break;
      }
    default:
      break; // do nothing
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
  lcd.backlight();
  lcd.clear();
  lcd.blink();
  lcd.setCursor(1, 0); lcd.write((uint8_t) 2); lcd.write((uint8_t) 3);
  lcd.print(" *RADIOINO*");
  lcd.setCursor(0, 1); lcd.write((uint8_t) 4); lcd.write((uint8_t) 5);
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
  delay(1600);
  lcd.noBlink();
  lcd.clear();
} //displayGreetings


// - - - - - - - - - - - - - - - - - - - - - - //
//  s e t u p   &   i n i t
// - - - - - - - - - - - - - - - - - - - - - - //
void setup() {
  // Initialize the Display
  lcd.init();
  
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
  //Callback for Buttons
  buttVolUp.attachClick(VolUp);
  buttVolUp.attachDoubleClick(Boost);
  //  buttVolUp.attachDuringLongPress();
  buttVolDown.attachClick(VolDown);
  buttVolDown.attachDoubleClick(Mute);
  //  buttVolDown.attachDuringLongPress();
  buttR3We.attachClick(R3We);
  //  buttR3We.attachDoubleClick();
  buttR3We.attachLongPressStart(SeekUp);
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

  // check for RDS data
  if (now > nextRDSTime) {
    radio.checkRDS();
    nextRDSTime = now + 50;
  }
  // update the display from time to time
  if (now > nextDispTime) {
    updateLCD();
    nextDispTime = now + 400;
  }
} //loop
