/*
 Version 1.1 - 12.11.18
 Copyright CHANG CHUN HAO All rights reserved.
 Sinric ESP8266 RGB LED
*/  

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h> //  https://github.com/kakopappa/sinric/wiki/How-to-add-dependency-libraries
#include <ArduinoJson.h> // https://github.com/kakopappa/sinric/wiki/How-to-add-dependency-libraries
#include <StreamString.h>

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
WiFiClient client;

#define MyApiKey "c300f1c8-bc23-4f9f-9d0b-1515d5e2ec2a" // TODO: Change to your sinric API Key. Your API Key is displayed on sinric.com dashboard
#define MySSID "***********" // TODO: Change to your Wifi network SSID
#define MyWifiPassword "*********" // TODO: Change to your Wifi network password

#define HEARTBEAT_INTERVAL 300000 // 5 Minutes 

uint64_t heartbeatTimestamp = 0;
bool isConnected = false;

const PROGMEM uint8_t RGB_LIGHT_RED_PIN = 5;
const PROGMEM uint8_t RGB_LIGHT_GREEN_PIN = 4;
const PROGMEM uint8_t RGB_LIGHT_BLUE_PIN = 0;

uint8_t m_rgb_brightness = 100;
uint8_t m_rgb_red = 255;
uint8_t m_rgb_green = 255;
uint8_t m_rgb_blue = 255;

void setup() {
  Serial.begin(115200);

  pinMode(RGB_LIGHT_BLUE_PIN, OUTPUT);
  pinMode(RGB_LIGHT_RED_PIN, OUTPUT);
  pinMode(RGB_LIGHT_GREEN_PIN, OUTPUT);
  analogWriteRange(255);
  setColor(0, 0, 0);
  
  WiFiMulti.addAP(MySSID, MyWifiPassword);
  Serial.println();
  Serial.print("Connecting to Wifi: ");
  Serial.println(MySSID);  

  // Waiting for Wifi connect
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if(WiFiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi connected. ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  // server address, port and URL
  webSocket.begin("iot.sinric.com", 80, "/"); //"iot.sinric.com", 80

  // event handler
  webSocket.onEvent(webSocketEvent);
  webSocket.setAuthorization("apikey", MyApiKey);
  
  // try again every 5000ms if connection has failed
  webSocket.setReconnectInterval(5000);   // If you see 'class WebSocketsClient' has no member named 'setReconnectInterval' error update arduinoWebSockets
}

void loop() {
  webSocket.loop();
  if(isConnected) {
      uint64_t now = millis();
      // Send heartbeat in order to avoid disconnections during ISP resetting IPs over night. Thanks @MacSass
      if((now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
          heartbeatTimestamp = now;
          webSocket.sendTXT("H");          
      }
  }   
}
 
void turnOn(String deviceId) {
  if (deviceId == "5c06963584e7f32472fae0ab") // Device ID of first device
  {  
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
    setColor(m_rgb_red, m_rgb_green, m_rgb_blue);
  } 
  else if (deviceId == "") // Device ID of second device
  { 
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
  }
  else {
    Serial.print("Turn on for unknown device id: ");
    Serial.println(deviceId);    
  }     
}

void turnOff(String deviceId) {
   if (deviceId == "5c06963584e7f32472fae0ab") // Device ID of first device
   {  
     Serial.print("Turn off Device ID: ");
     Serial.println(deviceId);
     setColor(0,0,0);
   }
   else if (deviceId == "") // Device ID of second device
   { 
     Serial.print("Turn off Device ID: ");
     Serial.println(deviceId);
  }
  else {
     Serial.print("Turn off for unknown device id: ");
     Serial.println(deviceId);    
  }
}


void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      isConnected = false;    
      Serial.printf("[WSc] Webservice disconnected from sinric.com!\n");
      break;
    case WStype_CONNECTED: {
      isConnected = true;
      Serial.printf("[WSc] Service connected to sinric.com at url: %s\n", payload);
      Serial.printf("Waiting for commands from sinric.com ...\n");        
      }
      break;
    case WStype_TEXT: {
        Serial.printf("[WSc] get text: %s\n", payload);
        // Example payloads

        // For Switch  types
        // {"deviceId":"xxx","action":"action.devices.commands.OnOff","value":{"on":true}} // https://developers.google.com/actions/smarthome/traits/onoff
        // {"deviceId":"xxx","action":"action.devices.commands.OnOff","value":{"on":false}}

        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject((char*)payload); 
        String deviceId = json ["deviceId"];     
        String action = json ["action"];
        
        if(action == "action.devices.commands.OnOff") { // Switch 
            String value = json ["value"]["on"];
            Serial.println(value); 
            if(value == "true") {
                turnOn(deviceId);
            } else {
                turnOff(deviceId);
            }
        }
        else if(action == "action.devices.commands.BrightnessAbsolute"){
            uint8_t value_brightness = json ["value"]["brightness"];
            Serial.println(value_brightness);
            if (deviceId == "5c06963584e7f32472fae0ab"){
              if (value_brightness<0 || value_brightness>100){
                //Do nothing
                return;
              }
              else{
                m_rgb_brightness = value_brightness;
                setColor(m_rgb_red, m_rgb_green, m_rgb_blue);
              }
            }
        }
        else if(action == "action.devices.commands.ColorAbsolute") {
          if (deviceId == "5c06963584e7f32472fae0ab"){
          String value_color_name = json ["value"]["color"]["name"];
          long value_color_spectrumRGB = json ["value"]["color"]["spectrumRGB"];
          Serial.println(value_color_name);
          Serial.println(value_color_spectrumRGB);
          int number = dectohex(value_color_spectrumRGB); //convert decimal to heximal than to 255
          decconvert(value_color_spectrumRGB);
          int rgb_red = number >> 16; //take out red value
          if (rgb_red < 0 || rgb_red > 255) {
            return;
            } 
            else {
              m_rgb_red = rgb_red;
              }
           int rgb_green = number >> 8 & 0xFF; //take out green value
           if (rgb_green < 0 || rgb_green > 255){
            return;
           }
           else{
            m_rgb_green = rgb_green;
           }
           int rgb_blue = number & 0xFF; //take out blue value
           if (rgb_blue < 0 || rgb_blue > 255){
            return;
           }
           else {
            m_rgb_blue = rgb_blue;
           }
           setColor(m_rgb_red, m_rgb_green, m_rgb_blue);
          }
        }
        else if (action == "test") {
            Serial.println("[WSc] received test command from sinric.com");
        }
      }
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      break;
  }
}

void decconvert(unsigned int numberToPrint) {
  if (numberToPrint >= 16){
    decconvert(numberToPrint / 16);
  }
  Serial.print("0123456789ABCDEF"[numberToPrint % 16]);
}

void setColor(uint8_t p_red, uint8_t p_green, uint8_t p_blue) {
  // convert the brightness in % (0-100%) into a digital value (0-255)
  uint8_t brightness = map(m_rgb_brightness, 0, 100, 0, 255);

  analogWrite(RGB_LIGHT_RED_PIN, map(p_red, 0, 255, 0, brightness));
  analogWrite(RGB_LIGHT_GREEN_PIN, map(p_green, 0, 255, 0, brightness));
  analogWrite(RGB_LIGHT_BLUE_PIN, map(p_blue, 0, 255, 0, brightness));
}

int dectohex(long decnumber){
  char hexstring [6];
  itoa (decnumber,hexstring,16);
  int number = (int) strtol( &hexstring[0], NULL, 16); //format: 255255255
  return number;
}



