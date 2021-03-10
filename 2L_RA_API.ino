#include <ESP8266WiFi.h>
#include "ESP8266WebServer.h"
#include <EEPROM.h>


#define HTTP_REST_PORT 80
#define CONNECTION_CHECK_MAX_TRIES 5
#define UNSET_IP "(IP unset)"
#define COMMAND_FEEDBACK_TIME_OUT 5000UL
#define PRINT_REPLY_MSG "ok"

const char* hostName     = "2l.robot.api";
IPAddress Ip(10, 1, 1, 10);
IPAddress NMask(255, 255, 255, 0);

String wifi_ssid = "[YOUR_WIFI_SSID]";
String wifi_password = "[YOUR_WIFI_PASSWORD]";

boolean isAPServing = false;
boolean busy = false;
unsigned long currentMillis;
ESP8266WebServer server(HTTP_REST_PORT);

void setup() {
  Serial.begin(115200);
  startWebServer();
  WiFi.mode(WIFI_STA);
}

void loop() {
  currentMillis = millis();
  if(WiFi.status() != WL_CONNECTED) {
    boolean connectionSuccess = try_reconnect();
    if(!connectionSuccess) {
      setup_ap();
      delay(500);
    } else {
      Serial.print("WiFi connected to ");
      Serial.print(WiFi.SSID());
      Serial.print(" at IP address: ");
      String ip = WiFi.localIP().toString();
      Serial.println(ip);   
      Serial.flush();   
    }
  } else if(WiFi.SSID() == wifi_ssid && WiFi.getMode() == WIFI_AP_STA) {
    Serial.println("Shutting down AP");
    Serial.flush();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
  } else {
    server.handleClient();
  }
}

void setup_ap() {
  WiFi.softAPConfig(Ip, Ip, NMask); 
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(hostName);  
  Serial.print("Soft-AP HOST NAME = ");
  Serial.println(hostName);
  Serial.print("Soft-AP IP address = ");
  Serial.println(Ip);
  boolean result = WiFi.softAPIP();
  if(result){
    Serial.println("AP started");
  } else {
    Serial.println("Unable to start Access Point");
  }
  Serial.flush();
}

boolean try_reconnect() {
  if(wifi_ssid == "") {
    return false;
  }
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < CONNECTION_CHECK_MAX_TRIES) {
    delay(2000);
    Serial.print(".");
    tries++;
  }
  Serial.println("");
  Serial.flush();
  return WiFi.status() == WL_CONNECTED;
}

String sendCommand(String command, bool isJson) {
  if(busy) {
    return "busy";  
  }
  if(isJson) {
    command = urldecode(command);
  }
  busy = true;
  command = ">>>" + command; 
  if(command.indexOf('\n') < 0) {
    command = command + '\r' +'\n';
  }
  Serial.print(command);
  Serial.flush();
  busy = true;
  String message = "";
  unsigned long startMillis = millis();
  unsigned long delta;
  bool timeout = false;
  while (!Serial.available()) {
    delta = millis() - startMillis;
    if(delta >= COMMAND_FEEDBACK_TIME_OUT) {
      timeout = true;
      break;  
    }
    ESP.wdtFeed();
  }
  if(timeout) {
    Serial.println("ERROR: wait for command feedback timeout");
    message="Timeout";
  } else {
    while(Serial.available()) {
      /*char c = Serial.read();
      message += c;
      if(message == PRINT_REPLY_MSG) {
        Serial.print("inside");
        break;
      }*/
      message = Serial.readString();
      ESP.wdtFeed();
    }
  }
  busy = false;
  return message;
}

String urldecode(String str)
{
    
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
      
      yield();
    }
    
   return encodedString;
}

String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
    
}

unsigned char h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}
