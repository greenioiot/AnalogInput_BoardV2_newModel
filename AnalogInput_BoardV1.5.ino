#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
#include <ArduinoJson.h>

#include <EEPROM.h>

#include <ModbusMaster.h>
 
#include <HardwareSerial.h>
#include "HardwareSerial_NB_BC95.h"


#include <TaskScheduler.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif


#include <ArduinoOTA.h>
#include <WiFi.h>
#include <Wire.h>

BluetoothSerial SerialBT;
#define _TASK_TIMECRITICAL


const long interval = 1000;  //millisecond
unsigned long previousMillis = 0;

String dataJson = "";

 
//###################### replace for new device ################
String deviceToken = "";

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */

int16_t adc0, adc1, adc2, adc3;

void _init() {



}


/**********************************************  WIFI Client 注意编译时要设置此值 *********************************
   wifi client
*/
const char* ssid = "greenio"; //replace "xxxxxx" with your WIFI's ssid
const char* password = "green7650"; //replace "xxxxxx" with your WIFI's password

//WiFi&OTA 参数
String HOSTNAME = "greenio-";
#define PASSWORD "green7650" //the password for OTA upgrade, can set it in any char you want

/************************************************  注意编译时要设置此值 *********************************
   是否使用静态IP
*/
#define USE_STATIC_IP false
#if USE_STATIC_IP
IPAddress staticIP(192, 168, 1, 22);
IPAddress gateway(192, 168, 1, 9);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(8, 8, 8, 8);
IPAddress dns2(114, 114, 114, 114);
#endif

