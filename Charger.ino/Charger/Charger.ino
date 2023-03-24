#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <IRremote.h>

#include "Index.h"

const char *ssid = "Wifi";
const char *password = "Pass";

IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

WebServer server(80);

int IRon = 19;
int relay1 = 4;
int relay2 = 32;
int relay3 = 33;

IRrecv irrecv(IRon);
decode_results results;

void handleRoot(){
  String s = MAIN_page;
  server.send(200, "text/html", s);
}

void handle_IRon(){
  if(irrecv.decode( & results)){
    String hex = String(results.value, HEX);
    Serial.print("Hex code: ");
    Serial.println(hex);

    if(hex == "ff18e7"){
      digitalWrite(relay1, HIGH); //turn on
    }

    if(hex == "ff4ab5"){
      digitalWrite(relay1, LOW); //turn off
    }
    irrecv.resume();
  }
}

void handle_IRoff(){

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
  WiFi.softAPConfig(local_ip, gateway, subnet);
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
  server.on("/", handleIRon);
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

}
