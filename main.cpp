#include <Arduino.h>
#include <IRremote.h>
#include <EEPROM.h>

const unsigned long BTN_MUTE = 0xffb24d;
const unsigned long BTN_MAXVOL = 0xff6897;
const unsigned long BTN_VOLUP = 0xff7887;
const unsigned long BTN_VOLDWN = 0xff50af;
const unsigned long BTN_REC = 0xff32cd;
const unsigned long BTN_BURST = 0xffffffff;
const unsigned long BTN_9 = 0xffe817;
const unsigned long BTN_8 = 0xffa857;
const unsigned long BTN_7 = 0xff8877;

#define PIN_IR_RECV 11
#define PIN_POT_INC_L 9
#define PIN_POT_INC_R 8
#define PIN_POT_UD_LR 10

#define NEG_EDGE_SLEEP_MUS 3
#define POT_STEPS 105
#define VOL_STEPS 39
#define VOL_STEP_DEFAULT 20
#define VOL_RIGHT_OFFSET_ZERO 100
#define EEPROM_DEFAULT 0xff
#define EEPROM_VOLUME_IDX 0
#define EEPROM_RIGHT_OFFSET_IDX 1

#define ENABLE_SERIAL_OUT

#ifdef ENABLE_SERIAL_OUT 
#define SERIALOUT(x) Serial.println(x);
#define SERIALOUT2(x,y) Serial.println(x,y);
#else
#define SERIALOUT(x)
#define SERIALOUT2(x,y)
#endif

IRrecv g_irrecv(PIN_IR_RECV);
decode_results g_irResults;
unsigned long g_lastIrCode;
uint8_t g_volIdx;
uint8_t g_rightOffset;


const uint8_t g_logResValuesKOhms[VOL_STEPS] = 
{
100,99,98,97,96,95,94,93,92,91,
90,89,88,87,86,85,84,83,82,81,
80,77,75,72,69,66,63,59,56,52,
47,43,38,32,27,21,15,9,0
};


void storeVolToEeprom()
{
  SERIALOUT("eeprom save");
  EEPROM.write(EEPROM_VOLUME_IDX, g_volIdx);
  EEPROM.write(EEPROM_RIGHT_OFFSET_IDX, g_rightOffset);
  SERIALOUT2(g_volIdx, DEC);
  SERIALOUT2(g_rightOffset, DEC);
}

void restoreVolFromEeprom()
{
  SERIALOUT("eeprom load");
  g_volIdx = EEPROM.read(EEPROM_VOLUME_IDX);
  g_rightOffset = EEPROM.read(EEPROM_RIGHT_OFFSET_IDX);

  if(g_volIdx == EEPROM_DEFAULT)
    g_volIdx = VOL_STEP_DEFAULT;
  if(g_rightOffset == EEPROM_DEFAULT)
    g_rightOffset = VOL_RIGHT_OFFSET_ZERO;

  SERIALOUT2(g_volIdx, DEC);
  SERIALOUT2(g_rightOffset, DEC);
}

uint8_t calculateRightOffsetIdx(uint8_t leftIdx)
{
  SERIALOUT("calculateRightOffsetIdx");
  SERIALOUT2(leftIdx, DEC);
  int offset = g_rightOffset - VOL_RIGHT_OFFSET_ZERO;
  int rightIdx = leftIdx - offset;

  int res = max(0,min(rightIdx, VOL_STEPS));
  SERIALOUT2(res, DEC);
  return res;
}

void setVolume(uint8_t resistance, uint8_t pin)
{
    for(uint8_t idx = 0; idx < resistance; idx++)
    {
      digitalWrite(pin, LOW);
      delayMicroseconds(NEG_EDGE_SLEEP_MUS);
      digitalWrite(pin, HIGH);
      delayMicroseconds(NEG_EDGE_SLEEP_MUS);
    }
}

void setVolumeZero()
{
  SERIALOUT("setVolumeZero");
  digitalWrite(PIN_POT_UD_LR, HIGH); 
  setVolume(POT_STEPS, PIN_POT_INC_L);
  setVolume(POT_STEPS, PIN_POT_INC_R);
}

