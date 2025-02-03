#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <SPIFFS.h>
#include <ArduinoOTA.h>

// Configuración WiFi
const char *ssid = "Yeisonint-TLM";
const char *password = "23HZFVWV32AH8";

// Pines de control para los motores
#define M1 12
#define M2 13
#define M3 14
#define M4 15

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WebServer server(80);
WebSocketsServer webSocket(81);
TaskHandle_t streamTaskHandle = NULL;

WiFiClient streamClient;
bool streamActive = false;

void startCamera()
{
  camera_config_t config = {
    .pin_pwdn       = PWDN_GPIO_NUM,
    .pin_reset      = RESET_GPIO_NUM,
    .pin_xclk       = XCLK_GPIO_NUM,
    .pin_sccb_sda   = SIOD_GPIO_NUM,
    .pin_sccb_scl   = SIOC_GPIO_NUM,
    .pin_d7         = Y9_GPIO_NUM,
    .pin_d6         = Y8_GPIO_NUM,
    .pin_d5         = Y7_GPIO_NUM,
    .pin_d4         = Y6_GPIO_NUM,
    .pin_d3         = Y5_GPIO_NUM,
    .pin_d2         = Y4_GPIO_NUM,
    .pin_d1         = Y3_GPIO_NUM,
    .pin_d0         = Y2_GPIO_NUM,
    .pin_vsync      = VSYNC_GPIO_NUM,
    .pin_href       = HREF_GPIO_NUM,
    .pin_pclk       = PCLK_GPIO_NUM,

    .xclk_freq_hz   = 20000000,
    .ledc_timer     = LEDC_TIMER_0,
    .ledc_channel   = LEDC_CHANNEL_0,
    .pixel_format   = PIXFORMAT_JPEG,
    .frame_size     = FRAMESIZE_QVGA,
    .jpeg_quality   = 15,
    .fb_count       = 1,
    .grab_mode      = CAMERA_GRAB_WHEN_EMPTY
  };

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Error al iniciar la cámara: 0x%x", err);
    return;
  }
}

// Capturar imagen de la cámara
// void handleCapture()
// {
//   camera_fb_t *fb = esp_camera_fb_get();
//   if (!fb)
//   {
//     server.send(500, "text/plain", "Error al capturar imagen");
//     return;
//   }
//   server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
//   esp_camera_fb_return(fb);
// }

// void handleStream()
// {
//   WiFiClient client = server.client();

//   if (!client.connected())
//     return;

//   String response = "HTTP/1.1 200 OK\r\n";
//   response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
//   client.print(response);

//   while (client.connected())
//   {
//     camera_fb_t *fb = esp_camera_fb_get();
//     if (!fb)
//     {
//       Serial.println("Error al capturar el frame");
//       break;
//     }

//     client.print("--frame\r\n");
//     client.print("Content-Type: image/jpeg\r\n");
//     client.print("Content-Length: " + String(fb->len) + "\r\n\r\n");
//     client.write(fb->buf, fb->len);
//     client.print("\r\n");

//     esp_camera_fb_return(fb);
//     delay(100);
//   }
// }

void handleStream() {
  streamClient = server.client();
  if (!streamClient.connected()) return;

  Serial.println("📡 Cliente conectado al streaming...");
  streamActive = true;

  streamClient.println("HTTP/1.1 200 OK");
  streamClient.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  streamClient.println();
}


void streamTask(void* pvParameters) {
    while (true) {
        if (streamActive && streamClient.connected()) {
            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb) {
                Serial.println("⚠️ Error al capturar frame");
                vTaskDelay(10);
                continue;
            }

            streamClient.println("--frame");
            streamClient.println("Content-Type: image/jpeg");
            streamClient.println("Content-Length: " + String(fb->len));
            streamClient.println();
            streamClient.write(fb->buf, fb->len);
            streamClient.println();

            esp_camera_fb_return(fb);
            vTaskDelay(100);
        } else {
            if (streamActive) {
                Serial.println("❌ Cliente desconectado del streaming");
                streamActive = false;
            }
            vTaskDelay(100);
        }
    }
}


// Iniciar SPIFFS
void initSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("Error montando SPIFFS");
  }
  else
  {
    Serial.println("SPIFFS montado correctamente");
  }
}

// Servir archivos desde SPIFFS
void handleFileRequest()
{
  String path = server.uri();
  if (path.endsWith("/"))
    path += "index.html";

  String contentType = "text/html";
  if (path.endsWith(".css"))
    contentType = "text/css";
  else if (path.endsWith(".js"))
    contentType = "application/javascript";

  File file = SPIFFS.open(path, "r");
  if (!file)
  {
    server.send(404, "text/plain", "Archivo no encontrado");
    return;
  }

  server.streamFile(file, contentType);
  file.close();
}

// Manejo de motores con WebSockets
void handleWebSocketMessage(uint8_t num, uint8_t *payload, size_t length)
{
  String command = String((char *)payload).substring(0, length);

  if (command == "forward")
  {
    digitalWrite(M1, HIGH);
    digitalWrite(M2, LOW);
  }
  else if (command == "backward")
  {
    digitalWrite(M1, LOW);
    digitalWrite(M2, HIGH);
  }
  else if (command == "left")
  {
    digitalWrite(M3, HIGH);
    digitalWrite(M4, LOW);
  }
  else if (command == "right")
  {
    digitalWrite(M3, LOW);
    digitalWrite(M4, HIGH);
  }
  else if (command == "stop")
  {
    digitalWrite(M1, LOW);
    digitalWrite(M2, LOW);
    digitalWrite(M3, LOW);
    digitalWrite(M4, LOW);
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("⚠️ Cliente [%u] desconectado\n", num);
    break;

  case WStype_CONNECTED:
    Serial.printf("✅ Cliente [%u] conectado desde %s\n", num, server.client().remoteIP().toString().c_str());
    webSocket.sendTXT(num, "Conectado");
    break;

  case WStype_TEXT:
    Serial.printf("📩 Mensaje recibido de [%u]: %s\n", num, payload);
    handleWebSocketMessage(num, payload, length);
    break;

  case WStype_BIN:
    Serial.printf("📂 Datos binarios recibidos de [%u]\n", num);
    break;

  case WStype_ERROR:
    Serial.printf("❌ Error en WebSocket de [%u]\n", num);
    break;

  case WStype_FRAGMENT_TEXT_START:
  case WStype_FRAGMENT_BIN_START:
  case WStype_FRAGMENT:
  case WStype_FRAGMENT_FIN:
    Serial.printf("📡 Fragmento recibido de [%u]\n", num);
    break;
  }
}

void initOTA()
{
  ArduinoOTA.begin();
}

void setup()
{
  // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);

  pinMode(M1, OUTPUT);
  pinMode(M2, OUTPUT);
  pinMode(M3, OUTPUT);
  pinMode(M4, OUTPUT);

  digitalWrite(M1, LOW);
  digitalWrite(M2, LOW);
  digitalWrite(M3, LOW);
  digitalWrite(M4, LOW);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  startCamera();
  initSPIFFS();
  initOTA();

  server.onNotFound(handleFileRequest);
  // server.on("/capture", HTTP_GET, handleCapture);
  server.on("/stream", HTTP_GET, handleStream);
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  disableCore0WDT();
  xTaskCreatePinnedToCore(
        streamTask, "StreamTask", 10000, NULL, 1, &streamTaskHandle, 0);
}

void loop()
{
  server.handleClient();
  webSocket.loop();
  ArduinoOTA.handle();
}
