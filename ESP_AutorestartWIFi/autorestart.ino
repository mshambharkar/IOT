#include <ESP8266Ping.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "ESP8266HTTPClient.h"
#include "ESP8266WiFi.h"
#include "FS.h";

ESP8266WebServer server(80); // starting the webserver 80

String appassword = "SecuredPassword";
String loginbody="username=Admin&password=%urlencodedpassword%&submit.htm%3Flogin.htm=Send";
String ssid = "";
String password = "";
String ipaddress = "";
bool clientmode = false;
String deviceid = "ESP_Autorestarter"; // also using as the default ap name
unsigned long previousMillis = 0;
unsigned long interval = 15000 * 60; //15 minutes

void setup() {
  delay(25000); //Sometimesdelay is required to get serial working
  Serial.begin(115200); // serialdebug
  Serial.print("\n\n ************************************Program Started ************************************ \n\n");
  bool result = SPIFFS.begin();
  Serial.println("  SPIFFS opened: " + result);
  File f = SPIFFS.open("/config.ini", "r");
  if (!f) {
    Serial.println("*************** No Config File Found Running in AP Mode *************** ");
    clientmode = false;
    APMode();
  } else {
    Serial.println("*************** Config File Found Running in Client Mode *************** ");
    clientmode = true;
    connectWifi();
  }
}

void connectWifi() {
  Serial.println("########### connecting wifi ############");
  File config = SPIFFS.open("/config.ini", "r");
  Serial.println("########### Opened config.ini ############");
  while (config.available()) {
    String str = config.readStringUntil('\n');
    char str_array[str.length()];
    str.toCharArray(str_array, str.length());
    char *ptr = strtok(str_array, "=");
    if (strcmp(ptr, "ssid") == 0) {
      Serial.println("######################## SSID ########################");
      ssid = strtok(NULL, "=");
      Serial.print(ssid);
    }
    if (strcmp(ptr, "password") == 0) {
      Serial.println("######################## PASSWORD ########################");
      password = strtok(NULL, "=");
      Serial.print(password);
    }

    if (strcmp(ptr, "ipaddress") == 0) {
      Serial.println("######################## ipaddress ########################");
      ipaddress = strtok(NULL, "=");
      Serial.print(ipaddress);
    }
    if (strcmp(ptr, "interval") == 0) {
      Serial.println("######################## interval ########################");
      String configinterval = strtok(NULL, "=");
      interval = configinterval.toInt() * 1000 * 60;
      Serial.print(interval);
    }

  }
  Serial.println(" ######################### Connecting to Wifi using #############################" );
  Serial.println(ssid);
  Serial.println(password);
  char char_array[deviceid.length()];
  deviceid.toCharArray(char_array, deviceid.length() + 1);
  wifi_station_set_hostname(char_array);
  bool hostname  = WiFi.hostname(char_array);
  Serial.println(" -------------------------------- HOSTNAME ----------------------------------------");
  Serial.println(deviceid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }
  if (MDNS.begin(deviceid)) {
    Serial.println("MDNS started");
  }
  Serial.println("-----------------------------------------------");
  Serial.println("Connected");
  server.on("/reset", handleReset);
  server.begin();
  MDNS.addService("http", "tcp", 80);
  Serial.println("HTTP server started");
  Serial.println("----------------------------------------------");
}


void APMode() {
  WiFi.hostname(deviceid);
  WiFi.softAP(deviceid, appassword);
  if (MDNS.begin(deviceid)) {
    Serial.println("MDNS started");
  }

  Serial.println("################## IOTAPI ################");
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/reset", handleReset);
  server.begin();
  Serial.println("HTTP server started");
}
void loop() {
  MDNS.update();
  // put your main code here, to run repeatedly:
  server.handleClient();

  if (clientmode) {
    pingandrestart();
  }
  delay(2000);
}

