#include "Ui.h"
#include "Arduino.h"
#include "appcfg.h"
#include "Valves.h"
#include "Config.h"

#define SCREEN_SAVE_DELAY_MINUTS 5

#define MENU_LIVE_LEN 20  //*500ms
#define LITER_C char(0)
#define MENU_TOP_C char(1)
#define MENU_BOT_C char(2)
#define MENU_TOPBOT_C char(3)
#define DEGR_CELC_C char(4)
#define LITER_P_MIN_C char(5)
#define SND_C char(6)
#define NOSND_C char(7)

#define POWER_GUARD 255

#define NOTE_SUSTAIN 250
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_A5  880
#define NOTE_B5  988

extern double totalWater;
extern byte saveEepromCounter;


byte key1_pin = 5;
byte key2_pin = 4;
byte key3_pin = 7;
byte key4_pin = 6;
byte frost_raining_led = 14;
byte sun_protect_led = 15;
byte alarm_led = 13;
byte piezo_pin = 9;


byte CCRam[8];

#define MaxCC 8
const PROGMEM byte CCFlash[MaxCC][8] =
{
//{0x1f, 0x1f, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe}, /* ram up */
{ //liter symbol
  B00010,
  B00010,
  B00010,
  B00010,
  B00010,
  B00010,
  B00001
},
{ //top
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00100,
  B00100,
},
{ //bot
  B00100,
  B00100,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100,
},
{ //topBot
  B00100,
  B01110,
  B10101,
  B00100,
  B10101,
  B01110,
  B00100,
},
{ //degCel
  B01000,
  B10100,
  B01000,
  B00011,
  B00100,
  B00100,
  B00011,
},
{ //lnm
  B00100,
  B00100,
  B00100,
  B00000,
  B01111,
  B10101,
  B10101,
},
{ //snd
  B00001,
  B00011,
  B11101,
  B10001,
  B11101,
  B00011,
  B00001
},

{ //no snd
  B01001,
  B01011,
  B11101,
  B10101,
  B11101,
  B00111,
  B00101
}
};

UIMgr::UIMgr(): lcd(0x27, 16, 2)
{
  lastTime = 0;
  lastFlowA = 0;

  alarmShowMode = false;
  antifrezModeLedOn = false;
  lastTimeBackLightOn = 0;

  ResetAlarms();
  ResetHistory();
  ResetTempHisotry();
}

void  UIMgr::ResetAlarms()
{
  alarmIndex = 0;
  for (int i = 0 ; i < ALARM_SIZE; i++)
  {
    Alarms[i].msg = NULL;
  }
}

void  UIMgr::ResetHistory()
{
  hisotryIndex = 0;
  for (int i = 0 ; i < HISTORY_SIZE; i++)
  {
    history[i].time = 0;
    history[i].t1 = 0;
    history[i].t2 = 0;
    history[i].flowA = 0;
    history[i].PowerPrec = POWER_GUARD;
  }
}

void UIMgr::ResetTempHisotry()
{
  for (int i = 0; i < 10; i++)
  {
    Temp1MinutHisotory[i][0] = -200.0;
    Temp1MinutHisotory[i][1] = -200.0;
  }
}

void UIMgr::Setup(Valves* v)
{
  lcd.init(); // initialize the lcd
  for (int i = 0 ; i < MaxCC ; i++)
  {
    memcpy_P(CCRam, &CCFlash[i][0], 8);
    lcd.createChar(i, CCRam);
  }

  lcd.backlight(); backlightOn = true;

  pinMode(key1_pin, INPUT_PULLUP);
  pinMode(key2_pin, INPUT_PULLUP);
  pinMode(key3_pin, INPUT_PULLUP);
  pinMode(key4_pin, INPUT_PULLUP);

  pinMode( frost_raining_led, OUTPUT);
  pinMode( sun_protect_led, OUTPUT);
  pinMode( alarm_led, OUTPUT);
  Invalidate();
  valves = v;
}

