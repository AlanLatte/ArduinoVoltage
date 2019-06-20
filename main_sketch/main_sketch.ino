
// Подлкючаем необходимые библиотеки
#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <SD.h>

const   int   timeInterval    = 4;          //Раз в сколько секунд будет происходить записьconst   float interval        = 0.5;         //Частота сканироввания датчика напряжение
const   int   sdPin           = 4;          //Pin SD карты
const   int   analogInput     = 0;
const   float interval        = 0.5;

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
    Serial.println("installation real time...");
    tuningClock();
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
    Serial.println(collected_all_data);
    writeToFile(collected_all_data, FileName);
  }
  else {
    collectedDataVoltage = getVoltageData();
  }
  String collectedDataDatetime = getDateTime();  
}


void tuningClock(){
  const String* date = getCurrentData();
  const String* time_= getCurrentTime();
  int month_ = date[0].toInt();
  int day_ = date[1].toInt();
  int year_ = date[2].toInt();
  int hour_ = time_[0].toInt();
  int minute_=time_[1].toInt();
  int second_=time_[2].toInt();
  setTime(hour_,minute_,second_,day_,month_,year_);
  RTC.set(now());
}

String* getCurrentTime(){
  static String data[3];
  const char CurrentTime[] = __TIME__;
  String collecterOfData;
  int index = 0;
  int dateLength = (sizeof(CurrentTime)/sizeof(char));
//  Serial.println(CurrentTime);
  for (int i = 0; i < dateLength; i++){
    String CurrentTimeString = String(CurrentTime[i]);
    if (CurrentTimeString == ":"){
      data[index] = collecterOfData;
      collecterOfData = "";
      index += 1;
    }
    else{
      collecterOfData += CurrentTimeString;
    }
  }
  data[index] = collecterOfData;
  return data;
}

String* getCurrentData(){
  static String data[3]; 
  const String monthName[12] =   {
                                  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
                                 };
  const char CurrentDate[]=__DATE__;
  
  String collecterOfData;
  int index = 0;
  int dateLength = (sizeof(CurrentDate)/sizeof(char));
  for (int i = 0; i < dateLength; i++){
    String CurrentDateString = String(CurrentDate[i]);
    
    if (CurrentDateString == " ") {
      data[index] = collecterOfData;
      collecterOfData = "";
      index+=1;
    }
    else{
      collecterOfData += CurrentDateString;
    }
  }
  data[index] = collecterOfData;
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
  }
  else {
    Serial.println("error opening LOGS.txt");
  }
  /*  ---------   */
}

String getVoltageData() {
  float* data = getData();
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


float * getData() {
  const int amountOfElements = timeLimit * (1 / (interval));
  static float data[3];
  /*
    обратная зависмость,
      |если Минимальное значение должно быть 4, устанавливаем его максимальному.
      |если Максимальное значение должно быть 4, устанавливаем его Минимальному.
  */
  float Array[amountOfElements];
  float maxElem;
  float minElem;
  float average = 0;    // Не изменять.
  float voltage;
  float amperage;
  for (int index = 0; index < amountOfElements; index ++)
  {
    val = analogRead(A0);
    voltage = (5 / 1024.0) * val;       // Формула кривой
    amperage = (voltage / 2.75)*200;
    average += amperage;
    Array[index] = amperage;              //Записываем в массив данные
    timeLimit -= interval;
    delay(interval*1000);                // Время простоя.
  }
  maxElem = Array[0]; minElem = Array[0];
  for(int i = 1; i < amountOfElements; i++){
    if (maxElem < Array[i]) {
        maxElem = Array[i];
      }
    else if (minElem > Array[i]){
        minElem = Array[i];
      }  
  }
  data[0] = minElem;
  data[1] = maxElem;
  data[2] = average / amountOfElements;
  return data;
}
