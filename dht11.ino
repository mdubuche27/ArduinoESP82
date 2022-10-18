#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <ESPDateTime.h>


//Wi-FI
const char* ssid = "ACTEMIUM_ML";
const char* password = "ACTEMIUM_ML";

//MQTT 
const char* mqttServer = "192.168.202.15";
const int mqttPort = 1883;
const char* mqttUser = "mqtt-sender";
const char* mqttPassword = "4Ve5*36Y%2%XPE";
const char* clientID = "mqtt-sender";

WiFiClient espClient;
PubSubClient client(espClient);

//DHT
#define DHTPIN 5
#define DHTTYPE    DHT11
DHT dht(DHTPIN, DHTTYPE);

//temp et humidity
float t = 0.0;
float h = 0.0;

AsyncWebServer server(80);

unsigned long previousMillis = 0;

const long interval = 10000;  

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>ESP8266 DHT Server</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">%</sup>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;
</script>
</html>)rawliteral";

String processor(const String& var){
  if(var == "TEMPERATURE"){
    return String(t);
  }
  else if(var == "HUMIDITY"){
    return String(h);
  }
  return String();
}

void Publish(String DeviceID, float temp, float hum){
  const String jour = DateTime.format(DateFormatter::DATE_ONLY);
  const String heure = DateTime.format(DateFormatter::TIME_ONLY);

  const char* DataJson = "{\"DeviceID\" = \"" + DeviceID + "\",\"temp\" = " + temp + ",\"hum\" = " + hum + ",\"jour\" = " + jour + ",\"heure\" = " + heure + ",}";

  Serial.println(DataJson);
  client.publish("plateforme/GB20008", DataJson);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
 
  Serial.println();
  Serial.println("-----------------------");
 
}

void setup(){
  //init
  Serial.begin(115200);
  dht.begin();

  //setup datetime
  DateTime.setTimeZone("CST-8");
  DateTime.begin();

  //Wi-Fi setup 
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }
  Serial.println("Wi-fi ok");

  //MQTT connexion broker
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  while (!client.connected()) {
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
      Serial.println("Connected to MQTT Broker!");
    }
    else {
      Serial.println("Connection to MQTT Broker failed...");
    }
    delay(2000);
  }

  // String DataToSend = String(DataJson(0,0));
  // const int size = DataToSend.length();
  // char Data[size];
  // DataToSend.toCharArray(Data, size);
  // Serial.println(Data);
  // client.publish("plateforme/GB20008", Data);

  //LAUNCH Server and define endpoint 
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(t).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(h).c_str());
  });

  server.begin();
}
 
void loop(){  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float newT = dht.readTemperature();
    float newH = dht.readHumidity();

    if (isnan(newT) && isnan(newH)) {
      Serial.println("Failed to read from DHT sensor!");
    }

    else {
      // Serial.println("--------------------");
      // t = newT;
      // h = newH;
      // String StringMQTTRequest= MQTTrequest(t, h);
      // Serial.println("-> "+ StringMQTTRequest);
      // char CharMQTTResult[StringMQTTRequest.length()+1];
      // //Serial.println("-> ", CharMQTTResult)
      // StringMQTTRequest.toCharArray(CharMQTTResult, StringMQTTRequest.length());
      // client.publish("plateforme/GB20008", CharMQTTResult);
      // Serial.println("--------------------");

      Publish("1", 0, 0);
    }
  }
}