void UIMgr::SetTemperature(double t, Termometr instance, bool showTrend)
{
  char trend = ' ';
  if (showTrend)
  {
    double lastT = Temp1MinutHisotory[(lastTime + 1) % 10][ instance == TopT ? 0 : 1];

    if (lastT > -200.0 && lastT + 0.2 < t) trend = MENU_TOP_C;
    if (lastT > -200.0 && lastT - 0.2 >  t) trend = MENU_BOT_C;
  }

  if (!alarmShowMode)
  {
    lcd.setCursor(10, instance == TopT ? 0 : 1);
    if (t > - 100.0)
    {
      lcd.print(trend);
      lcd.print(t, 1); lcd.print(DEGR_CELC_C); lcd.print(F(" "));
    } else
    {
      lcd.print(F(" -err-"));
    }
  }
  if ( instance == TopT ) lastT1 = t;
  else lastT2 = t;

  if (lastTime > 0)
  {
    Temp1MinutHisotory[lastTime % 10][ instance == TopT ? 0 : 1] = t;
  }

}

void UIMgr::SetTime(unsigned long timeFrom24,  bool saveHisotry )
{

  if (saveHisotry && timeFrom24 % 30 == 0) SaveHistoryItem(timeFrom24);
  int minuts = timeFrom24;
  if (!alarmShowMode)
  {
    PrintTime(minuts);
  }

  if (saveHisotry)
  {
    lastTime = timeFrom24;
    checkScreenSave();
  }
}

void UIMgr::PrintTime(int minuts )
{

  lcd.setCursor(0, 0);
  int h = (minuts / 60) % 24;
  if (h < 10)  lcd.print(F("0"));
  lcd.print(h);
  int m = minuts % 60;
  lcd.print(m < 10 ? F(":0") : F(":"));
  lcd.print(m);
}

void UIMgr::SaveHistoryItem(unsigned long timeFrom24)
{
  if (lastFlowA == 255) return;  //invalid
  unsigned long lst =  history[historyMinus(hisotryIndex)].time;
  if (lst != 0 && lst == timeFrom24) return;
  history[hisotryIndex].time = timeFrom24;
  history[hisotryIndex].t1 = lastT1;
  history[hisotryIndex].t2 = lastT2;
  history[hisotryIndex].flowA = lastFlowA;
  history[hisotryIndex].PowerPrec = lastPowerPrecent;

#if SERIAL_PRINT
  Serial.print(F("SaveHistoryI:"));
  Serial.print(hisotryIndex);
  Serial.print(F("Flow:"));
  Serial.println(lastFlowA);
#endif

  hisotryIndex =  hisotryPlus(hisotryIndex );
}

void UIMgr::SetAvargeFlowX10(byte value)
{
  if ( value != lastFlowA)
  {
    lastFlowA = value;
    if (!alarmShowMode)
    {
      lcd.setCursor(6, 0);
      lcd.print(F("   "));
      lcd.setCursor(6, 0);
      if (value >= 10 || value == 0)
      {
        lcd.print(value / 10);
      } else {
        lcd.print(F("."));
        lcd.print(value);
      }
      lcd.print(LITER_P_MIN_C);
    }
  }
}

void UIMgr::SetCurrentFlow(byte value)
{
  if ( value != lastFlowC)
  {
    lastFlowC = value;
    if (!alarmShowMode)
    {
      lcd.setCursor(6, 1);
      lcd.print(F("   "));
      lcd.setCursor(6, 1);
      lcd.print(value);
      lcd.print(LITER_P_MIN_C);
    }
  }
}


void UIMgr::SetPowerPrecent(byte value, bool halfAuto)
{
  if (value != lastPowerPrecent)
  {
    lastPowerPrecent = value;
    lcd.setCursor(0, 1);
    lcd.print(F("    "));
    lcd.setCursor(0, 1);
    if (value > VALES_FIX) lcd.print(F("#"));
    byte v = value % 101;
    if (halfAuto)
    {
      switch (v)
      {
        case 0:
          lcd.print(F("ZERO"));
          break;
        case 100:
          lcd.print(F("FULL"));
          break;
        default:
          lcd.print(F("["));
          lcd.print(v);
          lcd.print(F("%]"));
      };
    } else {
      lcd.print(v);
      lcd.print(F("%"));
    }
  }
}

