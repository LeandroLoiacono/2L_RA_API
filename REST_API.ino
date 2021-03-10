#define BUSY_TIMEOUT 2000
String rootWebPage;

void startWebServer() {
  server.on("/", HTTP_GET, handle_Root);
  server.on("/gcode", HTTP_POST, handle_html_GCode);
  server.on("/json", handle_json_GCode);
  server.on("/save", HTTP_POST, handle_SetWIFI);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("SERVER BEGIN!!");
}

void handle_Root() {
  if(isAPServing) {
    server.send(200, "text/html", generateAPHTML("")); 
  } else { 
    server.send(200, "text/html", generateHTML("")); 
  }
}

void handle_SetWIFI(){ 
  String message = "";
  int argsNr = server.args();
  if(argsNr != 2 || 
    !((server.argName(0) == "ssid" && server.argName(1) == "password") || 
      (server.argName(0) == "password" && server.argName(1) == "ssid"))
    ) {
    message = "<div class='error'>Errore: parametri errati - attesi: 'ssid', 'password'</div>";
  } else {
      if(server.argName(0) == "ssid") {
        wifi_ssid = server.arg(0).c_str();
        wifi_password = server.arg(1).c_str();
      } else {
        wifi_ssid = server.arg(1).c_str();
        wifi_password = server.arg(0).c_str();
      }
      //boolean connected = try_reconnect();
      //if( connected ) {
        message = "<div class='success'>WiFi configured, test your connection</div>"; 
      //} else {
      //  message = "<div class='error'>Errore: WiFi not connected, check 'ssid' and 'password'</div>";
      //}
  }
  //request->send(200, "text/html", generateAPHTML(message)); 
  server.send(200, "text/html", generateAPHTML(message)); 
}

void handle_NotFound() {
   server.send(404, "text/plain", "Not found");
}

void handle_GCode(bool isJson) {
  String message = "";
  String status = "";
  //int argsNr = request->params();
  int argsNr = server.args();
  if(argsNr != 1 || (server.argName(0) != "command")) {
    message = "parametri errati - atteso: 'command'";
    status = "ERROR";
  } else {
    String command = server.arg(0);
    unsigned long startMillis = millis();
    unsigned long delta = 0;
    while(busy) {
      delta = currentMillis - startMillis;
      if(delta >= BUSY_TIMEOUT) {
        break;  
      }
      ESP.wdtFeed();
    }
    if(delta >= BUSY_TIMEOUT) {
      message = "Too busy!";
    } else {
      message = sendCommand(command, isJson);
    }
    status = "SUCCESS";
  }
  String result = "";
  server.sendHeader(String(F("Access-Control-Allow-Origin")), String("*"));
  if(isJson) {
    result = "{\"status\": \"" + status + "\",\"message\": \"" + message + "\"}";
    //request->send(200, "application/json", result);
    server.send(200, "application/json", result);
  } else {
    result = "<div class='success'>" + message + "</div>";
    //request->send(200, "text/html", generateHTML(message));
    server.send(200, "text/html", generateHTML(message));
  } 
}

void handle_html_GCode() {
  handle_GCode(false);
}

void handle_json_GCode() {
  handle_GCode(true);
}

String generateHTML(String feedback){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>Roboto Arm Control</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px; padding: 10px; } h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +="button, .button {width: 100%;background-color: #3498db;border: none;color: white;padding: 13px 10px;text-align: center; text-decoration: none;font-size: 25px;margin: 10px 0;cursor: pointer;border-radius: 4px;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;} table { width: 50%; margin: auto}\n";
  ptr +=".textinput {padding: 12px 10px 11px; width: 100px; display: inline; font-size: 24px; border-radius: 4px; border: solid 1px; margin: 10px;}\n";
  ptr +=".label { font-size: 24px; }\n";
  ptr +=".error { color: red; }\n";
  ptr +=".success { color: green; }\n";
  ptr +="</style>\n";
  ptr +="<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js'></script>\n";
  ptr +="<script src='https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/js/bootstrap.bundle.min.js' integrity='sha384-ho+j7jyWK8fNQe+A12Hb8AhRq26LrZ/JpcUGGOn+Y7RsweNrtN/tE3MoK7ZeZDyx' crossorigin='anonymous'></script>\n";
  ptr +="<link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css' integrity='sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2' crossorigin='anonymous'>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>Robot Arm</h1>\n";
  ptr +="<h3 id='feedback'>" + feedback + "</h3>\n";
  ptr +="<p>";
  ptr +="<form action='/gcode' method='POST' enctype='multipart/form-data'>";
  ptr +="<input type='text' name='command' placeholder='G-Code Command'/>";
  ptr +="<input class='button' type='submit' value='Send'/>";
  ptr +="</p>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}

String generateAPHTML(String feedback){
  String options = "";
  if(feedback.indexOf("success" <= 0)) {
    int n = WiFi.scanComplete();
    if(n == -2){
      WiFi.scanNetworks(true);
    } else if(n){
      for (int i = 0; i < n; ++i){
        options += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>\n";
      }
    }
    WiFi.scanDelete();
  } else {
    String ssid = WiFi.SSID();
    options += "<option value='" + ssid + "'>" + ssid + "</option>\n";  
  }
    
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>Roboto Arm Control</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px; padding: 10px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +="button, .button {width: 100%;background-color: #3498db;border: none;color: white;padding: 13px 10px;text-align: center; text-decoration: none;font-size: 25px;margin: 10px 0;cursor: pointer;border-radius: 4px;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;} table { width: 50%; margin: auto}\n";
  ptr +=".textinput {padding: 12px 10px 11px; width: 100px; display: inline; font-size: 24px; border-radius: 4px; border: solid 1px; margin: 10px;}\n";
  ptr +=".label { font-size: 24px; }\n";
  ptr +=".error { color: red; }\n";
  ptr +=".success { color: green; }\n";
  ptr +="</style>\n";
  ptr +="<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js'></script>\n";
  ptr +="<script src='https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/js/bootstrap.bundle.min.js' integrity='sha384-ho+j7jyWK8fNQe+A12Hb8AhRq26LrZ/JpcUGGOn+Y7RsweNrtN/tE3MoK7ZeZDyx' crossorigin='anonymous'></script>\n";
  ptr +="<link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css' integrity='sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2' crossorigin='anonymous'>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>Robot Arm</h1>\n";
  ptr +="<h3 id='feedback'>" + feedback + "</h3>\n";
  ptr +="<p>";
  ptr +="<form action='/save' method='POST' enctype='multipart/form-data'>";
  ptr +="<select name='ssid' placeholder='ssid'>" + options +"</select>";
  ptr +="<input type='password' name='password' placeholder='password'/>";
  ptr +="<input class='button' type='submit' value='Save'/>";
  ptr +="</form>";
  ptr +="</p>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}
