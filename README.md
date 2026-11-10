# ESP32-C3 clima

Firmware para un ESP32-C3 con sensor DHT22. Al despertar conecta el WiFi, consulta la hora y publica por MQTT un JSON con el identificador del dispositivo, la temperatura, la humedad y la fecha-hora. Después permanece despierto el tiempo indicado en la configuración y entra en deep sleep.

## Conexiones

El dato del DHT22 entra por el **GPIO 4**. La alimentación del sensor es la de 3,3 V de la placa. El pin 3 del DHT22 no se conecta.

```
        ESP32-C3                              DHT22
   +------------------+                 +------------------+
   |                  |                 |  1 VCC           |
   |  3V3         GND |-----------------|  4 GND           |
   |                  |                 |                  |
   |  GPIO 4          |------[4,7k]-----|  2 DAT           |
   |                  |         |        |  3 NC  (libre)  |
   +------------------+         |        +------------------+
                                |
                               3V3
```

La resistencia de 4,7 kΩ va entre DAT y 3V3. Muchos módulos DHT22 ya la traen en la plaqueta; en ese caso no se agrega otra.

| ESP32-C3 | DHT22 | Uso |
| --- | --- | --- |
| 3V3 | Pin 1, VCC | Alimentación |
| GPIO 4 | Pin 2, DAT | Datos |
| — | Pin 3, NC | Sin conexión |
| GND | Pin 4, GND | Tierra común |

El monitor serial usa el USB nativo de la placa, a 115200 baudios. Durante el deep sleep ese USB se desconecta y vuelve a aparecer al despertar.

## Archivo de configuración

El sketch incluye `sketch_nov4a/config.h`. Ese archivo **no está en el repositorio**: `.gitignore` lo deja solo en la máquina donde se compila.

En el repositorio está `sketch_nov4a/config.example.h`, con los mismos campos y sin la red ni la contraseña. Para usarlo:

```bash
cp sketch_nov4a/config.example.h sketch_nov4a/config.h
```

Después se edita `config.h` en local: la red, la contraseña y el tópico MQTT. No hay que commitearlo.

| Campo | Qué define |
| --- | --- |
| `WIFI_SSID` | Nombre de la red WiFi. |
| `WIFI_PASSWORD` | Contraseña de esa red. |
| `MQTT_BROKER` | Servidor MQTT. El ejemplo usa `broker.hivemq.com`. |
| `MQTT_PORT` | Puerto del broker. `1883` es el puerto sin cifrado. |
| `MQTT_TOPIC` | Tópico donde se publica el JSON. En el ejemplo figura como `<TOPIC>`. |
| `MQTT_CLIENT_ID` | Prefijo del cliente MQTT. El firmware le agrega parte del MAC para no repetir el identificador. |
| `ID_DISPOSITIVO` | Valor del campo `idDispositivo` dentro del JSON. |
| `PUBLISH_INTERVAL_MS` | Cada cuántos milisegundos vuelve a publicar mientras está despierto. El DHT22 necesita al menos 2000. |
| `AWAKE_TIME_MIN` | Minutos encendido, con WiFi y MQTT. |
| `SLEEP_TIME_MIN` | Minutos en deep sleep. Al despertar conecta de nuevo y envía el JSON. |
| `TIME_API_URL` | Servicio que devuelve la fecha y la hora. Se consulta al conectar el WiFi y otra vez justo antes de cada publicación. |

El JSON publicado tiene esta forma:

```json
{"idDispositivo":"lto-monitor-clima","temperatura":23.5,"humedad":48.2,"datetime":"2026-11-10 09:45:08"}
```

`datetime` sale de [api.kope.cl/api/getDateTime](https://api.kope.cl/api/getDateTime). Si esa consulta falla, esa medición no se publica.
