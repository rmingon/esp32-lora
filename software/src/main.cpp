#include <WiiChuck.h>
#include <Wire.h>
Accessory nunchuck1;

#include <SPI.h>
#include <LoRa.h>
SPIClass spi;

#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>

#include <AsyncTCP.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// OLED display size
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SH1106 display(0);

void loraSend(byte*, size_t);
void onReceive(int);
void drawRoll(int);
void drawPitch(int);

const int numDoubles = 5;
const int doubleSize = sizeof(double);
byte buffer[numDoubles * doubleSize];
double receivedValues[numDoubles];

double speed = 0;
double lon = 0;
double lat = 0;

double data[5] = {0.0, 0.0, 0.0, 0.0, 0.0};

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin("ssid", "passsword");
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void setup() {
	Serial.begin(115200);
  Wire.begin(21,22);
  initWiFi();
  server.begin();
  nunchuck1.begin();
  if (nunchuck1.type == Unknown) {
		nunchuck1.type = NUNCHUCK;
	}

  spi.begin(14, 12, 13); // SCK, MISO, MOSI, SS
  LoRa.setSPI(spi);
  LoRa.setPins(15, 26, 16);
  if (!LoRa.begin(868E6)) {
      Serial.println("LoRa init failed. Check your connections.");
      while (true);                       // if failed, do nothing
  }
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(250E3);
  LoRa.setTxPower(20);
  LoRa.setSyncWord(0xF3);
  LoRa.receive();

  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    (void)len;

    if (type == WS_EVT_CONNECT) {
      ws.textAll("new client connected");
      Serial.println("ws connect");
      client->setCloseClientOnQueueFull(false);
      client->ping();

    } else if (type == WS_EVT_DISCONNECT) {
      ws.textAll("client disconnected");
      Serial.println("ws disconnect");

    } else if (type == WS_EVT_ERROR) {
      Serial.println("ws error");

    } else if (type == WS_EVT_PONG) {
      Serial.println("ws pong");

    } else if (type == WS_EVT_DATA) {
      AwsFrameInfo *info = (AwsFrameInfo *)arg;
      Serial.printf("index: %" PRIu64 ", len: %" PRIu64 ", final: %" PRIu8 ", opcode: %" PRIu8 "\n", info->index, info->len, info->final, info->opcode);
      String msg = "";
      if (info->final && info->index == 0 && info->len == len) {
        if (info->opcode == WS_TEXT) {
          data[len] = 0;
          Serial.printf("ws text: %s\n", (char *)data);
        }
      }
    }
  });

  // shows how to prevent a third WS client to connect
  server.addHandler(&ws).addMiddleware([](AsyncWebServerRequest *request, ArMiddlewareNext next) {
    // ws.count() is the current count of WS clients: this one is trying to upgrade its HTTP connection
    if (ws.count() > 1) {
      // if we have 2 clients or more, prevent the next one to connect
      request->send(503, "text/plain", "Server is busy");
    } else {
      // process next middleware and at the end the handler
      next();
    }
  });

  server.addHandler(&ws);

  server.begin();

  Serial.println("LoRa Initializing OK!");

  display.begin();
  display.clearDisplay(); 
  display.setTextSize(1); 
  display.setTextColor(WHITE);
}

void drawRoll(int degree) {
  display.drawLine(30, 20 + degree, display.width() - 38, 20 - degree,  WHITE);
}

void drawRollPlane(int degree) {
  display.drawLine(0, 20 + degree, display.width() - 8, 20 - degree,  WHITE);
}

void drawPitch(int degree) {
  display.drawLine(120, display.height() / 2 + degree, display.width() , display.height() / 2 + degree,  WHITE);
}

void drawPitchPlane(int degree) {
  display.drawLine(120, display.height() / 2 + degree, display.width() , display.height() / 2 + degree,  WHITE);
}

void loop() {
  display.clearDisplay(); 
	nunchuck1.readData();

  byte cmd = 0b00000000;

  int centerX = 60;
  int centerY = 20;
  int radius = 12;

  display.drawCircle(centerX, centerY, radius, WHITE);
  display.fillRect(centerX - radius , centerY, radius * 3, radius + 2, BLACK);

  display.drawLine(120, 0, 120 , display.height(),  WHITE);

  int roll = (nunchuck1.getAccelX() - 475) / 10;
  int pitch = (nunchuck1.getAccelY() - 475) / 9;

  drawRoll(roll);
  drawPitch(pitch);
  drawRollPlane(int(trunc(receivedValues[4])));
  drawPitchPlane(int(trunc(receivedValues[3])));

  display.setCursor(0, 0);
  if(nunchuck1.getButtonC()) {
    cmd |= 0b00000001;
    display.print("C");
  } else {
    display.print(" ");
  }

  display.setCursor(8, 0);
  if(nunchuck1.getButtonZ()) {
    cmd |= 0b00000010;
    display.print("Z");
  } else {
    display.print(" ");
  }


  display.setCursor(40, 0);
  display.println(WiFi.localIP());
  
  byte data[] = {
    cmd,
    (byte)(roll + 127),
    (byte)(pitch + 127)
  };

  loraSend(data, sizeof(data));

  onReceive(LoRa.parsePacket());

  display.setCursor(0, 48);
  display.print("RSSI ");
  display.print(LoRa.packetRssi());
  display.print("KMH ");
  display.print(receivedValues[2], 2);

  display.setCursor(0, 56);
  display.print("LAT ");
  display.print(receivedValues[0], 4);
  display.print("LON ");
  display.print(receivedValues[1], 4);
  display.display();
  delay(50);
}

void decodeLora(byte* data, size_t length) {

}

void onReceive(int packetSize) {
  if (packetSize == 0) return;

  if (packetSize == numDoubles * doubleSize) {
    for (int i = 0; i < packetSize; i++) {
      buffer[i] = LoRa.read();
    }

    // Unpack bytes back into doubles
    for (int i = 0; i < numDoubles; i++) {
        memcpy(&receivedValues[i], buffer + i * doubleSize, doubleSize);
    }

    // Print received doubles
    Serial.println("Received values:");
    for (int i = 0; i < numDoubles; i++) {
      Serial.print("Double ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(receivedValues[i], 6);
    }
    Serial.println("-----------");
  }
  Serial.print(" || RSSI: ");
  Serial.println(LoRa.packetRssi());
  Serial.println();
}

void loraSend(byte* data, size_t length) {
  LoRa.beginPacket();
  LoRa.write(data, length);
  LoRa.endPacket();
  LoRa.receive();
}