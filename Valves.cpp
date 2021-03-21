#include "Arduino.h"
#include "Valves.h"
#include "Config.h"
#include "appcfg.h"

int first_chanel = 17;
int second_chanel = 16;


VAlarm::VAlarm()
{
title = NULL;
}

VAlarm::VAlarm(const __FlashStringHelper* tit,byte typ)
{
  title = tit;
  type = typ;
}


Valves::Valves()
{
  Init();
}

void Valves::Init()
{
  curentMode = ValZero;
  lastSeconds = -1;
  lastChangeSeconds = -1;
  openFlowStatus = OFUnknown;
  minutsPeriod = 1;
  lastChanelUsed = OCSecond;
  valHighMinuts = -1;  
  fixedMode =  false;
}
void Valves::SetMinutsPeriod(int i)
{
  minutsPeriod = i;
}

bool Valves::GetValue(long allSeconds)
{
  if (curentMode == ValAntiFreeze)
  {
    Configuration& cfg = Config::Get();
    long period = allSeconds % cfg.AntiFreezePeriod;
    return  period < cfg.AntiFreezeOpened;
  }

  long second = allSeconds % (60 * minutsPeriod);
  switch (curentMode)
  {
    case ValZero:
      return false;
    case Val100:
      return true;
    case Val50:
      return second < (30 * minutsPeriod);
    case Val75:
      return second < (45 * minutsPeriod);
    case Val25:
      return second < (15 * minutsPeriod);
  };

  return true;
}

bool Valves::CanSetFixedMode()
{
  if (fixedMode) return false;
  if (IsNO())
  {
    if (curentMode == ValZero ) return true;;
  } else
  {
    if (curentMode == Val100) return true;
  }
  return false;
}

void Valves::SetFixedMode()
{
  fixedMode = true;
}

void Valves::SetMode(ValMode mode)
{
  if (curentMode != mode)
  {
    if (IsNO())
    {
      if (mode != ValZero) fixedMode =  false;
    } else
    {
      if (mode != Val100) fixedMode =  false;
    }
    curentMode = mode;
  }

}

int  Valves::PowerInMode()// procentowo wydatek
{
  if (curentMode == ValAntiFreeze)
  {
    Configuration& cfg = Config::Get();
    return   cfg.AntiFreezeOpened * 100 / cfg.AntiFreezePeriod ;
  }

  switch (curentMode)
  {
    case ValZero:
      return fixedMode ? VALES_0FIX : 0;
    case Val100:
      return fixedMode ? VALES_100FIX : 100;
    case Val50:
      return 50;
    case Val75:
      return 75;
    case Val25:
      return 25;
  };
}

bool Valves::IsNO()
{
  Configuration& cfg = Config::Get();
  return (cfg.ValvesConfig == NO_NO ||  cfg.ValvesConfig == NO_XX || cfg.ValvesConfig == XX_NO);
}

void Valves::Setup() {

  pinMode(first_chanel, OUTPUT);
  pinMode(second_chanel, OUTPUT);
  //check fixed mode
  if (IsNO() &&  curentMode == ValZero)
  {
    fixedMode = true;
  }
}

void Valves::ResetTime()
{
  lastSeconds = -1;
}

void  Valves::SetFlow(byte flow)
{
   long seconds = (millis() EXTRA_MILLIS) / 1000l;

  if (fixedMode && IsNO() && flow > 0)
  {
    fixedMode = false;
  }

  if (lastChangeSeconds != -1 && seconds - lastChangeSeconds > 2)
  {
    lastChangeSeconds = seconds + 67;// sprawdzamy znów za 67 s czy zgada się przepływ
    if (openFlowStatus == OFOpen && flow < 1) Alarm(F("Brak przeplywu"), ALARM_FLOW);
    if (openFlowStatus == OFClose && flow > 0) Alarm(F("Zbedny przeplyw"), ALARM_FLOW);
  }
}

void Valves::Adjust()
{
  unsigned long seconds = (millis() EXTRA_MILLIS) / 1000 ;
  if (lastSeconds == -1)
  {
    lastSeconds = seconds;
    return;
  }

  if (seconds == lastSeconds) return;

   lastSeconds = seconds;

  if (fixedMode)// fixed się wyłaczy gdy tylko zmieni się tryb!
  {
    if (openFlowStatus != OFFixed)
    {
      openFlowStatus = OFFixed;
      digitalWrite(first_chanel, LOW );
      digitalWrite(second_chanel, LOW );
      valHighMinuts = -1;
    }
    return;
  }
  CheckMaxHigh();

  bool isOpen = GetValue(seconds);
  if (openFlowStatus != (isOpen ? OFOpen : OFClose))
  {
    lastChangeSeconds = lastSeconds;

    openFlowStatus = isOpen ? OFOpen : OFClose;

    DoChange(isOpen);

  }
}

void Valves::ResetFlowStatus()
{
  Init();
  if (IsNO() &&  curentMode == ValZero)
  {
    fixedMode = true;
  }
}

void Valves::CheckMaxHigh()
{
  if (valHighMinuts == -1) return;
  Configuration& cfg = Config::Get();
  if (lastSeconds - valHighMinuts * 60 > cfg.ValMaxWorkMin * 60)
  {
    if (cfg.ValvesConfig == NO_NO && openFlowStatus == OFClose)
    {
      DoChange(false);
    } else {
      valHighMinuts = lastSeconds / 60;
      Alarm(F("Goraca cewka"), ALARM_SELENOID);
    }
  }
}

void Valves::Alarm(const __FlashStringHelper* msg, byte type)
{
  lastAlarm.title = msg;
  lastAlarm.type = type;
}

VAlarm Valves::PeekAlarm()
{
  const __FlashStringHelper* ret = lastAlarm.title;
  lastAlarm.title = NULL;
  
  return VAlarm(ret,lastAlarm.type);
}


void Valves::DoChange(bool open)
{
  Configuration& cfg = Config::Get();
  if (cfg.ValvesConfig == XX_XX)
  {
    digitalWrite(first_chanel, LOW );
    digitalWrite(second_chanel, LOW );
    valHighMinuts = -1;
    return;
  }

  if (cfg.ValvesConfig == NZ_XX || cfg.ValvesConfig == XX_NZ)
  {
    valHighMinuts = open ? lastSeconds / 60L : -1L;
    digitalWrite(cfg.ValvesConfig == NZ_XX ? second_chanel : first_chanel, LOW );
    digitalWrite(cfg.ValvesConfig == NZ_XX ? first_chanel : second_chanel, open ? HIGH : LOW );
  }

  if (cfg.ValvesConfig == NO_XX || cfg.ValvesConfig == XX_NO)
  {
    valHighMinuts = !open ? lastSeconds / 60L : -1L;
    digitalWrite(cfg.ValvesConfig == NO_XX ?  second_chanel : first_chanel, LOW );
    digitalWrite(cfg.ValvesConfig == NO_XX ? first_chanel : second_chanel, open ?  LOW : HIGH);
  }

  if (cfg.ValvesConfig == NO_NO)
  {
    valHighMinuts = !open ? lastSeconds / 60L : -1L;
    if (open)
    {
      digitalWrite(first_chanel, LOW );
      digitalWrite(second_chanel, LOW );

    } else
    {
      digitalWrite(first_chanel, lastChanelUsed == OCSecond ? LOW : HIGH);
      digitalWrite(second_chanel, lastChanelUsed == OCFirst ? LOW : HIGH );
      lastChanelUsed = lastChanelUsed == OCSecond ? OCFirst : OCSecond;
    }
  }
}
