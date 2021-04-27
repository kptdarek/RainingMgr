#ifndef CONFIG_H
#define CONFIG_H
//#define byte uint8_t

enum ConnValves
{
  XX_XX = 0,
  NO_NO = 1,
  NZ_NZ = 2,
  NO_XX = 3,
  XX_NO = 4 ,
  NZ_XX = 5,
  XX_NZ = 6
};

#define ALARM_FLOW     0x01
#define ALARM_SELENOID 0x02
#define ALARM_INFO    0x04
#define ALARM_TERMDEV 0x08
#define ALARM_SOUNDS 0x10

#define ALARM_ALL (ALARM_FLOW | ALARM_SELENOID | ALARM_INFO | ALARM_TERMDEV | ALARM_SOUNDS)
//#define ALARM_UNREAD  0x80

struct Configuration
{
  int AntiFreezePeriod;
  int AntiFreezeOpened;
  int DelayFlowerMinutsPeriod;
  float AntiFrezTemp;
  float Go25Temp;
  float Go50Temp;
  float Go75Temp;
  float Go100Temp;
  float HisterSize;  
  float DelayFlower100Temp;
  double flowFactor; //pulses per liter /60
  ConnValves ValvesConfig;
  unsigned char Alarms;
  int ValMaxWorkMin;
  int Sensors; //0 real 1-10 satic  11-> special
  float DelayStop;
  bool HalfAutomaticMode;
  float T1OnSunDelta;  
  bool AutoDisableValveIfError;
};

class Config
{
    static Configuration cfg;
  public:
    static Configuration& Get();
    static  long GetTotalWater();
    static void SetTotalWater( long v);
    static void Init();
    static void Load();
    static void Save();
};

#endif