void setupOTA()
{
  //Port defaults to 8266
  //ArduinoOTA.setPort(8266);

  //Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(HOSTNAME.c_str());

  //No authentication by default
  ArduinoOTA.setPassword(PASSWORD);

  //Password can be set with it's md5 value as well
  //MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  //ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]()
  {
    Serial.println("Start Updating....");
    SerialBT.println("Start Updating....");

    SerialBT.printf("Start Updating....Type:%s\n", (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem");

    Serial.printf("Start Updating....Type:%s\n", (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem");
  });

  ArduinoOTA.onEnd([]()
  {

    SerialBT.println("Update Complete!");
    Serial.println("Update Complete!");


    ESP.restart();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
  {
    String pro = String(progress / (total / 100)) + "%";
    //    int progressbar = (progress / (total / 100));
    //int progressbar = (progress / 5) % 100;
    //int pro = progress / (total / 100);


    SerialBT.printf("Progress: %u%%\n", (progress / (total / 100)));

    Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error)
  {
    Serial.printf("Error[%u]: ", error);
    String info = "Error Info:";
    switch (error)
    {
      case OTA_AUTH_ERROR:
        info += "Auth Failed";
        Serial.println("Auth Failed");
        break;

      case OTA_BEGIN_ERROR:
        info += "Begin Failed";
        Serial.println("Begin Failed");
        break;

      case OTA_CONNECT_ERROR:
        info += "Connect Failed";
        Serial.println("Connect Failed");
        break;

      case OTA_RECEIVE_ERROR:
        info += "Receive Failed";
        Serial.println("Receive Failed");
        break;

      case OTA_END_ERROR:
        info += "End Failed";
        Serial.println("End Failed");
        break;
    }


    Serial.println(info);
    ESP.restart();
  });

  ArduinoOTA.begin();
}

void setupWIFI()
{

  Serial.println("Connecting...");
  Serial.println(String(ssid));


  //连接WiFi，删除旧的配置，关闭WIFI，准备重新配置
  WiFi.disconnect(true);
  delay(1000);

  WiFi.mode(WIFI_STA);
  //WiFi.onEvent(WiFiEvent);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);    //断开WiFi后自动重新连接,ESP32不可用
  WiFi.setHostname(HOSTNAME.c_str());
  WiFi.begin(ssid, password);
#if USE_STATIC_IP
  WiFi.config(staticIP, gateway, subnet);
#endif

  //等待5000ms，如果没有连接上，就继续往下
  //不然基本功能不可用
  byte count = 0;
  while (WiFi.status() != WL_CONNECTED && count < 10)
  {
    count ++;
    delay(500);
    Serial.print(".");
  }


  if (WiFi.status() == WL_CONNECTED)
    Serial.println("Connecting...OK.");
  else
    Serial.println("Connecting...Failed");

}


String getMacAddress() {
    uint8_t baseMac[6];
    // Get MAC address for WiFi station
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    char baseMacChr[18] = {0};
    sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
    return String(baseMacChr);
}

void setup(void)
{
  Serial.begin(115200);

  Serial.println("Load config...");
  SerialBT.begin("Analog_001"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");

  ads.begin();

  previousMillis = millis();
 
   
  
  HOSTNAME.concat(getMacAddress());
  SerialBT.begin(HOSTNAME); //Bluetooth


  Serial.println("Initialize...");


  setupWIFI();
  setupOTA();

}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  if ((in_max - in_min) + out_min != 0) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  } else {
    return 0;
  }
}

void waitforOTA() {

  ArduinoOTA.handle();
  unsigned long ms = millis();
  if (ms % 60000 == 0)
  {
    Serial.println("Waiting for，OTA now");
    SerialBT.println("Waiting for, OTA now");
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

    if (val0 < 0) val0 = 0;
    if (val1 < 0) val1 = 0;
    if (val2 < 0) val2 = 0;
    if (val3 < 0) val3 = 0;

    String udpData2 = "{\"Tn\":\"";

    udpData2.concat(HOSTNAME);
    udpData2.concat("\",\"A4\":");
    udpData2.concat(val0);
    udpData2.concat(",\"A3\":");
    udpData2.concat(val1);
    udpData2.concat(",\"A2\":");
    udpData2.concat(val2);
    udpData2.concat(",\"A1\":");
    udpData2.concat(val3);

    udpData2.concat("}");
    Serial.println(udpData2);
    SerialBT.println(udpData2);

    previousMillis = currentMillis;

  }

  waitforOTA();
}


/****************************************************
   [通用函数]ESP32 WiFi Kit 32事件处理
*/
void WiFiEvent(WiFiEvent_t event)
{
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch (event)
  {
    case SYSTEM_EVENT_WIFI_READY:               /**< ESP32 WiFi ready */
      break;
    case SYSTEM_EVENT_SCAN_DONE:                /**< ESP32 finish scanning AP */
      break;

    case SYSTEM_EVENT_STA_START:                /**< ESP32 station start */
      break;
    case SYSTEM_EVENT_STA_STOP:                 /**< ESP32 station stop */
      break;

    case SYSTEM_EVENT_STA_CONNECTED:            /**< ESP32 station connected to AP */
      break;

    case SYSTEM_EVENT_STA_DISCONNECTED:         /**< ESP32 station disconnected from AP */
      break;

    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:      /**< the auth mode of AP connected by ESP32 station changed */
      break;

    case SYSTEM_EVENT_STA_GOT_IP:               /**< ESP32 station got IP from connected AP */
    case SYSTEM_EVENT_STA_LOST_IP:              /**< ESP32 station lost IP and the IP is reset to 0 */
      break;

    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:       /**< ESP32 station wps succeeds in enrollee mode */
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:        /**< ESP32 station wps fails in enrollee mode */
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:       /**< ESP32 station wps timeout in enrollee mode */
    case SYSTEM_EVENT_STA_WPS_ER_PIN:           /**< ESP32 station wps pin code in enrollee mode */
      break;

    case SYSTEM_EVENT_AP_START:                 /**< ESP32 soft-AP start */
    case SYSTEM_EVENT_AP_STOP:                  /**< ESP32 soft-AP stop */
    case SYSTEM_EVENT_AP_STACONNECTED:          /**< a station connected to ESP32 soft-AP */
    case SYSTEM_EVENT_AP_STADISCONNECTED:       /**< a station disconnected from ESP32 soft-AP */
    case SYSTEM_EVENT_AP_PROBEREQRECVED:        /**< Receive probe request packet in soft-AP interface */
    case SYSTEM_EVENT_AP_STA_GOT_IP6:           /**< ESP32 station or ap interface v6IP addr is preferred */
      break;

    case SYSTEM_EVENT_ETH_START:                /**< ESP32 ethernet start */
    case SYSTEM_EVENT_ETH_STOP:                 /**< ESP32 ethernet stop */
    case SYSTEM_EVENT_ETH_CONNECTED:            /**< ESP32 ethernet phy link up */
    case SYSTEM_EVENT_ETH_DISCONNECTED:         /**< ESP32 ethernet phy link down */
    case SYSTEM_EVENT_ETH_GOT_IP:               /**< ESP32 ethernet got IP from connected AP */
    case SYSTEM_EVENT_MAX:
      break;
  }
}