Task2Do UIMgr::ProcessKeys()
{
  KeybordKeys keys = GetPressed();
  if (keys & TopKey) return MinusTask;
  if (keys & BottomKey) return PlusTask;
  if (keys & EnterKey) return MainMenu;
  if (keys & BackKey) return Back;

  return NoneTask;
}

KeybordKeys  UIMgr::GetPressed()
{
  KeybordKeys result = NoneKey;
  if (!digitalRead(key1_pin)) result = BackKey;
  if (!digitalRead(key2_pin))  result = TopKey;
  if (!digitalRead(key3_pin)) result = BottomKey;
  if (!digitalRead(key4_pin)) result = EnterKey;

  if (result != NoneKey && result != TopKey) PingScreenSave();
  return result;
}

void UIMgr::Print(const __FlashStringHelper* txt, byte line)
{
  lcd.clear();
  lcd.setCursor(0, line);
  lcd.print(txt);
}

void UIMgr::Print(int i)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(i);
}
void UIMgr::Cls()
{
  lcd.clear();
}

void UIMgr::WaitKeyUp()
{
  do
  {
    delay(100);
  } while (GetPressed() != NoneKey);
}

int UIMgr::Menu(const __FlashStringHelper* title, const __FlashStringHelper* items[], int cnt, int currIndex)
{
  int curr = currIndex;
  int live = MENU_LIVE_LEN;
  bool invalidate = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(title);
  WaitKeyUp();

  KeybordKeys key = NoneKey;
  do
  {
    if (invalidate)
    {
      live = MENU_LIVE_LEN;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(title);

      char caseChar = ' ';
      if (curr > 0 && curr < cnt - 1 ) caseChar = MENU_TOPBOT_C;
      else if (curr > 0) caseChar = MENU_TOP_C;
      else if (curr < cnt - 1 ) caseChar = MENU_BOT_C;
      lcd.setCursor(0, 1);
      lcd.print(caseChar);

      lcd.setCursor(3, 1);
      lcd.print(items[curr]);
      invalidate = false;
    }
    delay(500);
    valves->Adjust();
    key = GetPressed();
    switch (key)
    {
      case TopKey:
        if (curr > 0)
        {
          curr--;
          invalidate = true;
        }
        break;
      case BottomKey:
        if (curr < cnt - 1)
        {
          curr++;
          invalidate = true;
        }
        break;
      case EnterKey:
        Invalidate();
        return curr;
    }
  } while (key != BackKey  && live--);

  Invalidate();
  return -1;
}

int UIMgr::SetValue(const __FlashStringHelper* title, int old, int change)
{
  int live = MENU_LIVE_LEN;
  int curr = old;
  bool invalidate = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(title);
  WaitKeyUp();

  KeybordKeys key = NoneKey;
  do
  {
    if (invalidate)
    {
      live = MENU_LIVE_LEN;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(title);

      lcd.setCursor(2, 1);
      lcd.print(old);
      lcd.print(F("->"));
      lcd.print(curr);
      invalidate = false;
    }
    delay(500);
    valves->Adjust();
    key = GetPressed();
    switch (key)
    {
      case TopKey:
        curr -= change;
        invalidate = true;
        break;
      case BottomKey:
        curr += change;
        invalidate = true;
        break;
      case EnterKey:
        Invalidate();
        return curr;
    }
  } while (key != BackKey  && live--);

  Invalidate();
  return old;
}

float  UIMgr::SetValue(const __FlashStringHelper* title, float old, float change)
{
  int live = MENU_LIVE_LEN;
  float curr = old;
  bool invalidate = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(title);
  WaitKeyUp();

  KeybordKeys key = NoneKey;
  do
  {
    if (invalidate)
    {
      live = MENU_LIVE_LEN;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(title);

      lcd.setCursor(2, 1);
      lcd.print(old);
      lcd.print(F("->"));
      lcd.print(curr);
      invalidate = false;
    }
    delay(500);
    valves->Adjust();
    key = GetPressed();
    switch (key)
    {
      case TopKey:
        curr -= change;
        invalidate = true;
        break;
      case BottomKey:
        curr += change;
        invalidate = true;
        break;
      case EnterKey:
        Invalidate();
        return curr;
    }
  } while (key != BackKey  && live--);

  Invalidate();
  return old;
}

