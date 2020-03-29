/*
 *Netmedias
 *https://github.com/nassir-malik/IOT-Smart-Temperature-Switch-With-Alexa-NodeMcu
 *  Created on: 09.19.2017
 *  
 */
#include <DHT.h>
#include <ArduinoJson.h> 
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <Hash.h>
#define DHTTYPE DHT11
// @@@@@@@@@@@@@@@ You only need to modify wi-fi and domain info @@@@@@@@@@@@@@@@@@@@
const char* ssid     = "Freebox_Saga_RDC"; //enter your ssid/ wi-fi(case sensitive) router name - 2.4 Ghz only
const char* password = "eYWq<3NM(XX|I}4=uIcxJqZ5*IXL;8{%)E=~P3jyA8QkX%&a;Cl>.+bmMI";     // enter ssid password (case sensitive)
char host[] = "humidityregulation.herokuapp.com"; //- better your Heroku domain name like  "smarttempswitch.herokuapp.com" 
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#define DHTPIN 2 
const int relayPin = LED_BUILTIN;
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266
int port = 80;
char path[] = "/ws"; 
ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
DynamicJsonBuffer jsonBuffer; 
String currState;
int pingCount = 0;
String triggerName ="";
String triggerVal ="";
int triggerEnabled = 0;
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) { //uint8_t *


    switch(type) {
        case WStype_DISCONNECTED:
           Serial.println("Disconnected! ");
           Serial.println("Connecting...");
               webSocket.begin(host, port, path);
               webSocket.onEvent(webSocketEvent);
            break;
            
        case WStype_CONNECTED:
            {
             Serial.println("Connected! ");
          // send message to server when Connected
            webSocket.sendTXT("Connected");
            }
            break;
            
        case WStype_TEXT:
            Serial.println("Got data");
              //data = (char*)payload;
           processWebScoketRequest((char*)payload);
            break;
            
        case WStype_BIN:

            hexdump(payload, length);
            Serial.print("Got bin");
            // send data to server
            webSocket.sendBIN(payload, length);
            break;
    }

}

void setup() {
    dht.begin();           // initialize temperature sensor
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    
    //pinMode(relayPin, OUTPUT);
    
      for(uint8_t t = 4; t > 0; t--) {
          delay(1000);
      }
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    
    //Serial.println(ssid);
    WiFiMulti.addAP(ssid, password);

    //WiFi.disconnect();
    while(WiFiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
        delay(100);
    }
    Serial.println("Connected to wi-fi");
    webSocket.begin(host, port, path);
    webSocket.onEvent(webSocketEvent);

}

void loop() {
    getTemp();
    webSocket.loop();

    if (triggerEnabled==1){
      setTrigger(triggerName,triggerVal);
    }
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  //If you make change to delay make sure adjust the ping
    delay(2000);
	// make sure after every 40 seconds send a ping to Heroku
	//so it does not terminate the websocket connection
	//This is to keep the conncetion alive between ESP and Heroku
    	if (pingCount > 20) {
    		pingCount = 0;
    		webSocket.sendTXT("\"heartbeat\":\"keepalive\"");
    	}else {pingCount += 1;}
  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
}

void processWebScoketRequest(String data){
            String jsonResponse = "{\"version\": \"1.0\",\"sessionAttributes\": {},\"response\": {\"outputSpeech\": {\"type\": \"PlainText\",\"text\": \"<text>\"},\"shouldEndSession\": true}}";
            JsonObject& root = jsonBuffer.parseObject(data);
            String query = root["query"];
            String message="";
            Serial.println(data);
            
            if(query == "cmd"){ //if query check state
                      String value = root["value"];  
                      Serial.println("Recieved command!");
                    if(value=="on"){
                      digitalWrite(relayPin, HIGH);
                      message = "{\"state\":\"ON\"}";
                      currState = "ON";
                    }else if (value=="off"){
                      digitalWrite(relayPin, LOW);
                      message = "{\"state\":\"OFF\"}";
                      currState = "OFF";
                    }else if (value=="deactivate"){
                      //deactivate trigger
                      triggerEnabled = 0;
                    }else{
                      String object = root["object"];
                      //set trigger for temp and humidity
                      triggerName = object;
                      triggerVal = value;
                      triggerEnabled = 1;
                      
                        
                    }
                    jsonResponse.replace("<text>", "It is done");
                  
            }else if(query == "?"){ //if command then execute   
              Serial.println("Recieved query!");
              int state = digitalRead(relayPin);
              String value = root["value"];
              //Serial.print("Value-->");
              //Serial.print(value);
                if(value=="switch"){
                 if(currState=="ON"){
                      message = "{\"state\":\"ON\"}";
                    }else{
                      message = "{\"state\":\"OFF\"}";
                    }
                }else if(value=="humidité"){
                  //response with current humidity DHT.humidity
                  Serial.println("Humidity response...");
                  jsonResponse.replace("<text>", "L'humidité est de " + String(dht.readHumidity()) + " percent");
                  
                }else if(value=="température"){  
                  //response with current temperature DHT.temperature /Celcius2Fahrenheit(DHT.temperature)
                  Serial.println("Temp response...");
                  jsonResponse.replace("<text>", "La température est de " + String(dht.readTemperature())+ " celsius");
                }
            }else{//can not recognized the command
              Serial.println("Command is not recognized!");
            }
            //jsonResponse.replace("<text>", "Garage door " + instance + " is " + message );
            Serial.print("Sending response back");
            Serial.println(jsonResponse);
                  // send message to server
                  webSocket.sendTXT(jsonResponse);
                  if(query == "cmd" || query == "?"){webSocket.sendTXT(jsonResponse);}
}

void setTrigger(String obj, String val){
  Serial.print("Trigger is set for ");
  Serial.print(val.toFloat());
  Serial.print(" ");
  Serial.print(triggerName);
  Serial.println("");
  
/*  if(String("fahrenheit") == obj){
    if(dht.readTemperature(true))>=val.toFloat()){
      Serial.println("Fahrenheit trigger on!");
        digitalWrite(relayPin, HIGH);
      }else{
        digitalWrite(relayPin, LOW);
        }
  }else if(String("celsius") == obj){
    //Celcius2Fahrenheit(DHT.temperature)
        if(dht.readTemperature()>=val.toFloat()){
          Serial.println("Celcius trigger on!");
            digitalWrite(relayPin, HIGH);
          }else{
            digitalWrite(relayPin, LOW);
          }
  }else{
    //DHT.humidity
        if(dht.readHumidity()>=val.toFloat()){
          Serial.println("Humidity trigger on!");
          digitalWrite(relayPin, HIGH);
          }else{digitalWrite(relayPin, LOW);
          }
  }*/
}


void getTemp(){
    //DHT.read11(dht_dpin);
    Serial.print("Current humidity = ");
    Serial.print(dht.readHumidity());
    Serial.print("%  ");
    Serial.print("temperature = ");
    Serial.print(dht.readTemperature()); 
    Serial.print("C  ");
    Serial.print(dht.readTemperature(true)); 
    Serial.println("F  ");
  delay(800);//Don't try to access too frequently... in theory
  //should be once per two seconds, fastest,
  //but seems to work after 0.8 second.
}

double Celcius2Fahrenheit(double celsius){return celsius * 9 / 5 + 32;}
