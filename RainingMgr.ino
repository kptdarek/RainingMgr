#include "Ui.h"
#include "Sensors.h"
#include "SensorsSymulator.h"
#include "Config.h"
#include "Valves.h"
#include "appcfg.h"
#define FLOW_SAMPLE_MILLIS  1000L

UIMgr ui;
SensorsBase* sensors;
Sensors realSensors;
SensorsSymulator dynaSensors1;
Valves valves;

double minNoErrorTemp = -30.0;
double forceTemp = -100.0;
bool firstIter = true;
byte avrFlow = 0;
byte sensorInterrupt = 0;  // 0 = digital pin 2
byte sensorPin       = 2;
int pulseCount = 0;
double totalWater = 0.0;
long  totalWaterAll = 0;

unsigned long lastPulsMillis = 0;
unsigned long lastGoodTermPeekMillis = 0;
WorkMode currentMode = WmNone;


void setup()
{
  Config::Load();
  Configuration& cfg = Config::Get();
  totalWaterAll = Config::GetTotalWater();
  sensors = &realSensors;

  if (cfg.Go25Temp + cfg.DelayStop < cfg.AntiFrezTemp)
  {
    ui.AddAlarm(F("Za male opoz.wy."), ALARM_INFO);
  }

  if (cfg.Sensors > 0 && cfg.Sensors <= 1 + dynaSensors1.Count())
  {
    sensors = &dynaSensors1;
    dynaSensors1.SetMode(cfg.Sensors - 1);
  };

  ui.Setup(&valves);
  sensors->Setup();
  valves.Setup();
  ui.ShowStatus();
  ui.Beep();

  pinMode(sensorPin, INPUT_PULLUP);
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  lastPulsMillis = (millis() EXTRA_MILLIS);
#if SERIAL_PRINT
  Serial.begin(9600);
  Serial.print(F("test"));
  Serial.print("cfg.Sensors:");
  Serial.println(cfg.Sensors);

  Serial.print("sizeof(Cfg):");
  Serial.println(sizeof(Configuration));

#endif
}

void SetMode(WorkMode mode)
{
  if (currentMode != mode)
  {
    currentMode = mode;
    Configuration& cfg = Config::Get();
    valves.SetMinutsPeriod(mode == WmDeleayFolowering ? cfg.DelayFlowerMinutsPeriod : 1);

    ui.ShowMode(mode);
  }
}

void DoStatTempMenu()
{
  const __FlashStringHelper* valvest[] = {F("Anty zmarz"), F("25%"), F("50%"), F("75%"), F("100%"), F("9oC")};
  int cfgIndex = ui.Menu(F("Stala temp"), valvest, sizeof(valvest) / sizeof(__FlashStringHelper*));
  if (cfgIndex != -1)
  {
    forceTemp = dynaSensors1.GetTempFromMode((StaticSymMode)(cfgIndex + 1));
  }
}

void DoDynamicSymMenu()
{
  Configuration& cfg = Config::Get();
  const __FlashStringHelper** valvest = dynaSensors1.Titles();
  int cfgIndex = ui.Menu(F("Stat dyn dla:"), valvest, dynaSensors1.Count());
  if (cfgIndex != -1)
  {
    cfg.Sensors = cfgIndex + 1;
    sensors = &dynaSensors1;
    dynaSensors1.SetMode(cfgIndex);
    sensors->Setup();
  }
}

void DoTestMenu()
{
  Configuration& cfg = Config::Get();
  const __FlashStringHelper* items[] = { F("Stala temp"), F("Sym. dyna"), F("Wyl. symul")
#if DEBUG
                                         , F("LEDy")
#endif
                                       };
  int index = ui.Menu(F("Testy"), items, sizeof(items) / sizeof(__FlashStringHelper*));
  switch (index)
  {
    case 0:
      DoStatTempMenu();
      break;
    case 1:
      DoDynamicSymMenu();
      break;
    case 2:
      sensors = &realSensors;
      cfg.Sensors = 0;
      sensors->Setup();
      break;
#if DEBUG
    case 3:
      ui.TestLeds();
      break;
#endif
  }
}

void DoValvesCfg()
{
  Configuration& cfg = Config::Get();
  const __FlashStringHelper* valvest[] = {F("WYL WYL"), F("NO NO"), F("NO WYL"), F("WYL NO"), F("NZ WYL"), F("WYL NZ")};
  int cfgIndex = ui.Menu(F("Podl. zawory"), valvest, sizeof(valvest) / sizeof(__FlashStringHelper*), cfg.ValvesConfig);
  if (cfgIndex != -1)
  {
    cfg.ValvesConfig = (ConnValves) cfgIndex;
    valves.ResetFlowStatus();
  }
}