bool  UIMgr::SetValue(const __FlashStringHelper* title, bool old)
{
  int live = MENU_LIVE_LEN;
  bool curr = old;
  bool invalidate = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(title);
  WaitKeyUp();

  KeybordKeys key = NoneKey;
  do
  {
    if (invalidate)
    {
      live = MENU_LIVE_LEN;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(title);

      lcd.setCursor(2, 1);
      lcd.print(old ? F("TAK") : F("NIE"));
      lcd.print(F("->"));
      lcd.print(curr ? F("TAK") : F("NIE"));
      invalidate = false;
    }
    delay(500);
    valves->Adjust();
    key = GetPressed();
    switch (key)
    {
      case TopKey:
      case BottomKey:
        curr = ! curr;
        invalidate = true;
        break;
      case EnterKey:
        Invalidate();
        return curr;
    }
  } while (key != BackKey  && live--);

  Invalidate();
  return old;
}


bool UIMgr::YesNo(const __FlashStringHelper* title)
{
  int live = MENU_LIVE_LEN;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(title);
  lcd.setCursor(0, 1);
  lcd.print(F("NIE / TAK ?"));
  WaitKeyUp();
  KeybordKeys key = NoneKey;
  do
  {
    delay(500);
    valves->Adjust();
    key = GetPressed();
    if (key == EnterKey)
    {
      Invalidate();
      return true;
    }
  } while (key != BackKey  && live--);

  Invalidate();
  return false;
}

void UIMgr::History()
{
  byte index = historyMinus(hisotryIndex);
  if (history[index].PowerPrec == POWER_GUARD) return;
  byte minInx1 =  findMinHistory(index, true);
  byte minInx2 =  findMinHistory(index, false);

  History(index, minInx1, minInx2);
  WaitKeyUp();

  KeybordKeys key = NoneKey;
  do
  {
    delay(500);
    valves->Adjust();
    key = GetPressed();
    byte i = index;
    switch (key)
    {
      case TopKey:
        i = historyMinus(index);
        break;
      case BottomKey:
        i = hisotryPlus(index);
        break;
      case EnterKey:
        i = (i == minInx2 ? minInx1 : minInx2);
        break;
    }
    if (i != index && history[i].PowerPrec != POWER_GUARD)
    {
      index = i;
      History(index, minInx1, minInx2);
    }

  } while (key != BackKey);

  Invalidate();
}

byte UIMgr::findMinHistory(byte ind, bool top)
{
  byte minIncex = ind;
  byte  index = ind;
  for (int i = 0 ; i < HISTORY_SIZE; i++)
  {
    if (history[index].PowerPrec == POWER_GUARD) break;
    index = historyMinus(index);
    if (history[index].PowerPrec == POWER_GUARD) break;

    if ((top ? history[index].t1 : history[index].t2) < (top ? history[minIncex].t1 : history[minIncex].t2 )) minIncex = index;
  }
  return minIncex;
}

void UIMgr::CheckAlarms(bool showLastActive)
{
  byte index = alarmIndex;

  if (showLastActive)
  {
    index = GetlastActiveAlarm();
    if (index == 255) index = alarmIndex;
  }

  bool invalidate = true;
  WaitKeyUp();

  KeybordKeys key = NoneKey;
  do
  {
    if (invalidate)
    {
      Alarm(index);
      char caseChar = ' ';
      if (Alarms[alarmMinus(index)].msg != NULL && Alarms[alarmPlus(index)].msg != NULL  ) caseChar = MENU_TOPBOT_C;
      else if (Alarms[alarmMinus(index)].msg != NULL) caseChar = MENU_TOP_C;
      else if (Alarms[alarmPlus(index)].msg != NULL) caseChar = MENU_BOT_C;
      lcd.setCursor(15, 0);
      lcd.print(caseChar);

      invalidate = false;
    }
    delay(500);
    valves->Adjust();
    key = GetPressed();
    byte  last = index;
    switch (key)
    {
      case TopKey:
        index = alarmMinus(index);
        break;
      case BottomKey:
        index = alarmPlus(index);
        break;
    }
    if (Alarms[index].msg == NULL) index = last;
    if (index != last)
    {
      invalidate =  true;
    }

    Alarms[index].flags = Alarms[index].flags & ~ALARM_UNREAD;
#if SERIAL_PRINT
    Serial.println(Alarms[index].flags);
#endif
  } while (key != BackKey  && key != EnterKey);

  Invalidate();
}

