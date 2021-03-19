#ifndef SENSORS_H
#define SENSORS_H
#include <OneWire.h>                    
#include <DallasTemperature.h> 

class SensorsBase
{
  public:
  virtual void Setup();
  virtual unsigned long GetTime();
  virtual void RequestTemps();
  virtual double GetTemp(int index);
  virtual byte GetFlow();
  virtual void SetFlowPulses(int cnt, int interval);
  
  virtual void AddHour();
  virtual void Add10Min();  
};

class Sensors : public SensorsBase
{
  unsigned long TimeOffset;
  byte flow;
  OneWire oneWire;                   //wywołujemy transmisję 1-Wire na pinie 10
//pin- pin przy pomocy, którego chcemy utworzyć transmisję 1-Wire
  DallasTemperature tsensors;         //informujemy Arduino, ze przy pomocy 1-Wire
  public:
  Sensors();
 void Setup();
  virtual unsigned long GetTime();
  virtual void RequestTemps();
  virtual double GetTemp(int index);
  virtual byte GetFlow();
  virtual void SetFlowPulses(int cnt, int interval);
  
  void AddHour();
  void Add10Min();

  
};

#endif
