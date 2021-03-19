#ifndef VALVES_H
#define VALVES_H

#define VALES_100FIX  201
#define VALES_0FIX  202
#define VALES_FIX  200

enum ValMode
{
  ValZero,
  Val100,
  Val75,
  Val50,
  Val25,
  ValAntiFreeze
};

enum ChanelUsedFlags
{
  OCFirst = 1,
  OCSecond = 2
};
enum OpenFlowStatus
{
  OFUnknown,
  OFOpen,
  OFClose,
  OFFixed
};

struct VAlarm
{
   const __FlashStringHelper* title;
   byte type;
   VAlarm();
   VAlarm(const __FlashStringHelper* tit,byte typ);
};

class Valves
{
    ValMode curentMode;
     long lastSeconds;
     long lastChangeSeconds;
     bool fixedMode;
    OpenFlowStatus openFlowStatus;
    int minutsPeriod;
    long valHighMinuts;
    VAlarm lastAlarm;
    ChanelUsedFlags lastChanelUsed;
    bool GetValue(long allSeconds);
    void DoChange(bool open);
    void CheckMaxHigh();
    void Alarm(const __FlashStringHelper* msg, byte type);
    void Init();
  public:
    VAlarm PeekAlarm();
    Valves();
    bool CanSetFixedMode();
    bool IsNO();
    void ResetFlowStatus();
    void Setup();
    void SetFixedMode();
    void SetMode(ValMode mode);
    void Adjust();
    void ResetTime();
    int PowerInMode();
    void SetMinutsPeriod(int i);
    void SetFlow(byte flow);
};

#endif
