#define DEBUG2 // 控制要不要輸出 DEBUG
#ifdef DEBUG
#define DebugPrint(...) Serial.print(__VA_ARGS__)       //DPRINT is a macro, debug print, __VA_ARGS__ 是 c99可變參數宏
#define DebugPrintln(...) Serial.println(__VA_ARGS__)   //DPRINTLN is a macro, debug print with new line, __VA_ARGS__ 是 c99可變參數宏
#define PrintAtResponse(...) PrintAtResponse()
#define DebugAvailable() Serial.available()
#define DebugRead() Serial.read()
#else
#define DebugAvailable(...) 0   //now defines a blank line
#define DebugPrint(...)        //now defines a blank line
#define DebugPrintln(...)      //now defines a blank line
#define PrintAtResponse(...)   //now defines a blank line
#define DebugRead(...) 0  //now defines a blank line
#endif

#define BUFFER_SZIE 256
#define SPLIT_BUFFER_SZIE 8
#define CONNECT_RETRY 3
#define CONNECT_RESP_TIMEOUT 8
#define VOLTAGE 5.00    //system voltage
#define pHPin A0
#define orpPin A3
#define temperaturePin  2
#define Offset 0.00
#define samplingInterval 20
#define pushInterval 1800000
#define ArrayLenth  40
#define commandInterval 10000
#define timeOutCheckInterval 120000

#include <OneWire.h>
#include <DallasTemperature.h>
#include <avr/wdt.h>        // 看門狗計時器函式庫
//#include <SoftwareSerial.h> // 軟體 Serial
#define NBIotSerial Serial1
//SoftwareSerial NBIotSerial(4, 5); // nb-iot serial
char responseBuffer[BUFFER_SZIE]; // at 回應暫存
char* atResponseBuffer[SPLIT_BUFFER_SZIE]; // at 回應分割字串
OneWire oneWire(temperaturePin);
DallasTemperature temperatureSensor(&oneWire);
int pHArray[ArrayLenth];
int orpArray[ArrayLenth];
int arrayIndex = 0;
unsigned long samplingTime = millis();
unsigned long pushTime = millis();
unsigned long commandTime = millis();

bool WaitATResponse(const int timeout = 10); // optional argument

void setup()
{
  NBIotSerial.begin(9600);
  while (!NBIotSerial) {}; // 等 NBIotSerial 開啟
#ifdef DEBUG
  Serial.begin(9600);
  DebugPrintln("Welcome to use SIM7020 NB-IoT board!");
#endif
  APNManualConfiguration();
  pushTime = millis();
}

void loop()
{
  static double _pHValue, _voltage;
  static double _orpValue;
  static double _Celsius;
  static bool isFirst = true;
  char temp[50];

  //採樣
  if (millis() - samplingTime > samplingInterval)
  {
    GetAllValue(&_pHValue, &_voltage, &_orpValue, &_Celsius);
    samplingTime = millis();
  }

  //推送到伺服器
  if (millis() - pushTime > pushInterval || (isFirst && millis() - pushTime > 10000))
  {
    DebugPrintln(_Celsius);
    DebugPrintln(_pHValue);
    DebugPrintln(_orpValue);
    ConnectToServer();//連結mqtt伺服器
    //傳送溫度
    NBIotSerial.print("AT+CMQPUB=0,\"sensors/temperature\",1,0,0,");
    ConvertInt(_Celsius, temp);
    NBIotSerial.print(strlen(temp));
    NBIotSerial.print(",\"");
    NBIotSerial.print(temp);
    NBIotSerial.println("\"");
    
    ClearData();//清除序列資料

    //傳送pH值
    NBIotSerial.print("AT+CMQPUB=0,\"sensors/pHValue\",1,0,0,");
    ConvertInt(_pHValue, temp);
    NBIotSerial.print(strlen(temp));
    NBIotSerial.print(",\"");
    NBIotSerial.print(temp);
    NBIotSerial.println("\"");

    ClearData();//清除序列資料

    //傳送氧化
    NBIotSerial.print("AT+CMQPUB=0,\"sensors/ORP\",1,0,0,");
    ConvertInt(_orpValue, temp);
    NBIotSerial.print(strlen(temp));
    NBIotSerial.print(",\"");
    NBIotSerial.print(temp);
    NBIotSerial.println("\"");

    ClearData();//清除序列資料

    NBIotSerial.println("AT+CMQDISCON=0");//斷開連結
    ClearData();//清除序列資料

    pushTime = millis();
    isFirst = false;
  }

  if (DebugAvailable()  > 0 )
  {
    int inByte = DebugRead();
    NBIotSerial.write(inByte);
  }

  if (NBIotSerial.available())
  {
    WaitATResponse();
  }

}

