#include <WiiChuck.h>
#include <Wire.h>
Accessory nunchuck1;

#include <SPI.h>
#include <LoRa.h>
SPIClass spi;

#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>


// OLED display size
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SH1106 display(0);

void loraSend(byte*, size_t);
void onReceive(int);
void drawRoll(int);
void drawPitch(int);

void setup() {
	Serial.begin(115200);
  Wire.begin(21,22);
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
  LoRa.onReceive(onReceive);
  LoRa.receive();
  Serial.println("LoRa Initializing OK!");

  display.begin();
  display.clearDisplay(); 
  display.setTextSize(1); 
  display.setTextColor(WHITE);
}

void drawRoll(int degree) {
  display.drawLine(30, display.height() / 2 + degree, display.width() - 38, display.height() / 2 - degree,  WHITE);
}

void drawPitch(int degree) {
  display.drawLine(120, display.height() / 2 + degree, display.width() , display.height() / 2 + degree,  WHITE);
}

void loop() {
  display.clearDisplay(); 
	nunchuck1.readData();

  byte cmd = 0b00000000;

  int centerX = 60;
  int centerY = 32;
  int radius = 12;

  // Draw full circle
  display.drawCircle(centerX, centerY, radius, WHITE);

  // Cover bottom half to make it a top semi-circle
  display.fillRect(centerX - radius , centerY, radius * 3, radius + 2, BLACK);

  display.drawLine(120, 0, 120 , display.height(),  WHITE);

  int roll = (nunchuck1.getAccelX() - 475) / 10;
  int pitch = (nunchuck1.getAccelY() - 475) / 9;

  drawRoll(roll);
  drawPitch(pitch);

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
  
  byte data[] = {
    cmd,
    (byte)(roll + 127),
    (byte)(pitch + 127)
  };

  loraSend(data, sizeof(data));
  display.display();
  delay(50);
}

void decodeLora(byte* data, size_t length) {

}

void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  int recipient = LoRa.read();
  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  int length = incoming.length();
  byte byteArray[length + 1];
  incoming.getBytes(byteArray, length + 1);

  decodeLora(byteArray, length + 1);

  Serial.print(incoming);
  Serial.print(" || RSSI: ");
  Serial.println(LoRa.packetRssi());
  display.setCursor(16, 0);
  display.print("RSSI: ");
  display.print(LoRa.packetRssi());
  Serial.println();
}

void loraSend(byte* data, size_t length) {
  LoRa.beginPacket();
  LoRa.write(data, length);
  LoRa.endPacket();
  LoRa.receive();
}