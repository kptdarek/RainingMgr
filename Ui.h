#ifndef UI_H
#define UI_H
#include <LiquidCrystal_I2C.h>
#include "appcfg.h"
#define byte uint8_t

#if DEBUG
#define HISTORY_SIZE  24  //30 min
#else
#define HISTORY_SIZE  48  //30 min
#endif

class Valves;

#define ALARM_SIZE 5

//#define ALARM_CRITICAL 0x01
//#define ALARM_WARNING 0x02
#define ALARM_UNREAD  0x80

enum WorkMode
{
  WmNone,
  WmDeleayFolowering,
  WmAntiFreez,
  WmAntiFreezHalfAuto  
};

enum Termometr
{
  TopT,
  BottomT
};

enum Task2Do
{
  NoneTask,
  Time15MinPlus,
  Time15MinMinus,
  MainMenu,
  Back
};

enum KeybordKeys
{
  NoneKey = 0,
  BackKey = 1,
  TopKey = 2,
  BottomKey = 4,
  EnterKey = 8
};

struct HistoryItem
{
  double t1;
  double t2;
  unsigned long time;
  byte flowA;
  byte PowerPrec;
};

struct AlarmItem
{
  const __FlashStringHelper* msg;
  byte cnt;
  byte flags;
  unsigned long time;
};

class UIMgr
{
    byte hisotryIndex;
    LiquidCrystal_I2C lcd;
    byte lastFlowC;
    byte lastFlowA;
    byte lastPowerPrecent;
    unsigned long lastTime;

    double lastT1;
    double lastT2;
    HistoryItem history[HISTORY_SIZE];
    double Temp1MinutHisotory[10][2];
    AlarmItem Alarms[ALARM_SIZE];
    byte alarmIndex;
    bool alarmShowMode;
    bool antifrezModeLedOn;
    Valves* valves;
  public:
    UIMgr();
    void Setup(Valves* v);
    void SetTemperature(double t, Termometr instance, bool showTrend = true);
    void SetTime(unsigned long timeFrom24, bool saveHisotry = true);    
    void SetAvargeFlowX10(byte value);    
    void SetCurrentFlow(byte value);    
    void SetPowerPrecent(byte value, bool halfAuto = false);
    int Menu(const __FlashStringHelper* title, const __FlashStringHelper* items[], int cnt, int currIndex = 0);
    int SetValue(const __FlashStringHelper* title, int old, int change = 1);
    bool SetValue(const __FlashStringHelper* title, bool old);
    bool YesNo(const __FlashStringHelper* title);
    float SetValue(const __FlashStringHelper* title, float old, float change = 0.1f);
    void Pulse(bool v, WorkMode mode);
    void Print(const __FlashStringHelper* title, byte line = 0);
    void Print(int i); 
    void ShowStatus();
    void ShowTempCfgStatus();
    void Cls();
    void ShowMode(WorkMode mode);
    void  ResetHistory();
    void ResetAlarms();
    void History();
    void CheckAlarms(bool showLastActive = false);
#if DEBUG    
    void TestLeds();
#endif    
    void AddAlarm(const __FlashStringHelper* title, byte flags);
    void ShowLastAlarm(bool show);
    Task2Do ProcessKeys();
    void Invalidate();
    bool IsActiveAlarm();
    bool IsAnyAlarm();
    bool Beep();
    void WaitKeyUp();
  protected:
    KeybordKeys GetPressed();

    void SaveHistoryItem(unsigned long timeFrom24);
    void History(byte index, byte minInx1,byte minInx2);
    void Alarm(byte index);


  private:
    byte findMinHistory(byte ind, bool top);
    byte hisotryPlus(byte index);
    byte historyMinus(byte index);
    void ResetTempHisotry();
    byte alarmPlus(byte index);
    byte alarmMinus(byte index);
    void PrintTime(int minuts );
    byte currAlarmIndex(const __FlashStringHelper* title);
    byte GetlastActiveAlarm();
};


#endif
