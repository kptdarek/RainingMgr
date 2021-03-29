#include "SensorsSymulator.h"
#include "Config.h"
#include "appcfg.h"

#define SYM_CNT 3
#define HIST_CNT 8
#define HIST_STEP 20

struct SymDef
{
  const __FlashStringHelper* title;
  SensStep* steps;
  int timeFactor;
};

const __FlashStringHelper* titles[SYM_CNT];

SensorsSymulator::SensorsSymulator():  Index(0),smallIndex(0)
{ 
  
}

void SensorsSymulator::Setup()
{
  StartTime =  (millis() EXTRA_MILLIS);
  LastStepTime = GetTime();
  Index = 0;
  RealFlow = false;
}

SymDef& GestSym(int index)
{
  static SensStep stepsHist[HIST_CNT];
  
  static bool dynamicHist = false;
  if (!dynamicHist)
  {
    Configuration& cfg = Config::Get();
    double hh = cfg.HisterSize + 0.01;
    double h = cfg.HisterSize / 2.2;   
    //(8 -> AF->  AF- h/2 -> 25% -> 50%-h/2 -> 50%+h/2 -> 50%-h-> 75%-> 100% -> 75% ->50% -> 25% ->AF -> 8   (NF ->25->50->50->25->75 ->100 -> 75-> 50 ->25 -> AF- 0)
//    stepsHist[0] = {8.1, 8, 0, HIST_STEP};
//    //stepsHist[1] = {cfg.AntiFrezTemp -hh, cfg.AntiFrezTemp -hh, 5, HIST_STEP* 2};
//    stepsHist[1] = {10, 11, 5, HIST_STEP* 2};    
//    stepsHist[2] = {cfg.Go50Temp -hh , cfg.Go50Temp -hh, 5, HIST_STEP };
//    stepsHist[3] = {cfg.Go50Temp +h , cfg.Go50Temp +h, 5, HIST_STEP};
//    stepsHist[4] = {cfg.Go100Temp-hh , cfg.Go100Temp-hh, 5, HIST_STEP*4};
//    stepsHist[5] = {cfg.Go100Temp-hh , cfg.Go100Temp-hh, 5, HIST_STEP*4};    
//    stepsHist[6] = {cfg.AntiFrezTemp+cfg.DelayStop , cfg.AntiFrezTemp+cfg.DelayStop, 5, HIST_STEP};
//    stepsHist[HIST_CNT-1] = {0, 0, 0, 0};

    
     stepsHist[0] = {8.1, 8, 0, HIST_STEP};
    stepsHist[1] = {cfg.AntiFrezTemp -hh, cfg.AntiFrezTemp -hh, 5, HIST_STEP* 2};    
    stepsHist[2] = {cfg.Go50Temp -hh , cfg.Go50Temp -hh, 5, HIST_STEP };
    stepsHist[3] = {cfg.Go50Temp +h , cfg.Go50Temp +h, 5, HIST_STEP};
    stepsHist[4] = {cfg.Go50Temp -h , cfg.Go50Temp -h, 5, HIST_STEP*2};    
    stepsHist[5] = {cfg.Go100Temp-hh , cfg.Go100Temp-hh, 5, HIST_STEP*4};    
    stepsHist[6] = {cfg.AntiFrezTemp+cfg.DelayStop , cfg.AntiFrezTemp+cfg.DelayStop, 5, HIST_STEP};
    stepsHist[HIST_CNT-1] = {0, 0, 0, 0};
     

 
    
   dynamicHist = true;    
  }

  static SensStep steps1[] =
  { {5.0, 5.2, 9, 240},
    {-6.0, -5.8, 9,80 },
    {4.0, 4.2, 9, 70},
    {-6.0, -5.8, 9,80 },
    {0, 0, 0, 0}
  };

    static SensStep delayFlower[] =
  { {5.0, 5.2, 9, 24},
    {12, 5, 9,255 },
   // {12, 5, 9,255 },
 //   {34, 24, 9,1240 },
 //   {3.0, 3, 9,240 },
    {0, 0, 0, 0}
  };

  static SymDef symDef[SYM_CNT] =
  {
    {F("Histereza"), stepsHist,1000},
    {F("Histereza*10"), stepsHist,10000},
 //   {F("Wielblad-mroz"), steps1,1000},
    {F("Opoznienie kwit"), delayFlower,1000}
  };
  return symDef[index];
}

int SensorsSymulator::Count()
{
  return SYM_CNT;
}

const __FlashStringHelper* SensorsSymulator::Title(int index)
{
  return GestSym(index).title;
}

const __FlashStringHelper** SensorsSymulator::Titles()
{
  for(int i=0; i< SYM_CNT; i++) titles[i] = Title(i);
  return titles;
}

void SensorsSymulator::SetMode(int index, bool realFlow)
{
  SymDef& def = GestSym(index);
  timeFaktor = def.timeFactor;  
  Steps = def.steps;
  RealFlow = realFlow;
}

unsigned long SensorsSymulator::GetTime()
{
  return 19L * 60L +  ((millis() EXTRA_MILLIS) - StartTime) / timeFaktor;
}

void SensorsSymulator::RequestTemps()
{
  unsigned long  t = GetTime();
  smallIndex =  t - LastStepTime;
  if (smallIndex >= Steps[Index].NexStepMin)
  {
    Index++;
    smallIndex = 0;
    LastStepTime = GetTime();
    if ( Steps[Index].NexStepMin == 0) Index --;
  } 
}
double  SensorsSymulator::GetTemp(int index)
{
  double t = index == 0 ? Steps[Index].T1 : Steps[Index].T2;
  if (Steps[Index+1].NexStepMin != 0)//nie ostatni
  {
       double nextT = index == 0 ? Steps[Index+1].T1 : Steps[Index+1].T2;
       double delta =  (nextT - t)*smallIndex / double(Steps[Index].NexStepMin);

  //  Serial.print(F("Delta("));Serial.print(smallIndex);Serial.print(F(") ")); Serial.print(t);Serial.print(F("->"));Serial.print(nextT);Serial.print(F(" d=")); Serial.println(delta);    
       
       return t+delta;
  }
  return t;
}

byte  SensorsSymulator::GetFlow()
{
  return  RealFlow ? flow : Steps[Index].Flow;
}


double SensorsSymulator::GetTempFromMode(StaticSymMode mode)
{
  Configuration& cfg = Config::Get();
  switch (mode)
  {
    case SSAntiFreez:
      return cfg.AntiFrezTemp - 0.2f;
    case SS25:
      return cfg.Go25Temp - 0.2f;
    case SS50:
      return cfg.Go50Temp - 0.2f;
    case SS75:
      return cfg.Go75Temp - 0.2f;
    case SS100:
      return cfg.Go100Temp - 0.2f;
    case SSNone:
      return 8.0f;
  };
};
