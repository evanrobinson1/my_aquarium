

// Import required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>   // File Wrapper. May not be in use
#include <Wire.h>  // I2C support not in use here

#include <DallasTemperature.h>
#include <OneWire.h>
#include <Adafruit_Sensor.h>   // not in use here
#include <DHT.h>               // not in use here

// ph sensor variables:
const int analogInPin = A0; 
int sensorValue = 0; 
unsigned long int avgValue; 
float b;
int buf[10],temp;

// -------- One Wire stuff -----------

#define ONE_WIRE_BUS 4                          //Raw pin 4 is D2 pin of nodemcu
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);            // Pass the oneWire reference to Dallas Temperature.


// ------ Network Stuff --------

// Replace with your network credentials

const char* ssid     = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

int port =  254; // default or replace with your desired port# here

// Create AsyncWebServer object on port xx
AsyncWebServer server(port); 

const int relayPin = 12;      // D6
const int relay2Pin = 13;     //  D7
const int fishFeederPin = 14;     // D5
const int pirPin = 5;      // D2
int pirStat = 0;                   // PIR status
int c = 0;


// Store states
String relayState;
String relay2State;
String fishFeederState;

// current temperature & ph, updated in loop()
float tf = 0.0;
float phValue = 0.0;
float phVol = 0.0;

String relay = "off";
String relay2 = "off";
String fishFeeder = "off";

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time sensors were updated

// Updates DHT readings every 10 seconds
const long interval = 10000;  

const char index_html[] PROGMEM = R"rawliteral(         
<!DOCTYPE HTML><html>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">

<HTML>
<HEAD>
<TITLE>My Fishtank</TITLE>

<META content="IE=5.0000" http-equiv="X-UA-Compatible">
<META content="text/html; charset=iso-8859-1" http-equiv=Content-Type>
<script src="https://code.highcharts.com/highcharts.js"></script>

 <style>
  button{ 
  background-color: #4CAF50; /* Green */
  border: none;
  text-align: center;
  display: inline-block;
  font-size: 18px;
        width: 90px;
        height: 60px;
    }
    power-labels{
    font-size: 8px;    
    }
</style>

</HEAD>

<BODY bgColor=#ffffff text=#000000>

<P><FONT size=6><FONT color=blue>My Fishtank</FONT></P>

<TABLE border=1 width=700>  
<TBODY>
<TR>
    
<TD height=300 width=700>

<a href="/on"><button class="button">OH LAMP ON</button></a>
<a href="/off"><button class="button button2">OH LAMP OFF</button></a>

<BR>
<a href="/onb"><button class="button button3">UW LIGHT ON</button></a>
<a href="/offb"><button class="button button4">UW LIGHT OFF</button></a>
 
 <br>
   <FONT size=4> OVERHEAD LAMP IS:<BR></FONT>
   <FONT size=5> <span id="ledState">%STATE%</span></FONT>
  
  <BR>
 
    <FONT size=4>UW LIGHT IS: </font><BR>
    <FONT size=5><span id="ledState2">%STATEB%</FONT>
  <BR><BR>

 <FONT size=5>Feed the Fish:</font> <BR> <a href="/onc"><button class="button button5">Fish Feed ON</button></a>
  
  <H6> Monitor: </H6>

 <FONT size=4>Temperature: </font> 
    <span id="tf">%TF%</span>
    <sup class="units">&deg;</sup>
  <br>

    <FONT size=4>PH: </font> 
    <span id="ph">%PH%</span>
    <sup class="units">&deg;</sup>
  <br>
    <FONT size=4>Volts read at sensor:
    <span id="phv">%PHV%</span>
    <sup class="units">volts</sup></font> 
  

</TD></TR></TABLE>

<table  class = "center" border = "1"><tr><td width="700" height="500" id="chart-temperature" class="container"></td>
               <td width="700" id="chart-humidity" class="container"></td></tr></table>

<SCRIPT>

// server functions:

var chartH = new Highcharts.Chart({
  chart:{ renderTo:'chart-humidity' },
  title: { text: 'PH' },
  series: [{
    showInLegend: false,
    data: []
  }],
  plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: true }
    }
  },
  xAxis: {
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: { text: 'PH (%)' }
  },
  credits: { enabled: false }
});
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
     document.getElementById("ph").innerHTML = this.responseText;
    if (this.readyState == 4 && this.status == 200) {
      var x = (new Date()).getTime(),
          y = parseFloat(this.responseText);
      //console.log(this.responseText);
      if(chartH.series[0].data.length > 40) {
        chartH.series[0].addPoint([x, y], true, true, true);
      } else {
        chartH.series[0].addPoint([x, y], true, false, true);
      }
    }
  };
  xhttp.open("GET", "/ph", true);
  xhttp.send();
}, 10000 ) ;



var chartT = new Highcharts.Chart({
  chart:{ renderTo : 'chart-temperature' },
  title: { text: 'Temperature' },
  series: [{
    showInLegend: false,
    data: []
  }],
  plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: true }
    },
    series: { color: '#059e8a' }
  },
  xAxis: { type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
   // title: { text: 'Temperature (Celsius)' }
    title: { text: 'Temperature (Fahrenheit)' }
  },
  credits: { enabled: false }
});
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
       document.getElementById("tf").innerHTML = this.responseText;
      var x = (new Date()).getTime(),
          y = parseFloat(this.responseText);
      //console.log(this.responseText);
      if(chartT.series[0].data.length > 40) {
        chartT.series[0].addPoint([x, y], true, true, true);
      } else {
        chartT.series[0].addPoint([x, y], true, false, true);
      }
    }
  };
  xhttp.open("GET", "/tf", true);
  xhttp.send();
}, 10000 ) ;


setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
       document.getElementById("phv").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/phv", true);
  xhttp.send();
}, 17000 ) ;


</SCRIPT>)rawliteral";    // end webpage


// Replaces Webpage placeholders with values

  String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if(digitalRead(relayPin)){
      relayState = "OFF";
    }
    else{
      relayState = "ON";
    }
    Serial.print(relayState);
    return relayState;
  }
    else if(var == "STATEB"){
    if(digitalRead(relay2Pin)){
      relay2State = "OFF";
    }
    else{
      relay2State = "ON";
    }
    Serial.print(relay2State);
    return relay2State;
  }
  else if (var == "TF"){
    return String(tf);
  }
  else if(var == "PH"){
    return String(phValue);
  }
   else if(var == "PHV"){
    return String(phVol);
  }
  return String();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
//  dht.begin();

  pinMode(relayPin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);
   pinMode(fishFeederPin, OUTPUT);
 
  digitalWrite(relayPin, HIGH);
  digitalWrite(relay2Pin, HIGH);
  digitalWrite(fishFeederPin, HIGH);
  
 pinMode(pirPin, INPUT);
 digitalWrite(pirPin, LOW);

    // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  
  Serial.println(" ");
  Serial.print("Host name: ");
  Serial.print(WiFi.hostname());
  Serial.print(" ; MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Connecting to WiFi "); 
  Serial.println(ssid);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }
  
  // Print ESP8266 Local IP Address
  
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Port: ");
  Serial.print(port);// Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
 
 server.on("/ph", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(phValue).c_str());
  });
  server.on("/phv", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(phVol).c_str());
  });

   server.on("/tf", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(tf).c_str());
  });

 // Route to set D6 GPI12 to LOW
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request)     {
   digitalWrite(relayPin, LOW);
 request->send_P(200, "text/html", index_html, processor);
                                                                    }
);
  
 // Route to set D6 GPIO12 to HIGH
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request)     {
   digitalWrite(relayPin, HIGH);
     request->send_P(200, "text/html", index_html, processor);
                                                                    }
);
  
 // Route to set D7 GPI13 to LOW
  server.on("/onb", HTTP_GET, [](AsyncWebServerRequest *request)     {
   digitalWrite(relay2Pin, LOW);
 request->send_P(200, "text/html", index_html, processor);
                                                                    }
);
  
 // Route to set D7 GPI3 to HIGH
  server.on("/offb", HTTP_GET, [](AsyncWebServerRequest *request)     {
   digitalWrite(relay2Pin, HIGH);
     request->send_P(200, "text/html", index_html, processor);
                                                                    }
);

// Route to set D5 FF to LOW
  server.on("/onc", HTTP_GET, [](AsyncWebServerRequest *request)     {
   digitalWrite(fishFeederPin, LOW);
 //  delay(3000);
 //  digitalWrite(fishFeederPin, HIGH);
 request->send_P(200, "text/html", index_html, processor);
                                                                    }
);

  // Start server
  server.begin();
  
}


 
void loop(){ 

 pirStat = digitalRead(pirPin); 
 if (pirStat == HIGH) {            // if motion detected
   digitalWrite(relayPin, LOW);  // turn Relay1 ON
   Serial.println("Hey I got you!!!");
 }
   
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the sensor values
    previousMillis = currentMillis;

   digitalWrite(fishFeederPin, HIGH);   // turn off fishfeeder

// Dallas:
  sensors.requestTemperatures();                // Send the command to get temperatures
  tf = sensors.getTempFByIndex(0);
  Serial.println("Temperature is: ");
  Serial.println(tf);
  delay(500);
 
 
// PH sensor: 
 int sensorValue = analogRead(A0);    // read A0 input
// Serial.print("sensorValue analogRead is: "); 
 // Serial.println(sensorValue);
   float voltage = sensorValue * (3.3 / 1023.0);
 // print out the value you read:
 Serial.print("converted voltage read is: "); 
 Serial.println(voltage);
  delay(500);

// float phread = calculatePH(); 
 for(int i=0;i<10;i++) 
 { 
  buf[i]=analogRead(analogInPin);
  delay(10);
 }
 for(int i=0;i<9;i++)
 {
  for(int j=i+1;j<10;j++)
  {
   if(buf[i]>buf[j])
   {
    temp=buf[i];
    buf[i]=buf[j];
    buf[j]=temp;
   }
  }
 }
 avgValue=0;
 for(int i=2;i<8;i++)
 avgValue+=buf[i];
 phVol=(float)avgValue*5/1024/6;

 // float phValue = -5.65 * pHVol + 21.34;
  phValue = -7.48 * phVol + 25.7409;


 Serial.print("phVol = ");
 Serial.println(phVol);
 
 Serial.print("PH= ");
 Serial.println(phValue);
 
// delay(1000);
// End PH Sensor

 pirStat = digitalRead(pirPin); 
 if (pirStat == HIGH) {            // if motion detected
   digitalWrite(relayPin, LOW);  // turn Relay1 ON
   Serial.println("Hey I got you!!!");
 }
 else {
  c = c+1;
  if (c == 3){
   digitalWrite(relayPin, HIGH); // turn Relay1 OFF if we have no motion
   c=0;
  }
  
 }

}
}
    
