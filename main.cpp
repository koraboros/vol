#include <Arduino.h>
#include <IRremote.h>
#include <EEPROM.h>

const unsigned long MY_MUTE = 0xffb24d;
const unsigned long MY_MAXVOL = 0xff6897;
const unsigned long MY_VOLUP = 0xff7887;
const unsigned long MY_VOLDWN = 0xff50af;
const unsigned long MY_REC = 0xff32cd;
const unsigned long MY_BURST = 0xffffffff;
const unsigned long MY_9 = 0xffe817;
const unsigned long MY_8 = 0xffa857;
const unsigned long MY_7 = 0xff8877;

#define IR_RECV_PIN 11
#define INC_PIN_L 9
#define INC_PIN_R 7
#define UD_PIN 8
#define BURST_SLEEP_MUS 3
#define POT_STEPS 105
#define VOL_STEPS 39
#define UNINTILAIZED_EEPROM 0xff
#define DEFULT_VOL_STEP 20

#define EEPROM_VOLUME_IDX 0
#define EEPROM_RIGHT_OFFSET_IDX 1

#define RIGHT_OFFSET_ZERO 100

#define MY_SERIAL_OUT

#ifdef MY_SERIAL_OUT 
#define SERIALOUT(x) Serial.println(x);
#define SERIALOUT2(x,y) Serial.println(x,y);
#else
#define SERIALOUT(x)
#define SERIALOUT2(x,y)
#endif

IRrecv irrecv(IR_RECV_PIN);
decode_results results;
unsigned long g_lastIrCode;
uint8_t g_volIdx;
uint8_t g_rightOffset;


uint8_t logResValues[VOL_STEPS] = 
{
100,99,98,97,96,95,94,93,92,91,
90,89,88,87,86,85,84,83,82,81,
80,77,75,72,69,66,63,59,56,52,
47,43,38,32,27,21,15,9,0
};


void saveVolInEeprom()
{
  SERIALOUT("eeprom save");
  EEPROM.write(EEPROM_VOLUME_IDX, g_volIdx);
  EEPROM.write(EEPROM_RIGHT_OFFSET_IDX, g_rightOffset);
}

void loadVolFromEeprom()
{
  SERIALOUT("eeprom load");
  g_volIdx = EEPROM.read(EEPROM_VOLUME_IDX);
  g_rightOffset = EEPROM.read(EEPROM_RIGHT_OFFSET_IDX);

  if(g_volIdx == UNINTILAIZED_EEPROM)
    g_volIdx = DEFULT_VOL_STEP;
  if(g_rightOffset == EEPROM_RIGHT_OFFSET_IDX)
    g_rightOffset = RIGHT_OFFSET_ZERO;
}

uint8_t calculateRightOffsetIdx(uint8_t leftIdx)
{
  int offset = g_rightOffset - RIGHT_OFFSET_ZERO;
  int rightIdx = leftIdx - offset;
  
  if(rightIdx < 0) rightIdx = 0;
  if(rightIdx > VOL_STEPS) rightIdx = VOL_STEPS;
  
  return rightIdx;
}

void setVolume(uint8_t resistance, uint8_t pin)
{
    for(uint8_t idx = 0; idx < resistance; idx++)
    {
      digitalWrite(pin, LOW);
      delayMicroseconds(BURST_SLEEP_MUS);
      digitalWrite(pin, HIGH);
      delayMicroseconds(BURST_SLEEP_MUS);
    }
}

void setVolumeZero()
{
  SERIALOUT("setVolumeZero");
  digitalWrite(UD_PIN, HIGH); 
  setVolume(logResValues[0], INC_PIN_L);
  setVolume(logResValues[0], INC_PIN_R);
}

void setVolumeMax()
{
  SERIALOUT("setVolumeMax");
  g_volIdx = VOL_STEPS;
  digitalWrite(UD_PIN, LOW); 
  setVolume(logResValues[0], INC_PIN_L);
  setVolume(logResValues[0], INC_PIN_R);
}

void setVolume(uint8_t volIdx)
{
  SERIALOUT("setVolume");
  SERIALOUT2(g_volIdx, DEC);

  if(volIdx < VOL_STEPS)
  {
    uint8_t resistanceL = logResValues[volIdx];
    uint8_t resistanceR = logResValues[calculateRightOffsetIdx(volIdx)];
    setVolumeZero();
    
    digitalWrite(UD_PIN, LOW); 
    setVolume(100 - resistanceL, INC_PIN_L);
    setVolume(100 - resistanceR, INC_PIN_R);
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
  setVolumeMax();
}

void OnRecord()
{
  saveVolInEeprom();
}

void OnRightOffsetUp()
{
  g_rightOffset++;
  SERIALOUT("OnRightOffsetUp");
  SERIALOUT2(g_rightOffset, DEC);

}

void OnRightOffsetZero()
{
  g_rightOffset = RIGHT_OFFSET_ZERO;
  SERIALOUT("OnRightOffsetZero");
}

void OnRightOffsetDown()
{
  g_rightOffset--;
  SERIALOUT("OnRightOffsetDown");
  SERIALOUT2(g_rightOffset, DEC);
}

void setup()
{
  #ifdef SERIAL
  Serial.begin(9600);
  #endif
  irrecv.enableIRIn(); // Start the receiver

  pinMode(INC_PIN_L, OUTPUT);
  pinMode(INC_PIN_R, OUTPUT);
  pinMode(UD_PIN, OUTPUT);
  digitalWrite(INC_PIN_L, HIGH); 
  digitalWrite(INC_PIN_R, HIGH); 
  digitalWrite(UD_PIN, HIGH); 
  loadVolFromEeprom();
  setVolume(g_volIdx);
}

void loop() {
  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    
    switch (results.value)
    {
    case MY_MUTE:
      g_lastIrCode = 0;
      OnMute();
      break;
    case MY_VOLUP:
      g_lastIrCode = results.value;
      OnVolUp();
      break;
    case MY_VOLDWN:
      g_lastIrCode = results.value;
      OnVolDown();
      break;
    case MY_MAXVOL:  
      g_lastIrCode = 0;
      OnMax();
      break;
    case MY_REC:  
      g_lastIrCode = 0;
      OnRecord();
      break; 
    case MY_9:  
      g_lastIrCode = 0;
      OnRightOffsetUp();
      break;              
    case MY_7:  
      g_lastIrCode = 0;
      OnRightOffsetDown();
      break;
    case MY_8:  
      g_lastIrCode = 0;
      OnRightOffsetZero();
      break;                
    case MY_BURST:
      switch (g_lastIrCode)
      {
        case MY_VOLUP:
          OnVolUp();
          break;
        case MY_VOLDWN:
          OnVolDown();
          break;
      }
      break;
    default:
      break;
    }
    irrecv.resume(); // Receive the next value
  }
  delay(100);
}