#include "Sensors.h"
#include "Arduino.h"
#include "Config.h"
#include "appcfg.h"

int one_wire = 10;
Sensors::Sensors(): oneWire(one_wire), tsensors(&oneWire)
{
  TimeOffset = (unsigned long)12 * 1000 * 60 * 60;
}

void Sensors::Setup()
{
  tsensors.begin();                           //rozpocznij odczyt z czujnika
 
}

unsigned long Sensors::GetTime()
{
  return (TimeOffset + (millis() EXTRA_MILLIS)) / 1000 / 60;
}

void  Sensors::AddHour()
{
  unsigned long mins = GetTime();
  unsigned long hours = mins / 60;
  mins = mins % 60;
  hours = hours + 1;
  TimeOffset = hours * 1000 * 60 * 60 + mins * 1000 * 60 - (millis() EXTRA_MILLIS);
}
void  Sensors::Add10Min()
{
  unsigned long mins = GetTime();
  unsigned long hours = mins / 60;
  mins = mins % 60;
  mins = mins + 10;

  TimeOffset = hours * 1000 * 60 * 60 + mins * 1000 * 60 - (millis() EXTRA_MILLIS);
}

void Sensors::RequestTemps()
{
  tsensors.requestTemperatures();            //zazadaj odczyt temperatury z czujnika
}
double  Sensors::GetTemp(int index)
{
  return tsensors.getTempCByIndex(index);
}

byte  Sensors::GetFlow()
{
  return flow;
}
SensorsBase::SensorsBase()
{
 flow = 0;
 }

void  SensorsBase::AddHour()
{
}
void  SensorsBase::Add10Min()
{
}

void SensorsBase::RequestTemps()
{
}

void SensorsBase::SetFlowPulses(int cnt, int interval)
{  Configuration& cfg = Config::Get();
  double pulsPerSeconds = double(cnt) / double(interval) * 1000.0f;

  int f = (int)( pulsPerSeconds / cfg.flowFactor);
  flow = f > 254 ? 254 : (byte)f;
  }
