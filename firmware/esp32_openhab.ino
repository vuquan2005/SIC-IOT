```cpp
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

// =====================================================
// 1. THÔNG SỐ CẤU HÌNH MẠNG
// =====================================================

const char* ssid = "Ke Coffee 3";
const char* password = "kecoffee68";

const char* mqtt_server = "192.168.100.142";
const int mqtt_port = 1883;


// =====================================================
// 2. CẤU HÌNH CHÂN GPIO
// =====================================================

#define DHTPIN 4
#define DHTTYPE DHT22

#define PIR_PIN 14

#define TRIG_PIN 5
#define ECHO_PIN 18

#define LED1_PIN 13          // Đèn phòng khách
#define LED2_PIN 12          // Đèn phòng ngủ

#define DOOR_PIN 25          // Cửa

// -------- CẢM BIẾN ÁNH SÁNG --------

#define LDR_LIVING_PIN 34    // LDR phòng khách
#define LDR_BEDROOM_PIN 35   // LDR phòng ngủ


// =====================================================
// 3. NGƯỠNG ÁNH SÁNG
// =====================================================

// Nếu LDR < DARK_THRESHOLD:
// → Phòng tối → Bật đèn

// Nếu LDR > BRIGHT_THRESHOLD:
// → Phòng sáng → Tắt đèn

#define DARK_THRESHOLD 1200
#define BRIGHT_THRESHOLD 1600


// =====================================================
// 4. KHỞI TẠO ĐỐI TƯỢNG
// =====================================================

DHT dht(DHTPIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClient espClient;

PubSubClient client(espClient);

unsigned long lastMsg = 0;


// =====================================================
// 5. HÀM ĐO KHOẢNG CÁCH
// =====================================================

float readDistance() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);

  float distance = duration * 0.034 / 2;

  return distance;
}


// =====================================================
// 6. ĐIỀU KHIỂN ĐÈN TỰ ĐỘNG THEO ÁNH SÁNG
// =====================================================

void autoLightControl(int livingLight, int bedroomLight) {

  // -------------------------------------------------
  // PHÒNG KHÁCH
  // -------------------------------------------------

  if (livingLight < DARK_THRESHOLD) {

    // Phòng khách tối
    digitalWrite(LED1_PIN, HIGH);

    Serial.println("Phong khach TOI -> Bat den");

  }

  else if (livingLight > BRIGHT_THRESHOLD) {

    // Phòng khách sáng
    digitalWrite(LED1_PIN, LOW);

    Serial.println("Phong khach SANG -> Tat den");
  }


  // -------------------------------------------------
  // PHÒNG NGỦ
  // -------------------------------------------------

  if (bedroomLight < DARK_THRESHOLD) {

    // Phòng ngủ tối
    digitalWrite(LED2_PIN, HIGH);

    Serial.println("Phong ngu TOI -> Bat den");

  }

  else if (bedroomLight > BRIGHT_THRESHOLD) {

    // Phòng ngủ sáng
    digitalWrite(LED2_PIN, LOW);

    Serial.println("Phong ngu SANG -> Tat den");
  }
}


// =====================================================
// 7. XỬ LÝ LỆNH TỪ OPENHAB
// =====================================================

void callback(char* topic, byte* payload, unsigned int length) {

  String message = "";

  for (unsigned int i = 0; i < length; i++) {

    message += (char)payload[i];
  }

  Serial.print("Nhan lenh tu topic [");
  Serial.print(topic);
  Serial.print("]: ");

  Serial.println(message);


  // =================================================
  // ĐÈN PHÒNG KHÁCH
  // =================================================

  if (String(topic) == "home/esp32/relay_light") {

    if (message == "ON") {

      digitalWrite(LED1_PIN, HIGH);

      lcd.setCursor(0, 1);
      lcd.print("LED1: ON        ");
    }

    else if (message == "OFF") {

      digitalWrite(LED1_PIN, LOW);

      lcd.setCursor(0, 1);
      lcd.print("LED1: OFF       ");
    }
  }


  // =================================================
  // ĐÈN PHÒNG NGỦ
  // =================================================

  if (String(topic) == "home/esp32/relay_light_bedroom") {

    if (message == "ON") {

      digitalWrite(LED2_PIN, HIGH);

      lcd.setCursor(0, 1);
      lcd.print("LED2: ON        ");
    }

    else if (message == "OFF") {

      digitalWrite(LED2_PIN, LOW);

      lcd.setCursor(0, 1);
      lcd.print("LED2: OFF       ");
    }
  }


  // =================================================
  // CỬA
  // =================================================

  if (String(topic) == "home/door") {

    if (message == "OPEN") {

      digitalWrite(DOOR_PIN, HIGH);

      lcd.setCursor(0, 1);
      lcd.print("DOOR: OPENED    ");
    }

    else if (message == "CLOSE") {

      digitalWrite(DOOR_PIN, LOW);

      lcd.setCursor(0, 1);
      lcd.print("DOOR: CLOSED    ");
    }
  }
}


// =====================================================
// 8. KẾT NỐI WIFI
// =====================================================

void setup_wifi() {

  delay(10);

  Serial.print("Dang ket noi WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.println("WiFi da ket noi!");

  Serial.print("IP ESP32: ");
  Serial.println(WiFi.localIP());
}


// =====================================================
// 9. KẾT NỐI MQTT
// =====================================================

void reconnect() {

  while (!client.connected()) {

    Serial.print("Dang ket noi MQTT Broker...");

    String clientId = "ESP32Client-";

    clientId += String(random(0xffff), HEX);


    if (client.connect(clientId.c_str())) {

      Serial.println("Thanh cong!");


      // Đèn phòng khách
      client.subscribe("home/esp32/relay_light");


      // Đèn phòng ngủ
      client.subscribe("home/esp32/relay_light_bedroom");


      // Cửa
      client.subscribe("home/door");
    }

    else {

      Serial.print("Loi, rc=");
      Serial.print(client.state());

      Serial.println(" Thu lai sau 5 giay");

      delay(5000);
    }
  }
}


// =====================================================
// 10. SETUP
// =====================================================

void setup() {

  Serial.begin(115200);


  // -----------------------------------------------
  // OUTPUT
  // -----------------------------------------------

  pinMode(LED1_PIN, OUTPUT);

  pinMode(LED2_PIN, OUTPUT);

  pinMode(DOOR_PIN, OUTPUT);


  // -----------------------------------------------
  // INPUT
  // -----------------------------------------------

  pinMode(PIR_PIN, INPUT);

  pinMode(TRIG_PIN, OUTPUT);

  pinMode(ECHO_PIN, INPUT);


  // LDR
  pinMode(LDR_LIVING_PIN, INPUT);

  pinMode(LDR_BEDROOM_PIN, INPUT);


  // -----------------------------------------------
  // TRẠNG THÁI BAN ĐẦU
  // -----------------------------------------------

  digitalWrite(LED1_PIN, LOW);

  digitalWrite(LED2_PIN, LOW);

  digitalWrite(DOOR_PIN, LOW);


  // -----------------------------------------------
  // DHT
  // -----------------------------------------------

  dht.begin();


  // -----------------------------------------------
  // LCD
  // -----------------------------------------------

  lcd.init();

  lcd.backlight();

  lcd.setCursor(0, 0);

  lcd.print("Smart Home");


  // -----------------------------------------------
  // WIFI
  // -----------------------------------------------

  setup_wifi();


  // -----------------------------------------------
  // MQTT
  // -----------------------------------------------

  client.setServer(mqtt_server, mqtt_port);

  client.setCallback(callback);
}


// =====================================================
// 11. LOOP
// =====================================================

void loop() {

  // -----------------------------------------------
  // KIỂM TRA MQTT
  // -----------------------------------------------

  if (!client.connected()) {

    reconnect();
  }

  client.loop();


  // -----------------------------------------------
  // ĐỌC CẢM BIẾN MỖI 3 GIÂY
  // -----------------------------------------------

  unsigned long now = millis();


  if (now - lastMsg > 3000) {

    lastMsg = now;


    // =================================================
    // ĐỌC DHT
    // =================================================

    float h = dht.readHumidity();

    float t = dht.readTemperature();


    // =================================================
    // ĐỌC PIR
    // =================================================

    int pir = digitalRead(PIR_PIN);


    // =================================================
    // ĐỌC SIÊU ÂM
    // =================================================

    float dist = readDistance();


    // =================================================
    // ĐỌC CẢM BIẾN ÁNH SÁNG
    // =================================================

    int livingLight = analogRead(LDR_LIVING_PIN);

    int bedroomLight = analogRead(LDR_BEDROOM_PIN);


    // =================================================
    // KIỂM TRA DHT
    // =================================================

    if (isnan(h) || isnan(t)) {

      h = 0;

      t = 0;
    }


    // =================================================
    // ĐIỀU KHIỂN ĐÈN TỰ ĐỘNG
    // =================================================

    autoLightControl(livingLight, bedroomLight);


    // =================================================
    // ĐÓNG GÓI JSON
    // =================================================

    StaticJsonDocument<300> doc;

    doc["temp"] = t;

    doc["humi"] = h;

    doc["pir"] = pir;

    doc["distance"] = dist;

    doc["living_light"] = livingLight;

    doc["bedroom_light"] = bedroomLight;


    char buffer[350];

    serializeJson(doc, buffer);


    // =================================================
    // GỬI MQTT
    // =================================================

    client.publish(
      "home/esp32/sensors",
      buffer
    );


    Serial.print("Gui du lieu: ");

    Serial.println(buffer);


    // =================================================
    // HIỂN THỊ LCD
    // =================================================

    lcd.setCursor(0, 0);

    lcd.print("T:");

    lcd.print((int)t);

    lcd.print("C H:");

    lcd.print((int)h);

    lcd.print("%    ");
  }
}
```
