/*
      VERSION:  0.1d
*/

// Подлкючаем необходимые библиотеки
#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <SD.h>
#include <string.h>

#define sdPin         4                             //  Pin SD карты.
#define analogInput   0                             //  Pin Для "+" датчика напряжения.

const   int    timeInterval          = 5;          //  Раз в сколько секунд будет происходить запись.
const   double  interval             = 0.1;         //  Частота сканироввания датчика напряжение. (second /interval)
const   double  resistance           = 2.75;        //  Сопротивление тока.
const   bool   InitializationSDcard  = false;       //  Включена SD карта в сборку? false - нет, true - да.
const   String FileName              = "LOGS.txt";  //  Название файла в который будет происходить запись.

/* Warning: Не изменять*/
double        timeLimit              = timeInterval;
const  int   amountOfElements        = timeInterval * (1 / (interval));
File         logFile;
/*----------------------------*/

void setup() {
  Serial.begin(9600);
  pinMode(analogInput, INPUT);
  while (!Serial) ;
  setSyncProvider(RTC.get);
  /*  Проверка на подключения всех модулей.  */
  Serial.println("Check all connections.\n");
  Serial.println("Connecting to RTC");
  if (timeStatus() != timeSet) {
    Serial.println("Unable to sync with the RTC");
  } else {
    Serial.println("installation real time...");
    tuningClock();
    Serial.println("Done!\n");
  }
  if (InitializationSDcard){
    Serial.println("Connecting to SD card...");
    if (SD.begin(sdPin)) {
      Serial.println("Done!\n\n");
    } else {
      Serial.println("Sd card connecting failed :( ");
    }
  } else {
    Serial.println("Initialization of the memory card is not included");
  }
  /* ---------------------------------- */
}

void loop() {
  /* Собираем данные с датчиков, объединяем в необходимый формат. */
  String collected_all_data = "";
  String collectedDataVoltage = getVoltageData();
  collected_all_data += getDateTime();
  collected_all_data += "-";
  collected_all_data += collectedDataVoltage;
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
  const String* date  =   getCurrentDate();
  const String* time_ =   getCurrentTime();
  int   month_        =   date[0].toInt();
  int   day_          =   date[1].toInt();
  int   year_         =   date[2].toInt();
  int   hour_         =   time_[0].toInt();
  int   minute_       =   time_[1].toInt();
  int   second_       =   time_[2].toInt();
  /* Настраиваем часы. */
  setTime(hour_,minute_,second_,day_,month_,year_);
  /*
    Формат настройки :
      час, минута, секунда, день, месяц, день
  */
  RTC.set(now());   // Загружаем в часы.
}

String* getCurrentTime(){
  static String data[3];
  int index = 0;
  char str[] = __TIME__;
  char * pch;

  pch = strtok (str,":");
  while (pch != NULL)
  {
    data[index] = pch;
    index += 1;
    pch = strtok (NULL, ":");
  }
  return data;
}

String* getCurrentDate(){
  const String monthName[12]  = {
                                  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
                                };
  static String data[3];
  int index = 0;
  char str[] = __DATE__;
  char * pch;

  pch = strtok (str," ");
  while (pch != NULL)
  {
    data[index] = pch;
    index += 1;
    pch = strtok (NULL, " ");
  }
  for (int i = 0; i < 12; i++){
    if (String(data[0]) == monthName[i]){
      data[0]= i+1;
    }
  }
  return data;
}

void writeToFile(String data, String FileName) {
  /*  Запись в файл   */
  logFile = SD.open(FileName, FILE_WRITE);
  if (logFile) {
    logFile.println(data);
    logFile.close();
  } else {
    Serial.println("error opening LOGS.txt");
  }
  /*  ---------   */
}

String getVoltageData() {
  double* data = getData();
  String collectedData = String(data[0]) + "-" + String(data[1]) + "-" + String(data[2]);
  return collectedData;
  /*
    data[0] -- Минимальное значние
    data[1] -- Максимальное значение
    data[2] -- Среднее значение
      |
      Мин.знач - Макс.знач - Сред.знач
  */
}

String getDateTime() {

  String collectedData = String(year()) + "." + month() + "." +  day() + "-" + hour() + ":" +  minute() + ":" + second();

  return collectedData;
  /*
    Год . Месяц . День - Час : Минута : Секунда
  */
}


double * getData() {
  /* Warning: Не изменять */
  static double data[3];
  int   analogValue;
  double average = 0;
  double Array[amountOfElements];
  double maxElem;
  double minElem;
  double voltage;
  double amperage;
  int delay_  = interval*1000;
  /* ------------ */
  for (int index = 0; index < amountOfElements; index ++)
  {
    analogValue = analogRead(analogInput);       // Считываем значения с аналогового порта.
    voltage = ( 5 / 1024.0 ) * analogValue;      // Формула для расчёта напряжения.
    amperage = voltage / resistance;             // Формула для расчёта силы тока.
    average += amperage;                         // Складываем в перменную все значения силы тока.
    Array[index] = amperage;                     // Записываем в массив данные.
    timeLimit -= interval;                       // Вычитаем от переменной интервал.
    delay(delay_);                               // Время простоя.
  }

  /* Алгоритм поиска максимального и минимального значения */
  maxElem = Array[0]; minElem = Array[0];
  for(int i = 1; i < amountOfElements; i++){
    if (maxElem < Array[i]) {
      maxElem = Array[i];
    }
    else if (minElem > Array[i]){
      minElem = Array[i];
    }
  }
  /*  ----------------------------- */
  data[0] = minElem;
  data[1] = maxElem;
  data[2] = average / amountOfElements;
  return data;
}
