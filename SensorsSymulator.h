#ifndef SENSORSSYMULATOR_H
#define SENSORSSYMULATOR_H
#include "Sensors.h"

struct SensStep
{
  double T1;
  double T2;
  byte Flow;
  byte NexStepMin;
} ;

enum StaticSymMode
{
   SSAntiFreez = 1,
   SS25 = 2,
   SS50 = 3,
   SS75 = 4,
   SS100 = 5,
   SSNone = 6,
   SSLast = 7   
};


class SensorsSymulator : public SensorsBase
{
  SensStep* Steps;
  int Index;
  bool RealFlow;
  int smallIndex;
  unsigned long StartTime, LastStepTime;
  int timeFaktor;
 public:
  SensorsSymulator();
  double GetTempFromMode(StaticSymMode mode);
 void Setup();
  void SetMode(int index, bool realFlow);
  const __FlashStringHelper* Title(int index);
  const __FlashStringHelper** Titles();
  int Count();
  virtual unsigned long GetTime();
  virtual void RequestTemps();
  virtual double GetTemp(int index);
  virtual byte GetFlow();
  
};

#endif