void pingandrestart()
{

  if ((millis() - previousMillis) > interval)
  {
    Serial.println(millis());
    Serial.println(previousMillis);
    previousMillis = millis();
    //ping to server
    bool ret = Ping.ping("www.google.com", 10);
    if (!ret) {

      HTTPClient http;
      http.begin("http://192.168.0.1/login.cgi");
      http.addHeader("Cache-Control", "application/x-www-form-urlencoded");
      http.addHeader("Cache-Control", "max-age=0");
      http.addHeader("Origin", "http://192.168.0.1");
      http.addHeader("Upgrade-Insecure-Requests", "1");
      http.addHeader("DNT", "1");
      http.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.87 Safari/537.36");
      http.addHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3");
      http.addHeader("Referer", "http://192.168.0.1/login.htm");
      http.addHeader("Accept-Encoding", "gzip, deflate");
      http.addHeader("Accept-Language", "en-IN,en-GB;q=0.9,en-US;q=0.8,en;q=0.7,hi;q=0.6");
      http.addHeader("Cookie", "SessionID=");
      int httpResponseCode = http.POST(loginbody);
      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Login:");
        Serial.println(httpResponseCode);
        Serial.println(response);
      } else {
        Serial.print("Login Error: ");
        Serial.println(httpResponseCode);
      }
      http.end();

      HTTPClient httpreboot;
      httpreboot.begin("http://192.168.0.1/form2Reboot.cgi");
      httpreboot.addHeader("Cache-Control", "application/x-www-form-urlencoded");
      httpreboot.addHeader("Cache-Control", "max-age=0");
      httpreboot.addHeader("Origin", "http://192.168.0.1");
      httpreboot.addHeader("Upgrade-Insecure-Requests", "1");
      httpreboot.addHeader("DNT", "1");
      httpreboot.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.70 Safari/537.36");
      httpreboot.addHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3");
      httpreboot.addHeader("Referer", "http://192.168.0.1/reboot.htm");
      httpreboot.addHeader("Accept-Encoding", "gzip, deflate");
      httpreboot.addHeader("Accept-Language", "en-IN,en-GB;q=0.9,en-US;q=0.8,en;q=0.7,hi;q=0.6");
      httpreboot.addHeader("Cookie", "SessionID=");
      int rebootstauscode = httpreboot.POST("reboot=Reboot&submit.htm%3Freboot.htm=Send");
      if (rebootstauscode > 0) {
        Serial.println("Reboot:");
        Serial.println(rebootstauscode);
      } else {
        Serial.print("Reboot error: ");
        Serial.println(rebootstauscode);
      }
      httpreboot.end();

      delay(2500);
      ESP.restart();
    }
    Serial.printf("Ping done :%d", ret);
  }


}

void handleRoot() {
  File config = SPIFFS.open("/index.html", "r");
  String finalString = "";
  while (config.available())
  {
    finalString += (char)config.read();
  }
  server.send(200, "text/html", finalString);
}


void handleReset() {
  SPIFFS.remove("/config.ini");
  server.send(200, "text/html", "<h1>Config Reset Done Restart </h1>");
  delay(2000);
  ESP.restart();
}

void handleConfig() {
  Serial.println("Received Config Request");
  Serial.print(server.args());
  File f = SPIFFS.open("/config.ini", "r");
  if (!f) {
    File config = SPIFFS.open("/config.ini", "w");
    Serial.print("Config File Opened -> ");
    Serial.print(f);
    if (config) {
      for (int i = 0; i < server.args(); i++) {
        if (server.argName(i) == "ssid") {
          Serial.print("SSID ");
          Serial.print(server.arg(i));
          config.print("ssid=");
          config.println(server.arg(i));
        }
        if (server.argName(i) == "password") {
          Serial.print("Password ");
          Serial.print(server.arg(i));
          config.print("password=");
          config.println(server.arg(i));
        }
        if (server.argName(i) == "ipaddress") {
          Serial.print("ipaddress ");
          Serial.print(server.arg(i));
          config.print("ipaddress=");
          config.println(server.arg(i));
        }
        if (server.argName(i) == "interval") {
          Serial.print("interval ");
          Serial.print(server.arg(i));
          config.print("interval=");
          config.println(server.arg(i));
        }
      }
      config.close();
      server.send(200, "text/html" , "<h2 style='color:green;'>Config Successfully Done Restarting in Client mode</h2>");
      ESP.restart();
    } else {
      server.send(200, "text/html" , "<h2 style='color:red'>Configuration Error Occured Please Try Again</h2>");
    }
  }
}
