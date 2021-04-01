
#include <ArduinoJson.h>

#include <EEPROM.h>

#include <Wire.h>
#include <Adafruit_ADS1015.h>

#include "HardwareSerial_NB_BC95.h"

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

const long intervalRestart = 24 * 60000 *60; //millisecond
//const long intervalRestart =  60000 *60; //millisecond


String _config = "{\"_type\":\"retrattr\",\"Tn\":\"vpbHN0ulNCCptps6qRAk\",\"keys\":[\"epoch\",\"ip\"]}";

String nccid = "";
String serverIP = "103.27.203.83"; // Your Server IP;
String serverPort = "19956"; // Your Server Port;
String json = "";


//BluetoothSerial SerialBT;

HardwareSerial_NB_BC95 AISnb;

const long interval = 60000;  //millisecond
unsigned long previousMillis = 0;

unsigned long _epoch = 0;
String _IP = "";
String dataJson = "";
StaticJsonDocument<400> doc;
boolean validEpoc = false;


signal meta ;


Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */

int16_t adc0, adc1, adc2, adc3;

void _writeEEPROM(String data) {


  Serial.print("Writing Data:");
  Serial.println(data);

  writeString(10, data);  //Address 10 and String type data
  delay(10);
}

void _loadConfig() {
  serverIP = read_String(10);
  Serial.print("IP:");
  Serial.println(serverIP);
}


char  char_to_byte(char c)
{
  if ((c >= '0') && (c <= '9'))
  {
    return (c - 0x30);
  }
  if ((c >= 'A') && (c <= 'F'))
  {
    return (c - 55);
  }
}
void _init() {

  Serial.println(_config);

  do {
    UDPSend udp = AISnb.sendUDPmsgStr(serverIP, serverPort, _config);
    dataJson = "";
    nccid = AISnb.getNCCID();
    Serial.print("nccid:");
    Serial.println(nccid);


    UDPReceive resp = AISnb.waitResponse();
    AISnb.receive_UDP(resp);
    Serial.print("waitData:");
    Serial.println(resp.data);


    for (int x = 0; x < resp.data.length(); x += 2)
    {
      char c =  char_to_byte(resp.data[x]) << 4 | char_to_byte(resp.data[x + 1]);

      dataJson += c;
    }
    Serial.println(dataJson);
    DeserializationError error = deserializeJson(doc, dataJson);

    // Test if parsing succeeds.
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      validEpoc = true;
      delay(4000);
    } else {
      validEpoc = false;
      unsigned long epoch = doc["epoch"];
      _epoch = epoch;
      String ip = doc["ip"];
      _IP = ip;
       Serial.println(dataJson);
      Serial.print("epoch:");  Serial.println(_epoch);
      _writeEEPROM(_IP);
      Serial.println(_IP);

    }

  } while (validEpoc);


}


void writeString(char add, String data)
{
  int _size = data.length();
  int i;
  for (i = 0; i < _size; i++)
  {
    EEPROM.write(add + i, data[i]);
  }
  EEPROM.write(add + _size, '\0'); //Add termination null character for String Data
  EEPROM.commit();
}


String read_String(char add)
{
  int i;
  char data[100]; //Max 100 Bytes
  int len = 0;
  unsigned char k;
  k = EEPROM.read(add);
  while (k != '\0' && len < 500) //Read until null character
  {
    k = EEPROM.read(add + len);
    data[len] = k;
    len++;
  }
  data[len] = '\0';

  return String(data);
}

void setup(void)
{
  Serial.begin(115200);
  EEPROM.begin(512);
  Serial.println("Load config...");
  SerialBT.begin("ENTECH_NBAnalog_001"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");

  ads.begin();
  AISnb.debug = false;


  AISnb.setupDevice(serverPort);

  String ip1 = AISnb.getDeviceIP();
  delay(1000);

  pingRESP pingR = AISnb.pingIP(serverIP);
  previousMillis = millis();
  _init();
  _loadConfig();
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  if ((in_max - in_min) + out_min != 0) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  } else {
    return 0;
  }
}

void loop(void)
{





  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    adc0 = ads.readADC_SingleEnded(0);
    adc1 = ads.readADC_SingleEnded(1);
    adc2 = ads.readADC_SingleEnded(2);
    adc3 = ads.readADC_SingleEnded(3);

    Serial.print(adc0); Serial.print(" ");  Serial.print(adc1); Serial.print(" ");   Serial.print(adc2); Serial.print(" ");  Serial.println(adc3);
    float val0 = mapfloat(adc0, 2128, 10560, 4, 20);
    float val1 = mapfloat(adc1, 2128, 10560, 4, 20);
    float val2 = mapfloat(adc2, 2128, 10560, 4, 20);
    float val3 = mapfloat(adc3, 2128, 10560, 4, 20);

    meta = AISnb.getSignal();
    String udpData2 = "{\"Tn\":\"vpbHN0ulNCCptps6qRAk\",\"adc0\":";
    udpData2.concat(val0);
    udpData2.concat(",\"adc1\":");
    udpData2.concat(val1);
    udpData2.concat(",\"adc2\":");
    udpData2.concat(val2);
    udpData2.concat(",\"adc3\":");
    udpData2.concat(val3);
    udpData2.concat(",\"nccid\":");
    udpData2.concat(nccid);
    udpData2.concat(",\"rssi\":");
    udpData2.concat(meta.rssi);
    udpData2.concat("}");
    Serial.println(udpData2);
    SerialBT.println(udpData2);
    UDPSend udp = AISnb.sendUDPmsgStr(serverIP, serverPort, udpData2);

    previousMillis = currentMillis;


    UDPReceive resp = AISnb.waitResponse();
    AISnb.receive_UDP(resp);
    Serial.print("waitData:");
    SerialBT.println("waitData:");

    Serial.println(resp.data);
    SerialBT.println(resp.data);


    previousMillis = currentMillis;
  }

  if (currentMillis   >= intervalRestart)
  {
    ESP.restart();
  }

}
