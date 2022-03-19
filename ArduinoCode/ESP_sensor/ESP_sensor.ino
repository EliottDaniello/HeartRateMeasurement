/**
 * \file ESP_sensor.c
 * \brief Send data form the pulse sensor to the hospital ESP.
 * \author Eliott DANIELLO and Cedric LE BEAUDOUR
 * \date February 2022
 *
 * Circuit:
 * Pulse Sensor (https://pulsesensor.com/products/pulse-sensor-amped)
 * Custom PCB shield in folder "ArduinoShield"
 * 
*/

#include <ESP8266WiFi.h>

IPAddress server(192,168,243,160);  // IP of the server (
WiFiClient client;

char ssid[] = "ssid";
char pass[] = "pwd";

int i;
int PulseSensorPurplePin = 0;       // Pulse Sensor PURPLE WIRE connected to ANALOG PIN 0
int cnt = 0;                        // Memory if the previous value is above or below the threshold
int sig;                            // Holds the incoming raw data. Signal value can range from 0-1024 (10-bits ADC)
int threshold = 330;                // Determine which signal to "count as a beat", and which to ingore.
int nb_beat = 0;                    // Number of beats in 5 seconds
int nb_beat_per_minute = 0;         // Number of beats in 1 minute
int tab_60measurements [] = {0,0,0,0,0,0,0,0,0,0,0,0};  // Array for storing the number of beats in 5 seconds (12x5sec to get 1 min mesure)
unsigned long previous_time_cnt_5sec = 0;  // Previous time in ms
unsigned long current_time_cnt_5sec = 0;   // Current time in ms
float average_old_values = 0;       // Average of the last 10 values in the array
float average_two_values = 0;       // Average of the last 2 values in the array


void initWiFi() {
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED){ // While wifi not connected
    Serial.print(".");
    delay(500);
  }
  Serial.println(WiFi.localIP());
}


void setup() {
   Serial.begin(115200);
   initWiFi();
}


void loop() {
  sig = analogRead(PulseSensorPurplePin);  // Read the Pulse Sensor value
  
  if(cnt == 0 && sig < threshold){  // Signal goes below the threshold and was above it before
    cnt = 1;
  }

  if(cnt == 1 && sig > threshold){  // Signal goes above the threshold and was below it before
    cnt = 0; 
    nb_beat++;    
  }
  
  current_time_cnt_5sec = millis();
  if(current_time_cnt_5sec - previous_time_cnt_5sec > 5000){  // Every 5 seconds
    previous_time_cnt_5sec = current_time_cnt_5sec;
    
    for(i = 11; i>0; i--){    // Shift values in the array
      tab_60measurements[i] = tab_60measurements[i-1];
    }
    tab_60measurements[0] = nb_beat;  
         
    for(i = 2; i<12; i++){
      average_old_values += tab_60measurements[i];
    }
    average_old_values = average_old_values/10.0;
    average_two_values = (tab_60measurements[0] + tab_60measurements[1])/2.0;
    if((average_two_values<average_old_values*0.6 || average_two_values>average_old_values*1.4) // If the last 2 values are too different from the rest of the array
     &&(50/12<average_two_values && average_two_values<90/12)){   // But they are not extreme values
      for(i = 2; i<12; i+=2){   // The array is filled with the last two measures
        tab_60measurements[i] = tab_60measurements[0];
      }
      for(i = 3; i<12; i+=2){
        tab_60measurements[i] = tab_60measurements[1];
      }
    }

    for(i = 0; i<12; i++){
      Serial.print(tab_60measurements[i]);  // Print for debug
      Serial.print(" | ");
      nb_beat_per_minute += tab_60measurements[i];
    }
    Serial.println();
    
    Serial.print("nombre de battements par minute : "); // Print for debug
    Serial.println(nb_beat_per_minute);
    client.connect(server, 90);          // Connection to the server
    client.print(nb_beat_per_minute);    // Send the value of beats per min to the server
    client.println("\r");  
    client.flush();

    average_old_values = 0;
    nb_beat_per_minute = 0;
    nb_beat = 0;
  }
  delay(10);
}