void UIMgr::Alarm(byte index)
{
  Print(Alarms[index].msg, 1);
  PrintTime(Alarms[index].time);
  lcd.setCursor(7, 0);
  lcd.print(F("[")); lcd.print(Alarms[index].cnt); lcd.print(F("]"));
}

void  UIMgr::History(byte index, byte minInx1, byte minInx2)
{
  Invalidate();
  double t1 = history[index].t1;
  double t2 = history[index].t2;
  SetTime(history[index].time, false);
  SetTemperature(t1, TopT, false);
  SetTemperature(t2, BottomT, false);
  SetAvargeFlowX10(history[index].flowA);
  SetPowerPrecent(history[index].PowerPrec);

  byte prev = historyMinus(index);
  bool prevExist = history[prev].PowerPrec != POWER_GUARD;
  char caseChar = ' ';
  if (prevExist && history[hisotryPlus(index)].PowerPrec != POWER_GUARD ) caseChar = MENU_TOPBOT_C;
  else if (prevExist) caseChar = MENU_TOP_C;
  else if (history[hisotryPlus(index)].PowerPrec != POWER_GUARD) caseChar = MENU_BOT_C;
  lcd.setCursor(5, 1);
  lcd.print(F("HST"));
  lcd.print(caseChar);

  if (prevExist)
  {

    double pt1 = history[prev].t1;
    double pt2 = history[prev].t1;

    if (index == minInx1)
    {
      lcd.setCursor(10, 0);
      lcd.print(F("!"));
    } else {
      if (pt1 != t1)
      {
        lcd.setCursor(10, 0);
        lcd.print(pt1 < t1 ? MENU_TOP_C : MENU_BOT_C);
      }
    }

    if (index == minInx2)
    {
      lcd.setCursor(10, 1);
      lcd.print(F("!"));
    } else {
      if (pt2 != t2)
      {
        lcd.setCursor(10, 1);
        lcd.print(pt2 < t2 ? MENU_TOP_C : MENU_BOT_C);
      }
    }
  }
}

void  UIMgr::Invalidate()
{
  lastFlowC = 255;
  lastFlowA = 255;
  lastPowerPrecent = 255;
  lcd.clear();
}

#if DEBUG
void UIMgr::TestLeds()
{
  Print(F("Test LED-ow"));
  static bool state_1 = false;
  for (int i = 0; i < 20; i++)
  {
    delay(500);
    state_1 = !state_1;

    digitalWrite(frost_raining_led, state_1 ? HIGH : LOW );
    digitalWrite(sun_protect_led, state_1 ? LOW : HIGH );
    digitalWrite(alarm_led, state_1 ? LOW : HIGH );
  }

  digitalWrite(frost_raining_led, LOW );
  digitalWrite(sun_protect_led, LOW);
  digitalWrite(alarm_led, LOW  );
  Cls();
}
#endif

void UIMgr::ShowMode(WorkMode mode)
{
  antifrezModeLedOn = mode == WmAntiFreez || mode == WmAntiFreezHalfAuto;
  digitalWrite(frost_raining_led, antifrezModeLedOn ? HIGH : LOW );

  digitalWrite(sun_protect_led,  mode == WmDeleayFolowering ? HIGH : LOW);
}

void UIMgr::Pulse(bool v, WorkMode mode)
{
  if (mode == WmAntiFreez || mode == WmAntiFreezHalfAuto)
  {
    if (lastPowerPrecent % 101 != 0)
    {
      if (!antifrezModeLedOn) ShowMode(mode); //zapalamy zgaśnietą.
    } else {
      if (antifrezModeLedOn != v)
      {
        digitalWrite(frost_raining_led, v ? HIGH : LOW );
        antifrezModeLedOn = v;
      }
    }
  }
}

