#include "Config.h"
#include "Arduino.h"
#include <EEPROM.h>
#define EEPROM_DATA_OFFSET 100

Configuration Config::cfg;

void Config::Init()
{
  cfg.AntiFreezePeriod = 4 * 60;
  cfg.AntiFreezeOpened = 20;
  cfg.DelayFlowerMinutsPeriod = 10;
  cfg.ValvesConfig = NZ_XX;
  cfg.Alarms = ALARM_ALL;
  cfg.AntiFrezTemp = 1.5f;
  cfg.Go25Temp = 0.0f;
  cfg.Go50Temp = -2.0f;
  cfg.Go75Temp = -3.0f;
  cfg.Go100Temp = -4.0f;
  cfg.HisterSize = 0.3f;
  cfg.DelayFlower100Temp = 22.0f;
  cfg.ValMaxWorkMin = 30;
  cfg.Sensors = 0;
  cfg.DelayStop = 2.0;
  cfg.HalfAutomaticMode = false;
}

Configuration& Config::Get()
{
  return cfg;
}

void Config::Load()
{
  EEPROM.get(0, cfg);
  cfg.flowFactor = 7.29;
  //Init();//do czasu zapisania na EEPROM
  if (cfg.AntiFreezePeriod == -1) Init();
  if (cfg.ValMaxWorkMin < 3) cfg.ValMaxWorkMin = 3;
  if (isnan(cfg.T1OnSunDelta) || !(cfg.T1OnSunDelta > 3 && cfg.T1OnSunDelta < 15))  cfg.T1OnSunDelta = 5;
}

void Config::Save()
{
  EEPROM.put(0, cfg);
}

 long Config::GetTotalWater()
{
   long ret;
   EEPROM.get(EEPROM_DATA_OFFSET + 0, ret);
   return ret == -1 ? 0: ret;
   
}
void Config::SetTotalWater( long v)
{
 
  EEPROM.put(EEPROM_DATA_OFFSET + 0, v);
}