void setVolumeMax()
{
  SERIALOUT("setVolumeMax");
  digitalWrite(PIN_POT_UD_LR, LOW); 
  setVolume(POT_STEPS, PIN_POT_INC_L);
  setVolume(POT_STEPS, PIN_POT_INC_R);
}

void setVolume(uint8_t volIdx)
{
  SERIALOUT("setVolume");
  SERIALOUT2(g_volIdx, DEC);

  if(volIdx < VOL_STEPS)
  {
    uint8_t resistanceL = g_logResValuesKOhms[volIdx];
    uint8_t resistanceR = g_logResValuesKOhms[calculateRightOffsetIdx(volIdx)];
    setVolumeZero();
    
    digitalWrite(PIN_POT_UD_LR, LOW); 
    setVolume(100 - resistanceL, PIN_POT_INC_L);
    setVolume(100 - resistanceR, PIN_POT_INC_R);
  }
}

void OnVolUp()
{
  SERIALOUT("OnVolUp");
  SERIALOUT2(g_volIdx, DEC);
  if(g_volIdx < VOL_STEPS)
  {
    g_volIdx++;
    setVolume(g_volIdx);
  }
}

void OnVolDown()
{
  SERIALOUT("OnVolDown");
  SERIALOUT2(g_volIdx, DEC);
  if(g_volIdx > 0)
  {
    g_volIdx--;
    setVolume(g_volIdx);
  }
}

void OnMute()
{
  g_volIdx = 0;
  setVolumeZero();
}

void OnMax()
{
  g_volIdx = VOL_STEPS;
  setVolumeMax();
}

void OnRecord()
{
  storeVolToEeprom();
}

void OnRightOffsetUp()
{
  g_rightOffset++;
  setVolume(g_volIdx);
  SERIALOUT("OnRightOffsetUp");
  SERIALOUT2(g_rightOffset, DEC);

}

void OnRightOffsetZero()
{
  g_rightOffset = VOL_RIGHT_OFFSET_ZERO;
  setVolume(g_volIdx);
  SERIALOUT("OnRightOffsetZero");
}

void OnRightOffsetDown()
{
  g_rightOffset--;
  setVolume(g_volIdx);
  SERIALOUT("OnRightOffsetDown");
  SERIALOUT2(g_rightOffset, DEC);
}

void setup()
{
  #ifdef ENABLE_SERIAL_OUT
  Serial.begin(9600);
  #endif
  g_irrecv.enableIRIn();

  pinMode(PIN_POT_INC_L, OUTPUT);
  pinMode(PIN_POT_INC_R, OUTPUT);
  pinMode(PIN_POT_UD_LR, OUTPUT);
  digitalWrite(PIN_POT_INC_L, HIGH); 
  digitalWrite(PIN_POT_INC_R, HIGH); 
  digitalWrite(PIN_POT_UD_LR, HIGH); 
  restoreVolFromEeprom();
  setVolume(g_volIdx);
}

void loop() 
{
  if (g_irrecv.decode(&g_irResults)) 
  {
    SERIALOUT2(g_irResults.value, HEX);
    switch (g_irResults.value)
    {
    case BTN_MUTE:
      g_lastIrCode = 0;
      OnMute();
      break;
    case BTN_VOLUP:
      g_lastIrCode = g_irResults.value;
      OnVolUp();
      break;
    case BTN_VOLDWN:
      g_lastIrCode = g_irResults.value;
      OnVolDown();
      break;
    case BTN_MAXVOL:  
      g_lastIrCode = 0;
      OnMax();
      break;
    case BTN_REC:  
      g_lastIrCode = 0;
      OnRecord();
      break; 
    case BTN_9:  
      g_lastIrCode = 0;
      OnRightOffsetUp();
      break;              
    case BTN_7:  
      g_lastIrCode = 0;
      OnRightOffsetDown();
      break;
    case BTN_8:  
      g_lastIrCode = 0;
      OnRightOffsetZero();
      break;                
    case BTN_BURST:
      switch (g_lastIrCode)
      {
        case BTN_VOLUP:
          OnVolUp();
          break;
        case BTN_VOLDWN:
          OnVolDown();
          break;
      }
      break;
    default:
      break;
    }
    g_irrecv.resume();
  }
  delay(100);
}
