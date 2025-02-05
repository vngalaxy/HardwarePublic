#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// ESP32 C3 SERIAL1 (second UART)
HardwareSerial mySerial1(1);

#define rxPin  20
#define txPin  21
#define EN_RAK 10
#define LED_R 8
#define LED_G 9
#define LED_B 2

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char* ssid = "RFThings Vietnam";
const char* password = "khongvaoduoc!";

String dataHTML = ""; // String to store the HTML content

// List of AT commands with corresponding LED bit patterns
const char* atCommands[] = {
  "ATC+VER",
  "ATC+FAN=1",
  "ATC+SCD4x",
  "ATC+BME680",
  "ATC+SGP30",
  "ATC+KXTJ3",
  "ATC+LTR",
  "ATC+PIR",
  "ATC+BAT",
  "ATC+POWER",
  "ATC+FAN=0",
  "ATC+SOUND",
  "ATC+SEND"
};

// Corresponding LED bit patterns in BGR order
uint8_t ledPatterns[] = {
  0b001, // Blue
  0b010, // Green
  0b011, // Blue + Green
  0b100, // Red
  0b101, // Blue + Red
  0b110, // Green + Red
  0b111, // Blue + Green + Red
  0b001, // Blue
  0b010, // Green
  0b011, // Blue + Green
  0b100, // Red
  0b101, // Blue + Red
  0b110, // Green + Red
  0b111, // Blue + Green + Red
  0b001, // Blue
  0b010  // Green
};

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void sendATCommand(const char* cmd) {
    mySerial1.write(cmd);
    mySerial1.write("\n");
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.println("WebSocket client connected");
        client->text(dataHTML);
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.println("WebSocket client disconnected");
    } else if (type == WS_EVT_PONG) {
        Serial.println("Pong received");
    } else if (type == WS_EVT_ERROR) {
        Serial.printf("WebSocket error: %s\n", (char*)arg);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("ESP32 WebServer");

    // Connect to Wi-Fi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Set up GPIO pins and initialize Serial1
    pinMode(txPin, OUTPUT);
    pinMode(rxPin, INPUT);
    pinMode(EN_RAK, OUTPUT); 

    pinMode(LED_R, OUTPUT); // LED Red
    pinMode(LED_G, OUTPUT); // LED Green
    pinMode(LED_B, OUTPUT); // LED Blue

    digitalWrite(LED_R, HIGH); // turn the LED off (HIGH is the voltage level)
    digitalWrite(LED_G, HIGH); // turn the LED off (HIGH is the voltage level)
    digitalWrite(LED_B, HIGH); // turn the LED off (HIGH is the voltage level)
    delay(1000);

    digitalWrite(EN_RAK, HIGH); // Switch on RAK
    delay(1000);
    mySerial1.begin(115200, SERIAL_8N1, rxPin, txPin);
    // while (mySerial1.available()) {
    //     Serial.write(mySerial1.read()); // read it and send it out Serial (USB)
    // }
    // mySerial1.println("ATE");
    // delay(100);

    // while (mySerial1.available()) {
    //     Serial.write(mySerial1.read()); // read it and send it out Serial (USB)
    // }

    // Handle root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String html = "<!DOCTYPE html><html><head><title>ESP32 Data</title>";
        html += "<style>";
        html += "body { font-family: Arial, sans-serif; background-color: #f4f4f9; color: #333; }";
        html += "h1 { color: #0056b3; }";
        html += ".data { padding: 10px; margin: 10px 0; background: #fff; border-radius: 5px; box-shadow: 0 0 5px rgba(0,0,0,0.1); }";
        html += ".data p { margin: 0; }";
        html += "</style>";
        html += "<script>";
        html += "let socket = new WebSocket('ws://' + window.location.hostname + '/ws');";
        html += "socket.onmessage = function(event) {";
        html += "document.getElementById('data').innerHTML = event.data;";
        html += "};";
        html += "socket.onclose = function() {";
        html += "setTimeout(function() { location.reload(); }, 3000);";
        html += "};";
        html += "</script></head><body>";
        html += "<h1>ESP32 Data</h1><div id='data' class='data'>" + dataHTML + "</div></body></html>";
        request->send(200, "text/html", html);
    });

    // Handle 404 not found
    server.onNotFound(notFound);

    // Initialize WebSocket
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Start server
    server.begin();
}

void loop() {
  String response = "";
  // Loop through and send each AT command
  dataHTML = ""; // Clear previous data
  while (mySerial1.available()) {
    char c = mySerial1.read();
    Serial.write(c);
    response += c;
    Serial.write(mySerial1.read()); // read it and send it out Serial (USB)
  }
  if (response.isEmpty()) 
  {
    dataHTML = "<p>" + response + "</p>";
    ws.textAll(dataHTML);
  }
  static uint32_t pingTime = millis();
    // Send ping to keep connection alive
  if (millis() - pingTime > 15000) { // Send ping every 15 seconds
    ws.cleanupClients();
    pingTime = millis();
  }
    
  for (int i = 0; i < sizeof(atCommands) / sizeof(atCommands[0]); i++) 
  {
      // Update LED status
    digitalWrite(LED_B, ledPatterns[i] & 0x01 ? LOW : HIGH);
    digitalWrite(LED_G, ledPatterns[i] & 0x02 ? LOW : HIGH);
    digitalWrite(LED_R, ledPatterns[i] & 0x04 ? LOW : HIGH);
    // Send the AT command and get the response
    sendATCommand(atCommands[i]);
    if(i >= sizeof(atCommands) / sizeof(atCommands[0]) - 2) 
    {
    delay(6000);
    }
    else 
    {
      delay(100);
    }
    response = "";
    while (mySerial1.available()) {
      char c = mySerial1.read();
      Serial.write(c);
      response += c;
    }
    // Check available memory before appending to HTML string
    if (ESP.getFreeHeap() > 500) { // Ensure at least 500 bytes free
      // Replace "OK" with HTML span to change color
      if (response.indexOf("failed") != -1) {
        response = "<span style='color: #FF0000;'>" + response + "</span>";
        dataHTML += "<p><strong><span style='color: #FF0000;'>" + String(atCommands[i]) + "</span>:</strong> " + response + "</p>";
      } else {
        response.replace("OK", "<strong><span style='color: #2be72b;'>OK</span></strong>");
        dataHTML += "<p><strong>" + String(atCommands[i]) + ":</strong> " + response + "</p>";
      }
      ws.textAll(dataHTML); // Update all WebSocket clients with new data
      } else {
          Serial.println("Not enough memory to append data");
        }

      delay(3000);
    }
}
