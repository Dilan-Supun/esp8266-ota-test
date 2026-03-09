#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>

//const char* ssid = "GPV_Test";
//const char* password = "K@$per$kY";

const char* ssid = "POCO X3 NFC";
const char* password = "i2345678";

const char* githubUser = "Dilan-Supun";
const char* githubRepo = "esp8266-ota-test";
const char* assetName  = "esp8266-ota-test.ino.nodemcu.bin";
const String currentVersion = "1.0";

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected: " + WiFi.localIP().toString());
  Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
}

void loop() {
  digitalWrite(LED_BUILTIN, (millis() / 500) % 2);

  static unsigned long lastCheck = 0;
  if (lastCheck == 0 || millis() - lastCheck > 30000) {
    lastCheck = millis();
    checkForUpdate();
  }

  delay(10);
}

void checkForUpdate() {
  Serial.println("\n=== OTA CHECK ===");

  String latestVersion = fetchVersionFromGitHub();
  if (latestVersion.length() == 0) {
    Serial.println("Version fetch failed");
    Serial.println("=================");
    return;
  }

  latestVersion.trim();
  Serial.println("Current version : " + currentVersion);
  Serial.println("GitHub version  : " + latestVersion);

  if (latestVersion != currentVersion) {
    String fwUrl = "https://github.com/";
    fwUrl += githubUser;
    fwUrl += "/";
    fwUrl += githubRepo;
    fwUrl += "/releases/latest/download/";
    fwUrl += assetName;

    Serial.println("Updating from:");
    Serial.println(fwUrl);
    doFirmwareUpdate(fwUrl);
  } else {
    Serial.println("Up to date");
  }

  Serial.println("=================");
}

String fetchVersionFromGitHub() {
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15000);

  const char* host = "raw.githubusercontent.com";
  const int httpsPort = 443;
  String path = "/";
  path += githubUser;
  path += "/";
  path += githubRepo;
  path += "/main/version.txt";

  Serial.print("Connecting to ");
  Serial.println(host);

  if (!client.connect(host, httpsPort)) {
    Serial.println("TLS connect failed");
    return "";
  }

  client.print(String("GET ") + path + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266-OTA\r\n" +
               "Connection: close\r\n\r\n");

  unsigned long start = millis();
  while (client.connected() && !client.available()) {
    if (millis() - start > 15000) {
      Serial.println("Timeout waiting response");
      client.stop();
      return "";
    }
    delay(10);
  }

  String headers = "";
  while (client.available()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
    headers += line;
  }

  Serial.println("Response headers:");
  Serial.println(headers);

  String body = "";
  while (client.available()) {
    body += client.readString();
  }

  client.stop();
  body.trim();
  return body;
}

void doFirmwareUpdate(const String& fwUrl) {
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(20000);

  t_httpUpdate_return ret = ESPhttpUpdate.update(client, fwUrl);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("Update failed. Error (%d): %s\n",
                    ESPhttpUpdate.getLastError(),
                    ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("No update available");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("Update success, rebooting");
      break;
  }
}
