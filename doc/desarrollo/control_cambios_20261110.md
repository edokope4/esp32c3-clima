# Control de Cambios — v20261110

**Versión:** 20261110
**Estado:** en curso
**Versión anterior liberada:** ninguna

---

## Resumen

| # | Cambio | Tipo | Requiere acción al instalar |
|---|--------|------|-----------------------------|
| 1 | El módulo de clima envía temperatura y humedad a un servidor de mensajes | Nueva funcionalidad | No |
| 2 | Cada reporte de clima incluye la hora de envío tomada del servicio de KOPE | Nueva funcionalidad | No |
| 3 | El módulo se apaga 10 minutos y vuelve a enviar al despertar | Mejora | No |

---

## 1. El módulo de clima envía temperatura y humedad a un servidor de mensajes

**Tipo:** Nueva funcionalidad · **Fecha:** 10/11/2026

**Situación anterior:** El módulo medía temperatura y humedad y solo las mostraba en el puerto serial, conectado a un computador. Nadie fuera de ese cable podía ver las lecturas.

**Qué cambia:** El módulo se conecta a la red WiFi configurada y publica un mensaje con el identificador del dispositivo, la temperatura y la humedad. Mientras permanece encendido, repite el envío cada 60 segundos. El mensaje queda en el servidor `broker.hivemq.com`, en el tópico indicado en la configuración. Si no hay red o el servidor no responde, la medición sigue viéndose en el puerto serial y se reintenta la conexión. El ritmo de encendido y apagado está en el punto 3.

**Efecto para la operación:** Quien esté suscrito a ese tópico recibe las lecturas sin estar conectado al módulo. El servidor es público: cualquiera que conozca el tópico puede leer los mensajes. El identificador publicado es `lto-monitor-clima`.

---

## 2. Cada reporte de clima incluye la hora de envío tomada del servicio de KOPE

**Tipo:** Nueva funcionalidad · **Fecha:** 10/11/2026

**Situación anterior:** El mensaje de clima no decía a qué hora se había enviado. El módulo no consultaba una hora de referencia.

**Qué cambia:** Al conectarse a la red, el módulo consulta la hora en el servicio de KOPE. Vuelve a consultarla justo antes de cada envío y agrega el dato `datetime` al mensaje, con fecha y hora juntas (por ejemplo `2026-11-10 09:45:08`). Si esa consulta falla, esa medición no se publica.

**Efecto para la operación:** Quien lee el tópico ve la hora de envío según [api.kope.cl](https://api.kope.cl/api/getDateTime), no según un reloj interno del módulo. Sin respuesta de ese servicio no hay mensaje nuevo en el tópico, aunque la medición siga viéndose en el puerto serial.

---

## 3. El módulo se apaga 10 minutos y vuelve a enviar al despertar

**Tipo:** Mejora · **Fecha:** 10/11/2026

**Situación anterior:** El módulo quedaba encendido todo el tiempo y podía seguir enviando cada 60 segundos sin pausa.

**Qué cambia:** El módulo permanece encendido 5 minutos y después se apaga 10 minutos. Al despertar conecta la red, abre la sesión de mensajes y envía el reporte. Durante esos 5 minutos sigue enviando cada 60 segundos. Esos dos tiempos parten en 5 y 10 minutos.

**Efecto para la operación:** Los reportes llegan en la ventana de 5 minutos y dejan de llegar durante los 10 minutos apagado. En ese lapso el módulo no está en la red. Quien espere una lectura continua va a ver huecos de 10 minutos.

---

## Referencias

No hay pauta de instalación: la actualización se aplica con el flasheo habitual del firmware.
