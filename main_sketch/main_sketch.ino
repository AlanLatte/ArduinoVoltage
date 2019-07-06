/*
      VERSION:  0.2.2a
*/

// Подлкючаем необходимые библиотеки
#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <SD.h>
#include <string.h>

/*  Настройка пинов */
#define analogAmperage  0                               //  Pin Для датчика сила тока.
#define analogVoltage   1                               //  Pin Для датчика напряжения.
#define analogTumbler   2                               //  Pin тумблера.

#define sdPin           4                               //  Pin SD карты.
#define errorOfInit     5                               //  Pin индикации ошибки              (светодиод 1)
#define initIndication  6                               //  Pin индикации подключения мудлей  (светодиод 2)
#define writeIndication 7                               //  Pin индикации о записи            (светодиод 3)
/*  --------------  */

/* TODO: Debug mode specification
  [-] Disables the approximation of data to the source (to the divisor)
  [+] USB debugging is enabled
*/

/*  Обозначаем константы  */

const   short int   timeInterval          = 10;           //  Раз в сколько секунд будет происходить запись.
const   float       interval              = 0.5;         //  Частота сканироввания датчика напряжение. (second /interval)
const   float       resistance            = 2.75;        //  Сопротивление тока.
const   bool        InitializationSDcard  = false;       //  Включена SD карта в сборку? false - нет, true - да.
const   size_t      highInputVoltage      = 100;         //  Наибольшее постпающее напряжение
const   size_t      maxOutputCurrent      = 300;         //  Наибольшее выходящее значение силы тока
const   String      FileName              = "LOGS.txt";  //  Название файла в который будет происходить запись.

const   short int   amountOfElements      = timeInterval * (1 / (interval));
const   short int   voltageDivider        = highInputVoltage / 5;
const   short int   amperageFactor        = maxOutputCurrent / 5;
const   char       *monthName[12]         =  {
                                              "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
                                            };
/* Warning: Не изменять*/
float               timeLimit             = timeInterval;
File                logFile;
tmElements_t        tm;
bool parse                                = false;
bool config                               = false;
/*----------------------------*/

void setup() {

  bool    checkOfInit  = true;
  size_t  checkOfTumbler;

  Serial.begin(9600);
  /*  Инициализация и настройка пинов */
  pinMode(analogAmperage  , INPUT );
  pinMode(analogVoltage   , INPUT );
  pinMode(errorOfInit     , OUTPUT);
  pinMode(writeIndication , OUTPUT);
  pinMode(initIndication  , OUTPUT);
  /*  ------------------------------- */
  /*  Отключение светодиодов*/
  /*  ------------------  */
  while(!Serial);
  checkOfTumbler = analogRead(analogTumbler);
  Serial.println(checkOfTumbler);

  if (checkOfTumbler == 0){
    Serial.println("tuningClock!");
    tuningClock();
    if (!(parse && config)) {
      checkOfInit &= false;
    }else{
      checkOfInit &= true;
    }
  }
  /*  Инициализация всех модулей.  */
  if (InitializationSDcard){
    if (SD.begin(sdPin)) {
      checkOfInit &= true;
    } else {
      if (checkOfInit) {
        checkOfInit &= false;
      }
    }
  } else {
      checkOfInit &= true;
  }
  /* ---------------------------------- */

  /*  Проверка на успешно пройденную инициализацю */
  if (!checkOfInit) {
    digitalWrite(errorOfInit, HIGH);
  }else{
    digitalWrite(initIndication, HIGH);
  }
  /*  ------------------------------------  */
}

void loop() {

  digitalWrite(writeIndication, LOW); // Отключаем индикацию записи в файл

  /* Собираем данные с датчиков, объединяем в необходимый формат. */
  String collected_all_data   =   "";
  String collectedDataVoltage =   getVoltageData();
  collected_all_data          +=  getDateTime();
  collected_all_data          +=  "-";
  collected_all_data          +=  collectedDataVoltage;
  /*--------------------------*/

  /*
    Формат данных:
        Год . Месяц . День - Час : Минута : Секунда - Мин.значМин.знач - Макс.знач - Сред.знач - Макс.знач - Сред.знач
  */
  if (InitializationSDcard){
    writeToFile(collected_all_data, FileName);  // Записываем в файл на SD карту.
  } else{
    Serial.println(collected_all_data);         // Выводит на экраз значения.
  }
  timeLimit = timeInterval;                     // Сбрасываем время отчета.
}