void DoTempMenu()
{
  const __FlashStringHelper* items[] = {F("Anty zmarz"), F("25%"), F("50%"), F("75%"), F("100%"),  F("Opoz kw 100%"), F("Rozmiar hist"), F("Opoz wyl 25%")};
  int index;
  do {
    index = ui.Menu(F("Temperatury"), items, sizeof(items) / sizeof(__FlashStringHelper*));
    Configuration& cfg = Config::Get();
    switch (index)
    {
      case 0:
        cfg.AntiFrezTemp = ui.SetValue(items[index], cfg.AntiFrezTemp);
        break;
      case 1:
        cfg.Go25Temp = ui.SetValue(items[index], cfg.Go25Temp);
        break;
      case 2:
        cfg.Go50Temp = ui.SetValue(items[index], cfg.Go50Temp);
        break;
      case 3:
        cfg.Go75Temp = ui.SetValue(items[index], cfg.Go75Temp);
        break;
      case 4:
        cfg.Go100Temp = ui.SetValue(items[index], cfg.Go100Temp);
        break;
      case 5:
        cfg.DelayFlower100Temp = ui.SetValue(items[index], cfg.DelayFlower100Temp);
        break;
      case 6:
        cfg.HisterSize = ui.SetValue(items[index], cfg.HisterSize);
        break;
      case 7:
        cfg.DelayStop = ui.SetValue(items[index], cfg.DelayStop);
        break;
    }
  }  while (index != -1);
}

void SetAlaramFlags(const __FlashStringHelper* title, byte bit)
{
  Configuration& cfg = Config::Get();
  bool ret = ui.SetValue(title, (bool) (cfg.Alarms & bit));
  if (ret) cfg.Alarms = cfg.Alarms | bit;
  else cfg.Alarms = cfg.Alarms & ~bit;
}

void DoAlarmsMenu()
{
  const __FlashStringHelper* items[] = {F("Al. Przeplyw"), F("Al. Cewka"), F("Al. Informa"), F("Al. termometr"), F("Al. dzwieki"), F("Al. Wszytkie")};
  int index;
  do {
    index = ui.Menu(F("Alarmy"), items, sizeof(items) / sizeof(__FlashStringHelper*));

    switch (index)
    {
      case 0:
        SetAlaramFlags(items[index], ALARM_FLOW);
        break;
      case 1:
        SetAlaramFlags(items[index], ALARM_SELENOID);
        break;
      case 2:
        SetAlaramFlags(items[index], ALARM_INFO);
        break;
      case 3:
        SetAlaramFlags(items[index], ALARM_TERMDEV);
        break;
      case 4:
        SetAlaramFlags(items[index], ALARM_SOUNDS);
        break;
      case 5:
        SetAlaramFlags(items[index], ALARM_ALL);
        break;

    }
  }  while (index != -1);

}

void DoSettingsMenu()
{
  const __FlashStringHelper* items[] = {F("Alarmy"), F("Zawory"), F("Temperaury"), F("Ant zam okres"), F("Ant zam otwar"), F("Max min cewki"),  F("Zapisz"), F("Przywroc"), F("Zeruj liczn."), F("Reset")};
  int index;
  do {
    index = ui.Menu(F("Ustawienia"), items, sizeof(items) / sizeof(__FlashStringHelper*));

    Configuration& cfg = Config::Get();
    switch (index)
    {
      case 0:
        DoAlarmsMenu();
        break;
      case 1:
        DoValvesCfg();
        break;
      case 2:
        DoTempMenu();
        break;
      case 3:
        cfg.AntiFreezePeriod = ui.SetValue(items[index], cfg.AntiFreezePeriod, 5);
        break;
      case 4:
        cfg.AntiFreezeOpened = ui.SetValue(items[index], cfg.AntiFreezeOpened);
        break;
      case 5:
        cfg.ValMaxWorkMin = ui.SetValue(items[index], cfg.ValMaxWorkMin);//TODO zmienic na 5
        break;
      case 6:
        Config::Save();//ok
        break;
      case 7:
        Config::Init();
      case 8:
        Config::SetTotalWater(0);
        totalWaterAll = 0;
      case 9:
        asm volatile ("  jmp 0");
    }
  } while (index != -1);
}

void DoFixedMode()
{
  if (valves.CanSetFixedMode())
  {
    if (ui.YesNo(valves.IsNO() ? F("Zamkniete #FIX") : F("Bajpas #FIX")))
    {
      valves.SetFixedMode();
    }
  }
}

