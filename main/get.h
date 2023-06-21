#include <WiFi.h>
#include <HTTPClient.h>
#include <../components/Arduino_JSON/src/Arduino_JSON.h>
#include <iostream>

//Your Domain name with URL path or IP address with path
const char* getServerName = "http://10.82.118.236:8000/bdd/valveState/"; /* serveur PC */
// const char* getServerName = "http://10.100.1.15:8001/bdd/valveState/"; /* serveur PC */


// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long getLastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long getTimerDelay = 5000;

String sensorReadings;
float sensorReadingsArr[3];
int k = 1;
int m = 1;

String httpGETRequest(const char* getServerName) {
  WiFiClient client;
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(client, getServerName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP GET Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}


int GetLoop() {

  //Send an HTTP POST request every 10 minutes
  if ((millis() - getLastTime) > getTimerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
              
      sensorReadings = httpGETRequest(getServerName);
      // Serial.println(sensorReadings);
      JSONVar myObject = JSON.parse(sensorReadings);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
      }
    
      // Serial.print("JSON object = ");
      // Serial.println(myObject);
    
      // myObject.keys() can be used to get an array of all the keys in the object
      JSONVar keys = myObject.keys();
    
      for (int i = 0; i < keys.length(); i++) {
        JSONVar value = myObject[keys[i]];
        // Serial.print(keys[i]);
        // Serial.print(" = ");
        // Serial.println(value);
        sensorReadingsArr[i] = double(value);

        String valveOrder = JSON.stringify(myObject);          /* converting JSON to string **********************/

        if((valveOrder[16] == 'A') || (valveOrder[16] == 'a')){
          if((valveOrder[36] == 't') && (k == 1)){
            k=2;
            return 1;   /* valve = true*/
          }
          if((valveOrder[36] == 'f') && (k == 2)){
            k=1;
            return 2;   /* valve = false */
          }
          if(((valveOrder[41] == 't') || (valveOrder[42] == 't')) && (m == 1)){
            m=2;
            return 3;   /* pump = true */
          }
          if(((valveOrder[41] == 'f') || (valveOrder[42] == 'f')) && (m == 2)){
            m=1;
            return 4;   /* pump = false */
          }
        } 
      }
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    getLastTime = millis();
  }
     return 0;
}