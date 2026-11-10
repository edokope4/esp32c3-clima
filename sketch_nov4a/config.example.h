#ifndef CONFIG_H
#define CONFIG_H

// Copiar este archivo a config.h y completar la red local y el tópico.
// config.h no se sube al repositorio.

// === WiFi ===
#define WIFI_SSID "<ESSID>"
#define WIFI_PASSWORD "<PASSWORD>"

// === MQTT ===
// Broker público HiveMQ (sin usuario ni contraseña), puerto estándar sin TLS.
#define MQTT_BROKER "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_TOPIC "<TOPIC>"
#define MQTT_CLIENT_ID "esp32c3-clima"

// Identificador que viaja en el campo idDispositivo del JSON.
#define ID_DISPOSITIVO "lto-monitor-clima"

// El DHT22 exige al menos 2 segundos entre lecturas.
// Mientras el módulo está despierto, vuelve a publicar cada este intervalo.
#define PUBLISH_INTERVAL_MS 60000

// Minutos encendido (WiFi, MQTT y reportes) y minutos en deep sleep.
#define AWAKE_TIME_MIN 5
#define SLEEP_TIME_MIN 10

// Hora oficial consultada al conectar el WiFi y antes de cada publicación.
#define TIME_API_URL "https://api.kope.cl/api/getDateTime"

#endif
