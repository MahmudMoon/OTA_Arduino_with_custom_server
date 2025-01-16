#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

// WiFi credentials
const char *ssid = "Thor";
const char *password = "3.3password";

// Update server information
const char *firmwareUrl = "http://192.168.0.107:3000/latestfile"; // Replace with your server's IP or domain

const char *updateUrl = "http://192.168.0.107:3000/latest";

int currentVersion = 1;
int fetchedVersion = 0;

void setup() {
  // Start Serial communication
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi: ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi.");
  
  // Check for updates
  checkForUpdates();
}


void loop() {
  delay(1000); // Check every 10 seconds (not recommended for production)
}

void checkForUpdates() {
  // Ensure Wi-Fi is connected
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    //String updateUrl = String(updateServer) + "/update-check?version=" + firmwareVersion;
    http.begin(updateUrl);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) { // If update-check request was successful
      String payload = http.getString();
      Serial.println("Update response: " + payload);


      StaticJsonDocument<200> jsonDoc;

    // Parse the JSON string
      DeserializationError error = deserializeJson(jsonDoc, payload);

      // Check for errors
      if (error) {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.c_str());
        return;
      }

      // Access JSON elements by their keys
      fetchedVersion = jsonDoc["version"];  // "version"

      if(fetchedVersion > 0 && fetchedVersion > currentVersion){
        Serial.println("Fetching update");
        downloadAndUpdate();
      }else {
        Serial.println("No updates available.");
      }
    } else {
      Serial.println("Failed to check for updates. HTTP response code: " + String(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("Wi-Fi not connected");
  }
}

void downloadAndUpdate() {
  WiFiClient client;
  HTTPClient http;

  Serial.println("Starting OTA update...");
  http.begin(client, firmwareUrl);
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) { // If the .bin file is found
    int contentLength = http.getSize();
    bool canBegin = Update.begin(contentLength);

    if (canBegin) {
      Serial.println("Begin OTA. Waiting for data...");
      size_t written = Update.writeStream(http.getStream());

      if (written == contentLength) {
        Serial.println("Written : " + String(written) + " successfully");
      } else {
        Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
      }

      if (Update.end()) {
        if (Update.isFinished()) {
          Serial.println("OTA update complete!");
          Serial.println("Restarting...");
          ESP.restart(); // Restart ESP32 after successful update
        } else {
          Serial.println("Update not finished. Something went wrong!");
        }
      } else {
        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      }
    } else {
      Serial.println("Not enough space to begin OTA");
    }
  } else {
    Serial.println("Firmware file not found. HTTP response code: " + String(httpResponseCode));
  }
  http.end();
}
