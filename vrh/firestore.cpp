#include "firestore.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include <memory>

#include "secrets.h"

#define SETTINGS_POLL_INTERVAL_MS 3000

// Defaults used until first successful Firestore fetch.
DeviceSettings settings = { 12, 100, 500, 3000 };
PendingCommand pendingCommand = { "", "" };
String lastResultCommandId = "";

static unsigned long lastSettingsFetchMs = 0;

static String firestoreDocUrl() {
  String url = "https://firestore.googleapis.com/v1/projects/";
  url += FIREBASE_PROJECT_ID;
  url += "/databases/(default)/documents/devices/";
  url += DEVICE_UID;
  return url;
}

bool fetchSettings() {
  lastSettingsFetchMs = millis();

  if (WiFi.status() != WL_CONNECTED) return false;

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();

  HTTPClient https;
  String url = firestoreDocUrl();
  url += "?key=";
  url += FIREBASE_API_KEY;

  if (!https.begin(*client, url)) {
    Serial.println("Firestore: https.begin failed");
    return false;
  }

  int code = https.GET();
  if (code != 200) {
    Serial.print("Firestore GET non-200: ");
    Serial.println(code);
    https.end();
    return false;
  }

  String payload = https.getString();
  https.end();

  Serial.print("Firestore payload bytes: ");
  Serial.println(payload.length());

  StaticJsonDocument<3072> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("Firestore JSON parse failed: ");
    Serial.println(err.c_str());
    return false;
  }

  JsonObject fields = doc["fields"];
  if (!fields.isNull()) {
    if (fields["gyroSensitivity"]["integerValue"].is<const char*>())
      settings.gyroSensitivity = atoi(fields["gyroSensitivity"]["integerValue"]);
    if (fields["volume"]["integerValue"].is<const char*>())
      settings.volume = atoi(fields["volume"]["integerValue"]);
    if (fields["minDelay"]["integerValue"].is<const char*>())
      settings.minDelay = atoi(fields["minDelay"]["integerValue"]);
    if (fields["maxDelay"]["integerValue"].is<const char*>())
      settings.maxDelay = atoi(fields["maxDelay"]["integerValue"]);

    // command is a mapValue: { fields: { id: stringValue, type: stringValue } }
    JsonVariant cmdFields = fields["command"]["mapValue"]["fields"];
    if (cmdFields.is<JsonObject>()) {
      const char* idStr   = cmdFields["id"]["stringValue"]   | "";
      const char* typeStr = cmdFields["type"]["stringValue"] | "";
      pendingCommand.id   = idStr;
      pendingCommand.type = typeStr;
    } else {
      pendingCommand.id   = "";
      pendingCommand.type = "";
    }

    if (fields["lastResultCommandId"]["stringValue"].is<const char*>()) {
      const char* rid = fields["lastResultCommandId"]["stringValue"];
      lastResultCommandId = rid;
    } else {
      lastResultCommandId = "";
    }
  }

  Serial.print("Settings: gyro=");
  Serial.print(settings.gyroSensitivity);
  Serial.print("% vol=");
  Serial.print(settings.volume);
  Serial.print(" delay=");
  Serial.print(settings.minDelay);
  Serial.print("..");
  Serial.print(settings.maxDelay);
  Serial.print(" cmd=");
  Serial.print(pendingCommand.id.length() ? pendingCommand.id : String("-"));
  Serial.print(":");
  Serial.println(pendingCommand.type);

  return true;
}

void maybeFetchSettings() {
  if (millis() - lastSettingsFetchMs >= SETTINGS_POLL_INTERVAL_MS) {
    fetchSettings();
  }
}

bool publishResult(const String& commandId, int durationMs) {
  if (WiFi.status() != WL_CONNECTED) return false;

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();

  HTTPClient https;
  // updateMask scopes the PATCH so other fields (settings, command) stay intact.
  String url = firestoreDocUrl();
  url += "?updateMask.fieldPaths=lastResultMs";
  url += "&updateMask.fieldPaths=lastResultCommandId";
  url += "&key=";
  url += FIREBASE_API_KEY;

  if (!https.begin(*client, url)) {
    Serial.println("Firestore publishResult: https.begin failed");
    return false;
  }

  StaticJsonDocument<256> body;
  JsonObject fields = body.createNestedObject("fields");
  fields["lastResultMs"]["integerValue"]        = String(durationMs);
  fields["lastResultCommandId"]["stringValue"]  = commandId;

  String payload;
  serializeJson(body, payload);

  https.addHeader("Content-Type", "application/json");
  int code = https.PATCH(payload);
  https.end();

  if (code < 200 || code >= 300) {
    Serial.print("Firestore PATCH non-2xx: ");
    Serial.println(code);
    return false;
  }

  Serial.print("Result published: cmd=");
  Serial.print(commandId);
  Serial.print(" durationMs=");
  Serial.println(durationMs);
  return true;
}
