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

//Your Domain name with URL path or IP address with path
const char* postServerName = "http://10.82.117.207:8000/bdd/receiveData/";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long postLastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long postTimerDelay = 5000;

void PostLoop() {
  //Send an HTTP POST request every 10 minutes
  if ((millis() - postLastTime) > postTimerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      WiFiClient client;
      HTTPClient http;
    
      /* Your Domain name with URL path or IP address with path */
      http.begin(client, postServerName);
      
      
      /* Header for JSON request */
      http.addHeader("Content-Type", "application/json");      
      // int httpResponseCode = http.POST("{\"api_key\":\"tPmAT5Ab3j7F9\",\"sensor\":\"BME280\",\"value1\":\"24.25\",\"value2\":\"49.54\",\"value3\":\"1005.14\"}");
      
      /* Data to be sent */
      String jsonData = "{\"node_id\":\"0x0A\",\"longitude\":\"1456\",\"latitude\":\"7456\",\"internal_humidity\":\"13\",\"external_humidity\":\"25\",\"internal_temperature\":\"25\",\"external_temperature\":\"33\"}";
      int httpResponseCode = http.POST(jsonData); /* POST request */
     
      Serial.println("Processing POST request");
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
        
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    postLastTime = millis();
  }
}