void ConnectToServer()
{
  static int retryCount = 0; // 重試次數
  retryCount++;
  if (retryCount > CONNECT_RETRY) // 手動設定 重試超過六次 呼叫 Arduino 重啟
    RebootArduino();
  DebugPrintln("\n[ConnectToServer]Start ...");
  DebugPrintln("\n[AT]");
  SendAutoRetryCommand("AT"); // AT
  DebugPrintln("\n[AT+CMQDISCON]");
  NBIotSerial.println("AT+CMQDISCON=0");
  WaitATResponse();
  DebugPrintln("\n[AT+CMQNEW]");
  SendAutoRetryCommand("AT+CMQNEW=\"220.134.220.50\",\"1883\",12000,100");
  DebugPrintln("\n[AT+CMQCON]");
  SendAutoRetryCommand("AT+CMQCON=0,3,\"sensor\",600, 0,0");
  DebugPrintln("\n[ConnectToServer] success!");
  retryCount = 0;
  pushTime = millis();
}

// clear data
void ClearData()
{
  unsigned long pushTime = millis();
  while (millis() - pushTime < 5000)
  {
    if (DebugAvailable() > 0)
    {
      int inByte = DebugRead();
      NBIotSerial.write(inByte);
    }

    while (NBIotSerial.available() > 0)
    {
      WaitATResponse();
    }
  }
}

void ConvertInt(double data, char* buf)
{
  String data_string(data);
  String target = "000000000000000000000000000000";
  char temp;
  int targetLenTemp;
  int intTemp;
  int nowCount = 0;
  for (int i = 0; i < data_string.length(); i++, nowCount += 2)
  {
    temp = data_string[i];
    intTemp = (int)temp;
    String x(intTemp, HEX);
    target[nowCount] = x[0];
    target[nowCount + 1] = x[1];
  }
  target[nowCount - 1] = '\0';
  targetLenTemp = strlen(target.c_str());
  for (int i = 0; i < targetLenTemp; i++)
  {
    buf[i] = target[i];
  }
  buf[targetLenTemp - 1] = '\0';
}

void GetAllValue(double* pHValue, double* voltage, double* orpValue, double* Celsius)
{
  pHArray[arrayIndex] = analogRead(pHPin);
  orpArray[arrayIndex++] = analogRead(orpPin);
  if (arrayIndex == ArrayLenth) arrayIndex = 0;
  *voltage = avergearray(pHArray, ArrayLenth) * VOLTAGE / 1024;
  *pHValue = GetpHVAlue(*voltage);
  *orpValue = GetOrpValue();
  *Celsius = GetTemperature();
}

double GetTemperature()
{
  temperatureSensor.requestTemperatures();
  return temperatureSensor.getTempCByIndex(0);
}

double GetOrpValue()
{
  return (((30 * (double)VOLTAGE * 1000) - (75 * avergearray(orpArray, ArrayLenth) * VOLTAGE * 1000 / 1024)) / 75 - Offset);
}

double GetpHVAlue(double voltage)
{
  return 3.5 * voltage + Offset;
}

double avergearray(int* arr, int number) {
  int i;
  int max, min;
  double avg;
  long amount = 0;
  if (number <= 0) {
    Serial.println("Error number for the array to avraging!/n");
    return 0;
  }
  if (number < 5) {
    for (i = 0; i < number; i++) {
      amount += arr[i];
    }
    avg = amount / number;
    return avg;
  }

  else {
    if (arr[0] < arr[1]) {
      min = arr[0]; max = arr[1];
    }
    else {
      min = arr[1]; max = arr[0];
    }
    for (i = 2; i < number; i++) {
      if (arr[i] < min) {
        amount += min;
        min = arr[i];
      } else {
        if (arr[i] > max) {
          amount += max;
          max = arr[i];
        } else {
          amount += arr[i];
        }
      }
    }
    avg = (double)amount / (number - 2);
  }
  return avg;
}

//  手動設定 APN
void APNManualConfiguration()
{
  static int retryCount = 0; // 重試次數
  retryCount++;
  if (retryCount > CONNECT_RETRY) // 手動設定 APN 重試超過六次 呼叫 Arduino 重啟
    RebootArduino();
  DebugPrintln("\n[APNManualConfiguration]Start ...");
  DebugPrintln("\n[AT]");
  SendAPNAutoRetryCommand("AT"); // AT
  DebugPrintln("\n[Disable RF]");
  //SendAPNAutoRetryCommand("AT+CFUN=0"); // Disable RF
  //DebugPrintln("\n[Set the APN manually]");
  SendAPNAutoRetryCommand("AT*MCGDEFCONT =\"IP\",\"internet.iot\""); // Set the APN manually
  DebugPrintln("\n[Enable RF]");
  //SendAPNAutoRetryCommand("AT+CFUN=1"); // Enable RF
  //DebugPrintln("\n[Inquiry PS service]");
  SendAPNAutoRetryCommand("AT+CGREG?"); // Inquiry PS service
  DebugPrintln("\n[Attached PS domain and got IP address automatically]");

  //NBIotSerial.println("AT+CSTT=\"internet.iot\""); // Inquiry PS service
  //NBIotSerial.println("AT+CSTT=\"AT+CIICR\""); // Inquiry PS service
  
  SendAPNAutoRetryCommand("AT+CGCONTRDP"); // Attached PS domain and got IP address automatically
  IsAPNConnect();
  DebugPrintln("\n[APNManualConfiguration] success!");
  retryCount = 0;
}