void DoMainMenu()
{
  const __FlashStringHelper* items[] = {F("Alarmy"), F("Status"), F("Hisoria"), F("Opoz kwitnien"), F("Pol automat"), F("Ustawienia"),  F("Testy"),  F("#FIX Bajp/Zamk")};
  int index = ui.Menu(F("Meni glowne"), items, sizeof(items) / sizeof(__FlashStringHelper*) -  (valves.CanSetFixedMode() ? 0 : 1));

  switch (index)
  {
    case 0:
      if (ui.IsAnyAlarm()) ui.CheckAlarms();
      break;
    case 1:
      ui.ShowStatus();
      break;
    case 2:
      ui.History();
      break;
    case 3:
      SetMode(WmDeleayFolowering);
      break;
    case 4:
      DoHalfAutoMode(items[index]);
      break;
    case 5:
      DoSettingsMenu();
      break;
    case 6:
      DoTestMenu();
      break;
    case 7:
      DoFixedMode();
      break;
  }
}

void DoHalfAutoMode( const __FlashStringHelper* title)
{
  Configuration& cfg = Config::Get();
  cfg.HalfAutomaticMode = ui.SetValue(title, cfg.HalfAutomaticMode);
  if (cfg.HalfAutomaticMode) cfg.Alarms = cfg.Alarms | ALARM_INFO;
  if (cfg.HalfAutomaticMode && currentMode == WmAntiFreez)  currentMode == WmAntiFreezHalfAuto;
  else if (!cfg.HalfAutomaticMode && currentMode == WmAntiFreezHalfAuto)  currentMode == WmAntiFreez;
}

void PressSmartKey()
{
  ui.Beep();
  ui. WaitKeyUp();

  if (valves.CanSetFixedMode())
  {
    DoFixedMode();
    return;
  }

  if (ui.IsActiveAlarm())
  {
    ui.CheckAlarms();
    return;
  }

  if (IsOk(forceTemp))
  {
    forceTemp = -100;
    return;
  }

  ui.ShowStatus();
}

void loop()
{
  static byte iter = 0;
  static byte saveEepromCounter = 0;
  static long avrFlowSum = 0;
  static int avrFlowSamples = 0;
  static unsigned long lastChekAvrFlowMillis = 0 ;
  static byte t2RetryCnt = 0;
  Configuration& cfg = Config::Get();
  iter++;
  sensors->RequestTemps();

  Task2Do task = ui.ProcessKeys();
  switch (task)
  {
    case Time15MinPlus:
      sensors->AddHour();
      ui.ResetHistory();
      ui.Beep();
      break;
    case Time15MinMinus:
      sensors->Add10Min();
      ui.ResetHistory();
      ui.Beep();
      break;
    case MainMenu:
      DoMainMenu();
      break;
    case Back:
      PressSmartKey();
  };

  unsigned long mils = (millis() EXTRA_MILLIS);
  if (mils - lastPulsMillis > FLOW_SAMPLE_MILLIS )
  {
    sensors->SetFlowPulses(pulseCount, mils - lastPulsMillis);
    lastPulsMillis = mils;
    pulseCount = 0;
  }

  ui.ShowLastAlarm((mils / 1000 / 2) % 2 == 0 );

  double t1 = sensors->GetTemp(0);
  double t2 = sensors->GetTemp(1);

  if (IsOk(t1) || IsOk(t2))
  {
    lastGoodTermPeekMillis = mils;
  } else {
    if ((mils - lastGoodTermPeekMillis) / 1000L > 60L)
    {
      ui.AddAlarm(F("Awaria termometr"), ALARM_TERMDEV);
      lastGoodTermPeekMillis = mils;
    }
  }

  if (IsOk(forceTemp))
  {
    t1 = t2 = forceTemp;
  }
  ui.SetTime(sensors->GetTime(), !firstIter); firstIter = false; //prevent from save -err- as temperature in first iteration

  ui.SetTemperature(t1, TopT);
  ui.SetTemperature(t2, BottomT);

  int flow = sensors->GetFlow();
  ui.SetFlow(flow, CurrentV);

  if (iter % 4 == 0) { //~ onece a seconds
    if (avrFlowSamples == 0) lastChekAvrFlowMillis = mils;
    avrFlowSum += flow; avrFlowSamples++;
    if (avrFlowSamples >=  ((currentMode == WmDeleayFolowering) ? cfg.DelayFlowerMinutsPeriod * 60 : cfg.AntiFreezePeriod))
    {
      avrFlow = avrFlowSum / avrFlowSamples;

      unsigned long secs = (mils - lastChekAvrFlowMillis) / 1000L;
      totalWater += (double)((double)avrFlowSum / (double)avrFlowSamples) * ((double)secs / 60.0f);

#if SERIAL_PRINT

      Serial.print(F("totalWater:"));
      Serial.print(totalWater);
      Serial.print(F("totalWaterAll:"));
      Serial.print(totalWaterAll);
      Serial.print(F("secs:"));
      Serial.print(secs);
      Serial.print(F("avrFlow:"));
      Serial.print(avrFlow);
      Serial.print(F("saveEepromCounter:"));
      Serial.println(saveEepromCounter);
#endif
      if (saveEepromCounter >= 15 ) // 15 x 4 min
      {
        saveEepromCounter = 0;
        if (totalWater > 0) Config::SetTotalWater(totalWaterAll + (long)totalWater);
      }
      saveEepromCounter ++;

      avrFlowSum = avrFlowSamples = 0;
    }

    valves.SetFlow(flow);
    ui.SetFlow(avrFlow , AverageF);

    VAlarm alrm = valves.PeekAlarm();
    if (alrm.title != NULL)
    {
      ui.AddAlarm(alrm.title, alrm.type);
    }
  } //~ onece a seconds


  if (currentMode == WmAntiFreez || currentMode == WmAntiFreezHalfAuto)
  {
    if ( t1 >= 5.0 && t2 >= 5.0) SetMode(WmNone);
  } else
  {
    //automatic antifreez mode
    if ((t1 < 3.0 && t1 > -20.0) || (t2 < 3.0 && t2 > -20.0))
    {
      SetMode(cfg.HalfAutomaticMode ? WmAntiFreezHalfAuto : WmAntiFreez);
    }
  }

  if (IsOk(t2) || ((t2RetryCnt++) >= 40))
  {
    switch (currentMode)
    {
      case WmDeleayFolowering:
        CheckDelayFlowMode(t1, t2);
        break;
      case   WmAntiFreez:
        CheckAntiFreezMode(t1, t2, false);
        break;
      case WmAntiFreezHalfAuto:
        CheckAntiFreezMode(t1, t2, true);
        break;
    }

    t2RetryCnt = 0;
  }

  valves.Adjust();
  int powerPrec = valves.PowerInMode();
  ui.SetPowerPrecent(powerPrec, Config::Get().HalfAutomaticMode);
  ui.Pulse((mils / 1000) % 2 == 0, currentMode);

  delay(250);
}

