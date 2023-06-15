/*
  Rui Santos
  Complete project details at Complete project details at https://RandomNerdTutorials.com/esp32-http-get-post-arduino/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <../components/Arduino_JSON/src/Arduino_JSON.h>
#include <iostream>

//Your Domain name with URL path or IP address with path
const char* getServerName = "http://10.100.0.62:8000/bdd/valveState/"; /* serveur PC */

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long getLastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long getTimerDelay = 5000;

String sensorReadings;
float sensorReadingsArr[3];

String httpGETRequest(const char* getServerName) {
  WiFiClient client;
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(client, getServerName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.println("Processing GET request");
    Serial.print("HTTP Response code: ");
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

// void GetLoop() {
//   //Send an HTTP POST request every 10 minutes
//   if ((millis() - getLastTime) > getTimerDelay) {
//     //Check WiFi connection status
//     if(WiFi.status()== WL_CONNECTED){
              
//       sensorReadings = httpGETRequest(getServerName);
//       Serial.println(sensorReadings);
//       JSONVar myObject = JSON.parse(sensorReadings);
  
//       // JSON.typeof(jsonVar) can be used to get the type of the var
//       if (JSON.typeof(myObject) == "undefined") {
//         Serial.println("Parsing input failed!");
//         return;
//       }
    
//       Serial.print("JSON object = ");
//       Serial.println(myObject);
    
//       // myObject.keys() can be used to get an array of all the keys in the object
//       JSONVar keys = myObject.keys();
    
//       for (int i = 0; i < keys.length(); i++) {
//         JSONVar value = myObject[keys[i]];
//         Serial.print(keys[i]);
//         Serial.print(" = ");
//         Serial.println(value);
//         sensorReadingsArr[i] = double(value);
//       }

//     }
//     else {
//       Serial.println("WiFi Disconnected");
//     }
//     getLastTime = millis();
//   }
// }




int GetLoop() {

  int valveOrderLength;

  //Send an HTTP POST request every 10 minutes
  if ((millis() - getLastTime) > getTimerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
              
      sensorReadings = httpGETRequest(getServerName);
      Serial.println(sensorReadings);
      JSONVar myObject = JSON.parse(sensorReadings);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        // return EXIT_SUCCESS;
      }
    
      Serial.print("JSON object = ");
      Serial.println(myObject);
    
      // myObject.keys() can be used to get an array of all the keys in the object
      JSONVar keys = myObject.keys();
    
      for (int i = 0; i < keys.length(); i++) {
        JSONVar value = myObject[keys[i]];
        Serial.print(keys[i]);
        Serial.print(" = ");
        Serial.println(value);
        sensorReadingsArr[i] = double(value);


        String valveOrder = JSON.stringify(myObject);                 /* converting JSON to string **********************/
        // printf("valveOrder = %s\n", valveOrder.c_str());       /* getting the number of characters in the string */
        valveOrderLength = strlen(valveOrder.c_str());         /* saving the number of characters ****************/
        // printf("%d\n", valveOrderLength);

        if(valveOrderLength == 35){
          return 1;
        }
        else if(valveOrderLength == 36){
          return 2;
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