byte UIMgr::historyMinus(byte index)
{
  return (index + HISTORY_SIZE - 1 ) % HISTORY_SIZE;
}

byte UIMgr::hisotryPlus(byte index)
{
  return (index + 1) % HISTORY_SIZE;
}


byte UIMgr::GetlastActiveAlarm()
{
  byte index = alarmIndex;
  for (int i = 0 ; i < ALARM_SIZE; i++)
  {
    if (Alarms[index].msg == NULL) return 255;
    if (Alarms[index].flags & ALARM_UNREAD) return index;
    index = alarmMinus(index);
  }
  return 255;
}

void UIMgr::ShowLastAlarm(bool show)
{
  bool on = (IsActiveAlarm() && show);
  if (on != alarmShowMode)
  {
    BacklightOn();
    alarmShowMode = on;
    lcd.clear();
    if (on)
    {
      if (Config::Get().Alarms & ALARM_SOUNDS)
      {
        tone(piezo_pin, 1000, 500);
      }

      byte index = GetlastActiveAlarm();
      if (index != 255) Alarm(index);
    } else {
      Invalidate();
    }
    digitalWrite(alarm_led, on ? HIGH : LOW);
  }
}

bool UIMgr::IsActiveAlarm()
{
  for (int i = 0 ; i < ALARM_SIZE; i++)
  {
    if (Alarms[i].msg  && (Alarms[i].flags & ALARM_UNREAD)) return true;
  }
  return false;
}

bool UIMgr::IsAnyAlarm()
{
  for (int i = 0 ; i < ALARM_SIZE; i++)
  {
    if (Alarms[i].msg) return true;
  }
  return false;
}

byte  UIMgr::currAlarmIndex(const __FlashStringHelper* title)
{
  for (int i = 0 ; i < ALARM_SIZE; i++)
  {
    if (Alarms[i].msg  == title) return i;
  }
  return 255;
}

void UIMgr::AddAlarm(const __FlashStringHelper* title, byte flags)
{
  Configuration& cfg = Config::Get();
  if (!(cfg.Alarms & flags)) return;

  byte exist = currAlarmIndex(title);
  if (exist != 255)
  {
    if (Alarms[exist].cnt < 99)
    {
      Alarms[exist].cnt ++;
    }
    Alarms[exist].flags = flags | ALARM_UNREAD;
    return;

  }
  alarmIndex = alarmPlus(alarmIndex);
  Alarms[alarmIndex].msg = title;
  Alarms[alarmIndex].flags = flags | ALARM_UNREAD;
  Alarms[alarmIndex].time = lastTime;
  Alarms[alarmIndex].cnt = 1;

}


byte UIMgr::alarmPlus(byte index)
{
  return (index + 1) % ALARM_SIZE;
}

byte UIMgr::alarmMinus(byte index)
{
  return (index + ALARM_SIZE - 1 ) % ALARM_SIZE;
}

bool UIMgr::Beep()
{
  tone(piezo_pin, 3000, 50);
}


// XX_XX = 0, NO_NO = 1,NZ_NZ = 2, NO_XX = 3, XX_NO = 4 , NZ_XX = 5, XX_NZ = 6
void UIMgr::ShowStatus()
{
  const __FlashStringHelper* valves[] = {F("----"), F("NoNo"), F("NzNz"), F("No--"), F("--No"), F("Nz--"), F("--Nz")};
  Configuration& cfg = Config::Get();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(valves[cfg.ValvesConfig]);
#if DEBUG
  lcd.print(F(" D"));
#else
  lcd.print(F(" R"));
#endif

  lcd.print(F(">"));
  lcd.print(cfg.FlowAlarmThreshold);
  lcd.print(LITER_P_MIN_C);
  if (cfg.AutoDisableValveIfError) lcd.print(F("*"));
  lcd.print(F(" "));
  lcd.print(cfg.Alarms & ALARM_SOUNDS ? SND_C : NOSND_C);
  lcd.print(cfg.Alarms & ALARM_INFO ? F("I") : F("-"));
  lcd.print(cfg.Alarms & ALARM_FLOW ? F("P") : F("-"));
  lcd.print(cfg.Alarms & ALARM_SELENOID ? F("C") : F("-"));
  lcd.print(cfg.Alarms & ALARM_TERMDEV ? F("T") : F("-"));


  lcd.setCursor(0, 1);
  lcd.print((millis() EXTRA_MILLIS) / 1000 / 60 / 60);
  lcd.print(F("h"));

  lcd.print(F(" "));
  lcd.print(Config::GetTotalWater());
  lcd.print(F("/"));
  lcd.print(totalWater, 0);
  lcd.print(LITER_C);
  //lcd.print((char) ('A'+saveEepromCounter));

  delay(3000);
  WaitKeyUp();
  Invalidate();
}

