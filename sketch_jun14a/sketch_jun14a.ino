// Подлкючаем необходимые библиотеки
#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <SD.h>

const   int   timeInterval    = 4;          //Раз в сколько секунд будет происходить запись
const   float interval        = 0.5         //Частота сканироввания датчика напряжение
const   int   sdPin           = 4;          //Pin SD карты

/* Warning: Не изменять*/
float   timeLimit             = timeInterval;
int     val                   = 0;
String  collectedDataVoltage  = "";
File    logFile;
/*----------------------------*/

void setup() {

  Serial.begin(9600);
  pinMode(A0, INPUT);
  while (!Serial) ;
  setSyncProvider(RTC.get);

  /*  Проверка на подключения всех модулей.   */

  Serial.println("Connecting to SD card...");
  if (SD.begin(sdPin)) {
    Serial.println("Done!\n");
  }
  else {
    Serial.println("Sd card connecting failed");
  }
  Serial.println("Connecting to RTC");
  if (timeStatus() != timeSet) {
    Serial.println("Unable to sync with the RTC");
  }
  else {
    Serial.println("Done!\n");
  }

  /* ---------------------------------- */
}

void loop() {
  String FileName = "LOGS.txt";       //название файла

  if (timeLimit == 0) {

    String collectedDataDatetime = getDateTime();
    String collected_all_data = collectedDataDatetime + "-" + collectedDataVoltage;
    /*
      Год . Месяц . День - Час : Минута : Секунда - Мин.значМин.знач - Макс.знач - Сред.знач - Макс.знач - Сред.знач
    */
    timeLimit = timeInterval;
    writeToFile(collected_all_data, FileName);
  }
  else {
    collectedDataVoltage = getVoltageData();
  }
}


void writeToFile(String data, String FileName) {
  /*  Запись в файл   */
  logFile = SD.open(FileName, FILE_WRITE);

  if (logFile) {
    logFile.println(data);
    logFile.close();
  }
  else {
    Serial.println("error opening LOGS.txt");
  }
  /*  ---------   */
}

String getVoltageData() {
  float* data = getVoltage();
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


float * getVoltage() {
  const int amountOfElements = timeLimit * (1 / (interval));
  static float data[3];
  /*
    обратная зависмость,
      |если Минимальное значение должно быть 4, устанавливаем его максимальному.
      |если Максимальное значение должно быть 4, устанавливаем его Минимальному.
  */
  float maxElem = 0;    // Максимальное значение
  float minElem = 5;    // Минимальное значение
  float average = 0;    // Не изменять.
  float voltage;
  for (int index = 0; index < amountOfElements; index ++)
  {
    val = analogRead(A0);
    voltage = (5 / 1024.0) * val;       // Формула кривой
    average += voltage;
    if (voltage > maxElem) {
      maxElem = voltage;
    }
    if (voltage < minElem) {
      minElem = voltage;
    }
    timeLimit -= interval;
    delay(interval*100);                // Время простоя.
  }
  data[0] = minElem;
  data[1] = maxElem;
  data[2] = average / amountOfElements;
  return data;
}
