#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <IRremote.h>

#include "Index.h"

const char *ssid = "MERCUSYS_D8CC";
const char *password = "12345678";

//IPAddress local_ip(192,168,1,1);
//IPAddress gateway(192,168,1,1);
//IPAddress subnet(255,255,255,0);

WebServer server(80);

int IRon = 19;
int relay1 = 4;
int relay2 = 32;
int relay3 = 33;
bool relay1state = LOW;
bool relay2state = LOW;
bool relay3state = LOW;

IRrecv irrecv(IRon);
decode_results results;

void handleRoot(){
  server.send(200, "text/html", SendHTML(relay1state));
}

void handle_IRon(){
  if(irrecv.decode( & results)){
    String hex = String(results.value, HEX);
    Serial.print("Hex code: ");
    Serial.println(hex);

    if(hex == "ff18e7"){
      digitalWrite(relay1, HIGH); //turn on
      server.send(200, "text/html", SendHTML(true));
    }

    if(hex == "ff4ab5"){
      digitalWrite(relay1, LOW); //turn off
      server.send(200, "text/html", SendHTML(false));
    }
    irrecv.resume();
  }
}

void handle_IRoff(){

}

String SendHTML(uint8_t state){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name =\"viewport\" content=\"width=dedvice-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>Charger_Box</title>\n";
  ptr += "<style>html {font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444; margin: 50px auto: 30px;} h3 {color: #444444; margin-bottom: 50px;}\n";
  ptr += ".button {display: block; width: 80px; background-color: #3498db; border: none; color: white; padding: 13px 30px; text-decoration: none; font-size: 25px; margin: 0px auto 35px; cursor: pointer; border-radius: 4px;}\n";
  ptr += ".button-on {background-color: #3498db;}\n";
  ptr += ".button-on:active {background-color: #2980b9;}\n";
  ptr += ".button-off {background-color: #34495e;}\n";
  ptr += ".button-off:active {background-color: #2c3e50;}\n";
  ptr += "p {font-size: 14px; color: #888; margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "</body>\n";
  ptr += "<h1>Charger Box of VMC</h1>\n";
  ptr += "<img src="D:\7.GITHUB\Charger_Box\Charger.ino\Charger\Logo Viettel manufacturing_1 (2).jpg"  width="330" height="120">\n";
  
  if(relay1state){
    ptr += "<p>Relay state: ON</p><a class = \"button button-off\" href=\"/relay1on\">OFF</a>";
  }
  else{
    ptr += "<p>Relay state: OFF</p><a class = \"button button-on\" href=\"/relay1off\">ON</a>";
  }

  ptr += "</body>\n";
  ptr += "</html>\n";

  return ptr;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(500);
  irrecv.enableIRIn();
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
//  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  Serial.println("");

  Serial.print("Connecting");
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Successfully connected to: ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  server.on("/", handleRoot);
  server.on("/relay1on", handleIRon);
  server.on("/relay1off", handleIRon);
  server.on("/", hanldeIRoff);

  server.begin();
  Serial.println("HTTP servere started");

  Serial.println();
  Serial.println("Wait 10 seconds");

}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  handle_IRon();
  // if(relay1state){
  //   digitalWrite(relay1, HIGH);
  // }
  // else{
  //   digitalWrite(relay1, LOW);
  // }

}
