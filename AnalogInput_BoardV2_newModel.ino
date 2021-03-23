

#include <Wire.h>
#include <Adafruit_ADS1015.h>

#include "HardwareSerial_NB_BC95.h"

//#include <ArduinoJson.h>


String serverIP = "103.27.203.83"; // Your Server IP;
String serverPort = "9956"; // Your Server Port;
String json = "";



//BluetoothSerial SerialBT;

HardwareSerial_NB_BC95 AISnb;

const long interval = 20000;  //millisecond
unsigned long previousMillis = 0;

signal meta ;


Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */

int16_t adc0, adc1, adc2, adc3;
void _init() {



  Serial.println("Start...");

  AISnb.setupDevice(serverPort);

  String ip1 = AISnb.getDeviceIP();
  delay(1000);

  pingRESP pingR = AISnb.pingIP(serverIP);


  String nccid = AISnb.getNCCID();
  Serial.print("nccid:");
  Serial.println(nccid);




}
void setup(void)
{
  Serial.begin(115200);
  Serial.println("Hello!");
  //  SerialBT.begin("AnalogInput_BoardV2"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
  _init();
  ads.begin();
  AISnb.debug = false;


  previousMillis = millis();

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
    udpData2.concat(",\"rssi\":");
    udpData2.concat(meta.rssi);
    udpData2.concat("}");
    Serial.println(udpData2);

    UDPSend udp = AISnb.sendUDPmsgStr(serverIP, serverPort, udpData2);

    previousMillis = currentMillis;
 

    UDPReceive resp = AISnb.waitResponse();
    AISnb.receive_UDP(resp);
    Serial.print("waitData:");
    Serial.println(resp.data);


    previousMillis = currentMillis;
  }



}