void UIMgr::ShowTempCfgStatus()
{
  Configuration& cfg = Config::Get();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("C<"));
  lcd.print(cfg.ValMaxWorkMin); //4
  lcd.print(F(" <"));//2
  lcd.print(abs(cfg.AntiFrezTemp), 1);
  lcd.print(F(","));
  lcd.print(abs(cfg.DelayStop), 1);
  lcd.print(F(">"));
  lcd.print(DEGR_CELC_C);

  lcd.setCursor(0, 1);
  lcd.print(abs(cfg.Go25Temp), 1);
  lcd.print(F("<"));//4
  lcd.print(abs(cfg.Go50Temp), 1);
  lcd.print(F("<"));//4
  lcd.print(abs(cfg.Go75Temp), 1);
  lcd.print(F("<"));//4
  lcd.print(abs(cfg.Go100Temp), 1);
  lcd.print(DEGR_CELC_C);//4

  delay(3000);
  WaitKeyUp();
  Invalidate();
}

void UIMgr::BacklightOff()
{
  lcd.noBacklight(); backlightOn = false;
  lastTimeBackLightOn = 1; //wlaczamy wygaszacz
}
bool UIMgr::IsBacklightOn()
{
  return backlightOn;
}

void UIMgr::BacklightOn()//wylaczamy wygaszac
{
  lastTimeBackLightOn = 0;
  if (backlightOn) return;
  lcd.backlight(); backlightOn = true;
}

void UIMgr::checkScreenSave()
{
  if (lastTimeBackLightOn == 0 || !backlightOn) return;
  if (lastTime - lastTimeBackLightOn >= SCREEN_SAVE_DELAY_MINUTS)
  {
    lcd.noBacklight(); backlightOn = false;
  }
}

void UIMgr::PingScreenSave()
{
  if (lastTimeBackLightOn == 0)
  {
    BacklightOn();
  } else {
    if (!backlightOn)
    {
      lcd.backlight();
      backlightOn = true;
    }
    lastTimeBackLightOn = lastTime;
  }
}

void UIMgr::SuccessSnd(unsigned long timeFrom24, int times)
{
int h = (timeFrom24 / 60) % 24;
if ( h < 6 || h > 21) return;
for(int i=0; i < times; i++)
{
  if (i) delay(1000);
tone(piezo_pin,NOTE_A5 );
delay(NOTE_SUSTAIN);
tone(piezo_pin,NOTE_B5);
delay(NOTE_SUSTAIN);
tone(piezo_pin,NOTE_C5);
delay(NOTE_SUSTAIN);
tone(piezo_pin,NOTE_B5);
delay(NOTE_SUSTAIN);
tone(piezo_pin,NOTE_C5);
delay(NOTE_SUSTAIN);
tone(piezo_pin,NOTE_D5);
delay(NOTE_SUSTAIN);
tone(piezo_pin,NOTE_C5);
delay(NOTE_SUSTAIN);
tone(piezo_pin,NOTE_D5);
delay(NOTE_SUSTAIN);
tone(piezo_pin,NOTE_E5);
delay(NOTE_SUSTAIN);
tone(piezo_pin,NOTE_D5);
delay(NOTE_SUSTAIN);
tone(piezo_pin,NOTE_E5);
delay(NOTE_SUSTAIN);
tone(piezo_pin,NOTE_E5);
delay(NOTE_SUSTAIN);
noTone(piezo_pin);

}
}