void pulseCounter()
{
  //Serial.print("iii");
  // Increment the pulse counter
  pulseCount++;
}

bool IsOk(double t1)
{
  return t1 > minNoErrorTemp;
}

double Right(double t1, double t2)
{
  if (IsOk(t2)) return t2; //dolny wazniejszy
  return t1;
}

void CheckDelayFlowMode(double t1, double t2)
{
  Configuration& cfg = Config::Get();
  double t = Right( t1,  t2);
  if (!IsOk(t))
  {
    valves.SetMode(Val50);
    return;
  }

  valves.SetMode(t > cfg.DelayFlower100Temp ? Val100 : Val50);
}

void CheckAntiFreezMode(double t1, double t2, bool halfAuto)
{
  static ValMode lastMode = ValZero;
  double t = Right( t1,  t2);
  if (IsOk(t))
  { ValMode m;
    ValMode mG = ModeGrow(t);
    ValMode mD = ModeDecr(t);
    if (mG != mD)
    {
      if (mG == lastMode) m = mG;
      else m = mD;
    } else
    {
      m = mG;
    }

    ValMode newMode = m;

    if (halfAuto)
    {
      if (m == Val75 || m == Val50 || m == Val25) newMode = Val100;
      if (lastMode == ValAntiFreeze && newMode == Val100)
      {
        ui.AddAlarm(F("Wlaczony 100%"), ALARM_INFO);
      }
    }

    valves.SetMode(newMode);

    if (lastMode == Val25 && valves.CanSetFixedMode()) // nie zadziała jak opróźnienie wyłaczenia jest mniejsze niż antyfreez generalnie przy wyłaczaniu nie ma antyfreez
    {
      ui.AddAlarm(F("Zalecany FIX #0%"), ALARM_INFO);
    }

    lastMode = m;
  }
}

ValMode ModeGrow(double t)
{
  Configuration& cfg = Config::Get();
  return Mode(t, cfg.HisterSize / -2.0);
}

ValMode ModeDecr(double t)
{
  Configuration& cfg = Config::Get();
  return Mode(t, cfg.HisterSize / 2.0);
}

ValMode Mode(double t, float h)
{
  Configuration& cfg = Config::Get();
  ValMode m = ValZero;
  if (t < cfg.AntiFrezTemp + h) m = ValAntiFreeze;
  if (t < cfg.Go25Temp + (h > 0 ? h : cfg.DelayStop)) m = Val25;
  if (t < cfg.Go50Temp + h) m = Val50;
  if (t < cfg.Go75Temp + h) m = Val75;
  if (t < cfg.Go100Temp + h) m = Val100;
  return m;
}