void tuningClock(){

  if (getCurrentDate(__DATE__) && getCurrentTime(__TIME__)) {
    parse = true;
    if (RTC.write(tm)) {
      config = true;
    }
  }
}

bool getCurrentTime(const char *str){
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

bool getCurrentDate(const char *str){
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}


void writeToFile(String data, String FileName) {
  /*  Запись в файл   */
  logFile = SD.open(FileName, FILE_WRITE);
  if (logFile) {
    logFile.println(data);
    logFile.close();
    digitalWrite(writeIndication, HIGH);  // Включение светодиода
  } else {
    Serial.println("error opening LOGS.txt");
  }
  /*  ---------   */
}

String getVoltageData() {
  float* data = getData();
  String collectedData    = String(data[0]) + "-"
                          + String(data[1]) + "-"
                          + String(data[2]) + "-"
                          + String(data[3]) + "-"
                          + String(data[4]) + "-"
                          + String(data[5]);
  return collectedData;
  /*
    data[0] -- Минимальное значние силы тока
    data[1] -- Максимальное значение силы тока
    data[2] -- Среднее значение силы тока
    data[3] -- Минимальное значние напряжения
    data[4] -- Максимальное значение напряжения
    data[5] -- Среднее значение напряжения
      |
      Мин.знач_СТ - Макс.знач_СТ - Сред.знач_СТ - Мин.знач_Н - Макс.знач_Н - Сред.знач_Н
  */
}

String getDateTime() {
  if (RTC.read(tm)){
    String collectedData = String(tmYearToCalendar(tm.Year)) + "." + tm.Month + "." +  tm.Day + "-" + tm.Hour + ":" +  tm.Minute + ":" + tm.Second;
    /*
      Год . Месяц . День - Час : Минута : Секунда
    */
    return collectedData;
  }else{
      digitalWrite(errorOfInit, HIGH);
  }

}


float * getData() {
  /* Warning: Значения не изменять */
  static  float   data[6];
  short   int     analogOfAmperage;
  short   int     analogOfVoltage;
  float           voltageOf_AS;
  float           voltageOf_VS;
  float           currentVoltage;
  float           amperage;
  float           averageOfAmperage = 0;
  float           averageOfVoltage  = 0;
  float           maxElem_Amperage  = 0;
  float           maxElem_Voltage   = 0;
  float           minElem_Amperage  = maxOutputCurrent;
  float           minElem_Voltage   = highInputVoltage;
  short   int     delay_            = interval*1000;
  /* --------------------------- */

  for (size_t index = 0; index < amountOfElements; index ++)
  {
    analogOfAmperage  = analogRead(analogAmperage);                     // Считываем значения с аналогового порта для датчика тока. (ДТ)
    analogOfVoltage   = analogRead(analogVoltage );                     // Считываем значения с аналогового порта для датчика напряжение. (ДН)
    voltageOf_AS      = ( 5 / 1024.0 ) * analogOfAmperage;              // Формула для расчёта напряжения c ДТ.
    voltageOf_VS      = ( 5 / 1024.0 ) * analogOfVoltage ;              // Формула для расчёта напряжения с ДН.
    currentVoltage    = voltageOf_VS * voltageDivider;                  // Обратное возрващение в напряжение (после делителя напряжения)
    amperage          = (voltageOf_AS / resistance) * amperageFactor; // Формула для расчёта силы тока. (V/R)
    averageOfVoltage  += currentVoltage;                                // Скалдываем в перменную все значение напряжения.
    averageOfAmperage += amperage;                                      // Складываем в перменную все значения (силы тока. * максимально выходной ток)

    /*  Проверка на минимальные и максимальные значения  */
    if   (amperage <= minElem_Amperage)       {
      minElem_Amperage = amperage;
    } if (amperage >= maxElem_Amperage)       {
      maxElem_Amperage = amperage;
    } if (currentVoltage <= minElem_Voltage)  {
      minElem_Voltage = currentVoltage;
    } if (currentVoltage >= maxElem_Voltage)  {
      maxElem_Voltage = currentVoltage;
    }
    /*  --------------------- */

    timeLimit -= interval;                        // Вычитаем от переменной интервал.
    delay(delay_);                                // Время простоя.
  }
  data[0] = minElem_Amperage;
  data[1] = maxElem_Amperage;
  data[3] = minElem_Voltage ;
  data[4] = maxElem_Voltage ;
   /* Получение среднего значения. (Сумму всех значений делим на количество элементов) */
  data[2] = averageOfAmperage / amountOfElements;
  data[5] = averageOfVoltage  / amountOfElements;
  /*  ------------------------------------------- */
  return data;
}