// 檢查有沒有連到 apn
bool IsAPNConnect()
{
  for (int i = 0; atResponseBuffer[i] != NULL; i++)
    for (int j = 0; atResponseBuffer[i][j] != '\0'; j++)
      if (strncmp(&atResponseBuffer[i][j], "internet.iot", 12) == 0) {
        return true;
      }
  DebugPrint("----**[APNConnectError]");
  APNManualConfiguration();
  return false;
}

// 送出自動重複 apn 指令
void SendAutoRetryCommand(const char* command)
{
  int retryCount = 0; // 重試次數
  bool isError = true;
  while (isError) {
    NBIotSerial.println(command);
    isError = WaitATResponse(CONNECT_RESP_TIMEOUT);
    retryCount++;
    if (retryCount > CONNECT_RETRY) // 重試超過次數 再次呼叫 手動設定 APN
    {
      ConnectToServer();
    }
  }
}


// 送出自動重複 apn 指令
void SendAPNAutoRetryCommand(const char* command)
{
  int retryCount = 0; // 重試次數
  bool isError = true;
  while (isError) {
    NBIotSerial.println(command);
    isError = WaitATResponse(CONNECT_RESP_TIMEOUT);
    retryCount++;
    if (retryCount > CONNECT_RETRY) // 重試超過次數 再次呼叫 手動設定 APN
    {
      APNManualConfiguration();
    }
  }
}

// 檢查有沒有 Error 字串
bool IsErrorResponse()
{
  for (int i = 0; atResponseBuffer[i] != NULL; i++)
    for (int j = 0; atResponseBuffer[i][j] != '\0'; j++)
      if (strncmp(&atResponseBuffer[i][j], "ERROR", 5) == 0) {
        DebugPrint("----**[ErrorRespArg");
        DebugPrint(i);
        DebugPrint("]:");
        DebugPrintln(atResponseBuffer[i]);
        return true;
      }
  return false;
}

#ifdef DEBUG
// 印出 at 回應
void PrintAtResponse()
{
  DebugPrint('\n');
  for (int i = 0; atResponseBuffer[i] != NULL; i++)
  {
    DebugPrint("----[RespArg");
    DebugPrint(i);
    DebugPrint("]:");
    DebugPrintln(atResponseBuffer[i]);
  }
}
#endif

// 等待 AT 回應
bool WaitATResponse(const int timeout = 10)
{
  memset(responseBuffer, '\0', BUFFER_SZIE * sizeof(responseBuffer[0])); // 清空暫存
  memset(atResponseBuffer, NULL, SPLIT_BUFFER_SZIE * sizeof(atResponseBuffer[0]));
  int delayCount = 0;
  while (NBIotSerial.available() == 0)  // 檢測 timeout
  {
    delay(100);
    delayCount++;
    if (timeout != -1 && delayCount > timeout * 10)
    {
      DebugPrintln("\n--**[WaitATResponse] Response timeout!");
      return true;
    }
  }
  delay(1000); // 確保字元傳輸完成
  int index = 0;
  while (NBIotSerial.available() > 0) // 讀入整串回應
    responseBuffer[index++] = NBIotSerial.read();
  for (int i = 0; responseBuffer[i] != '\0' && i < BUFFER_SZIE; i++) // 濾除換行
    if ((int)(responseBuffer[i]) == 10 || (int)(responseBuffer[i]) == 13 || responseBuffer[i] == '\n') {
      for (int j = i--; responseBuffer[j] != '\0' && j + 1 < BUFFER_SZIE; j++)
        responseBuffer[j] = responseBuffer[j + 1];
    }
  bool isNeedSplit = false;
  for (int i = 0; responseBuffer[i] != '\0' && i < BUFFER_SZIE; i++)
    if (responseBuffer[i] == '+')
      isNeedSplit = true;
  if (isNeedSplit)
    SplitAtResponse(); // 切割 at
  else
    atResponseBuffer[0] = &responseBuffer[0];
  PrintAtResponse();
  return IsErrorResponse();
}

// 切割 at 指令
void SplitAtResponse()
{
  int index = 0;
  char *ptr = strtok(responseBuffer, ":");
  while (ptr != NULL)
  {
    atResponseBuffer[index] = ptr;
    index++;
    ptr = strtok(NULL, ",");
    if (*ptr == ' ') ptr++;
  }
}

// 利用不回應看門狗計時器, 強制 Arduino 重開
void RebootArduino()
{
  DebugPrintln("\n[RebootArduino] Reboot ...");
  delay(2000);
  wdt_enable(WDTO_15MS); // 設定看門狗為 15ms 回應時間
  delay(10000); // delay 10s 不回應看門狗
}
