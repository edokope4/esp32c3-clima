#include "config.h"
#include "DHT.h"
#include <ctype.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <esp_sleep.h>

#define DHTPIN 4       // GPIO del ESP32-C3 conectado al pin DAT del DHT22
#define DHTTYPE DHT22  // Definimos que el sensor es un DHT22

DHT dht(DHTPIN, DHTTYPE);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long lastPublish = 0;
unsigned long lastPublishAttempt = 0;
bool awaitingFirstReport = true;

const unsigned long AWAKE_TIME_MS = (unsigned long)AWAKE_TIME_MIN * 60UL * 1000UL;

bool extractQuoted(const String& body, const char* key, char* out, size_t outLen) {
  String token = String("\"") + key + "\"";
  int pos = body.indexOf(token);
  if (pos < 0) {
    return false;
  }

  pos = body.indexOf(':', pos + token.length());
  if (pos < 0) {
    return false;
  }
  pos++;

  while (pos < (int)body.length() && isspace((unsigned char)body[pos])) {
    pos++;
  }
  if (pos >= (int)body.length() || body[pos] != '"') {
    return false;
  }
  pos++;

  int end = body.indexOf('"', pos);
  if (end < 0 || (size_t)(end - pos) >= outLen) {
    return false;
  }

  body.substring(pos, end).toCharArray(out, outLen);
  return true;
}

bool fetchDateTime(char* datetime, size_t datetimeLen) {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setConnectTimeout(10000);
  http.setTimeout(10000);
  if (!http.begin(client, TIME_API_URL)) {
    Serial.println(F("No se pudo iniciar la consulta de hora."));
    return false;
  }

  Serial.println(F("Consultando hora en api.kope.cl..."));
  int status = http.GET();
  if (status != HTTP_CODE_OK) {
    Serial.print(F("getDateTime respondió: "));
    Serial.println(status);
    http.end();
    return false;
  }

  String body = http.getString();
  http.end();

  char date[16];
  char timeText[16];
  if (!extractQuoted(body, "date", date, sizeof(date)) ||
      !extractQuoted(body, "time", timeText, sizeof(timeText))) {
    Serial.println(F("getDateTime no trajo date y time."));
    return false;
  }

  snprintf(datetime, datetimeLen, "%s %s", date, timeText);
  mqtt.loop();
  return true;
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.print(F("Conectando a WiFi "));
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print('.');
    attempts++;
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("No se pudo conectar al WiFi. Se reintentará."));
    return;
  }

  Serial.print(F("WiFi conectado. IP: "));
  Serial.println(WiFi.localIP());

  char datetime[32];
  if (fetchDateTime(datetime, sizeof(datetime))) {
    Serial.print(F("Hora al conectar: "));
    Serial.println(datetime);
  } else {
    Serial.println(F("No se pudo obtener la hora al conectar el WiFi."));
  }
}

void connectMqtt() {
  if (mqtt.connected() || WiFi.status() != WL_CONNECTED) {
    return;
  }

  // El identificador de cliente incluye parte del MAC para no chocar con otro equipo.
  char clientId[48];
  snprintf(clientId, sizeof(clientId), "%s-%04X", MQTT_CLIENT_ID,
           (unsigned)(ESP.getEfuseMac() & 0xFFFF));

  Serial.print(F("Conectando a MQTT "));
  Serial.println(MQTT_BROKER);

  if (mqtt.connect(clientId)) {
    Serial.print(F("MQTT conectado. Tópico: "));
    Serial.println(MQTT_TOPIC);
  } else {
    Serial.print(F("Fallo MQTT, estado: "));
    Serial.println(mqtt.state());
  }
}

bool publishReading() {
  float humedad = dht.readHumidity();
  float temperatura = dht.readTemperature();

  if (isnan(humedad) || isnan(temperatura)) {
    Serial.println(F("¡Error! No se pudo leer el sensor DHT22. Revisa las conexiones."));
    return false;
  }

  Serial.print(F("Humedad: "));
  Serial.print(humedad, 1);
  Serial.print(F("%  |  Temperatura: "));
  Serial.print(temperatura, 1);
  Serial.println(F("°C"));

  if (!mqtt.connected()) {
    Serial.println(F("MQTT desconectado: la lectura quedó solo en el monitor serial."));
    return false;
  }

  char datetime[32];
  if (!fetchDateTime(datetime, sizeof(datetime))) {
    Serial.println(F("No se publica: no hay hora de envío."));
    return false;
  }

  char tempStr[16];
  char humStr[16];
  dtostrf(temperatura, 0, 1, tempStr);
  dtostrf(humedad, 0, 1, humStr);

  char payload[192];
  snprintf(payload, sizeof(payload),
           "{\"idDispositivo\":\"%s\",\"temperatura\":%s,\"humedad\":%s,\"datetime\":\"%s\"}",
           ID_DISPOSITIVO, tempStr, humStr, datetime);

  if (!mqtt.publish(MQTT_TOPIC, payload)) {
    Serial.println(F("No se pudo publicar en MQTT."));
    return false;
  }

  Serial.print(F("Publicado: "));
  Serial.println(payload);
  return true;
}

void enterDeepSleep() {
  Serial.print(F("Deep sleep por "));
  Serial.print(SLEEP_TIME_MIN);
  Serial.println(F(" minutos."));
  Serial.flush();

  if (mqtt.connected()) {
    mqtt.disconnect();
  }
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100);

  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_TIME_MIN * 60ULL * 1000000ULL);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);

  // Pausa fundamental para permitir que el USB nativo del ESP32-C3 se establezca
  delay(3000);

  Serial.println();
  Serial.println(F("===================================="));
  Serial.println(F(" Clima ESP32-C3: DHT22 + WiFi + MQTT"));
  Serial.println(F("===================================="));

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println(F("Despertar por temporizador."));
  } else {
    Serial.println(F("Arranque."));
  }
  Serial.print(AWAKE_TIME_MIN);
  Serial.print(F(" min despierto, "));
  Serial.print(SLEEP_TIME_MIN);
  Serial.println(F(" min en deep sleep."));

  dht.begin();
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);

  // El DHT22 necesita estabilizarse antes de la primera lectura de este arranque.
  delay(2000);

  // Al despertar: WiFi, MQTT y recién ahí el JSON.
  connectWiFi();
  connectMqtt();
  if (publishReading()) {
    awaitingFirstReport = false;
    lastPublish = millis();
  }
  lastPublishAttempt = millis();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (mqtt.connected()) {
      mqtt.disconnect();
    }
    connectWiFi();
  }

  bool mqttJustConnected = false;
  if (!mqtt.connected()) {
    connectMqtt();
    mqttJustConnected = mqtt.connected();
  }
  mqtt.loop();

  unsigned long now = millis();
  bool reportDue = awaitingFirstReport || mqttJustConnected || (now - lastPublish >= PUBLISH_INTERVAL_MS);
  if (reportDue && mqtt.connected() && (now - lastPublishAttempt >= 2000)) {
    lastPublishAttempt = now;
    if (publishReading()) {
      awaitingFirstReport = false;
      lastPublish = now;
    }
  }

  if (millis() >= AWAKE_TIME_MS) {
    enterDeepSleep();
  }

  // Cede tiempo a la pila WiFi del ESP32.
  delay(100);
}
