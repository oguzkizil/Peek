#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <NewPing.h>

#include "FS.h"
#include "DHTesp.h"
//#include "index.h" //Our HTML webpage contents with javascripts

int LED = 16; //D0
String LedS = ""; //ledState holder

int s1trig = 5; //D1
int s1echo = 4; //D2
int s2trig = 0; //D3
int s2echo = 2; //D4

DHTesp dht; 
int dhtPin = 14; //D5

#define ACTIVATION_DISTANCE 60 
#define MAX_DISTANCE 200 //Maximum distance (in cm) to ping.

int person = 0;
const char* ssid = "Reburn";
const char* password = "01238520";
//const char* ssid = "TTNET_ZyXEL_EMY4";
//const char* password = "4824f669d5099";
ESP8266WebServer server(80); //Server on port 80

//===============================================================
// This routine is executed when you open its IP in browser // Server Responses
//===============================================================
void handleRoot() {
 //String s = MAIN_page; //Read HTML contents
 //server.send(200, "text/html", s); //Send web page
 File file = SPIFFS.open("/index.html","r");
 server.streamFile(file,"text/html");
 file.close();
}

//Default mode unless mode set
void ManuelMode() {
 String ledState = "OFF";
 String t_state = server.arg("LEDstate"); //Refer  xhttp.open("GET", "setLED?LEDstate="+led, true);
 Serial.println(t_state);
 if(t_state == "1")
 {
  digitalWrite(LED,HIGH); //LED ON
  ledState = "ON"; //Feedback parameter
 }
 else
 {
  digitalWrite(LED,LOW); //LED OFF
  ledState = "OFF"; //Feedback parameter  
 }
 
 server.send(200, "text/plane", ledState); //Send web page
}

void setPerson() {
 String pNumber = String(person);
 server.send(200, "text/plane", pNumber); //Send person number only to client ajax request
}
/*
void checkLED() {
  AutoMode();
 String tempPerson = String(person);
 server.send(200, "text/plane", tempPerson); //Send tempNum
}

void AutoMode(){
   if (person > 0) {
       LedS="ON";
       //checkLED();
       //digitalWrite(LED,HIGH); //LED ON
    } else {
       LedS="OFF";
       //checkLED(); 
       //digitalWrite(LED,LOW); //LED OFF
    }
}*/

void handleTempature() {
 TempAndHumidity lastValues = dht.getTempAndHumidity();
 String tempature = String(lastValues.temperature,0);
 
 server.send(200, "text/plane", tempature); //Send tempature value only to client ajax request
}

void handleHumidity() {
 TempAndHumidity lastValues = dht.getTempAndHumidity();
 String humidity = String(lastValues.humidity,0);
 
 server.send(200, "text/plane", humidity); //Send humidity value only to client ajax request
}

//==============================================================
//                  SENSOR SETUP
//==============================================================
class USSensor {
public:
  USSensor(int trigPin, int echoPin, String aName) {
    trig = trigPin;
    echo = echoPin;
    sonar = new NewPing(trigPin, echoPin, MAX_DISTANCE);
    isActivated = false;
    name = aName;
    time = 0;
  }
  
  void update() {
    updateSensor();
    if (value < ACTIVATION_DISTANCE) {
      setActive(true);
    } else {
      if (millis() - time > 500) {
        setActive(false);
      } 
    }
  }
  
  void reset() {
    setActive(false);
    time = 0;
  }
  
  void setActive(boolean state) {
    if (!isActivated && state) {
      time = millis();
    }
    if (isActivated != state) {
      isActivated = state;
      Serial.print(name + ": ");
      Serial.println(state);
    }
  }
  
  boolean wasActive() {
    return (time != 0) && !isActivated;
  }
  
  int lastActive() {
    return millis() - time;
  }
  
private:
  int trig;
  int echo;
  String name;
  boolean isActivated;
  unsigned long time;
  float value;
  NewPing *sonar;
  
  void updateSensor() {
    
    delay(50);
    value = sonar->ping_cm();
   // Serial.print(name + ": ");
     // Serial.println(value);
  }
};

USSensor *A;
USSensor *B;

//==============================================================
//                  SETUP
//==============================================================
void setup(void){
  SPIFFS.begin();
  Serial.begin(115200);
  
  A = new USSensor(s1trig, s1echo, "A");
  B = new USSensor(s2trig, s2echo, "B");

  // LED port Direction output
  pinMode(LED,OUTPUT); 
  digitalWrite(LED, LOW);
  
  dht.setup(dhtPin, DHTesp::DHT11);
   
  Serial.print("Connecting to the Newtork");
  WiFi.begin(ssid, password);     //Connect to your WiFi router
  Serial.println("");
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
 
  server.on("/", handleRoot);      
  server.on("/setLED", ManuelMode); 
  server.on("/getPerson",setPerson);
  server.on("/getTempature", handleTempature);
  server.on("/getHumidity",handleHumidity);
 // server.on("/checkLED",checkLED);

  
  server.begin();                  //Start server
  Serial.println("HTTP server started");
}
//==============================================================
//                     LOOP
//==============================================================
void loop(void){
    A->update();
    B->update(); 
    
    if (A->wasActive() && B->wasActive()) {
      int a_time = A->lastActive();
      int b_time = B->lastActive();
      if (a_time < 3000 && b_time < 3000) {
        if (a_time > b_time) {
          person++;
        } else {
          person--;
        }
        Serial.print("People in room: ");
        Serial.println(person);
        setPerson();
      }
      A->reset();
      B->reset();
    }

    if (person < 0) { 
      person = 0;
    }
     
  server.handleClient();          //Handle client requests
}
