#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

// --- 1. THÔNG SỐ CẤU HÌNH MẠNG ---
const char* ssid = "Ke Coffee 3";            // Tên Wi-Fi
const char* password = "kecoffee68";         // Mật khẩu Wi-Fi
const char* mqtt_server = "192.168.100.142"; // Địa chỉ IP của Raspberry Pi
const int mqtt_port = 1883;

// --- 2. CẤU HÌNH CHÂN NỐI (GPIO) ---
#define DHTPIN 4
#define DHTTYPE DHT22

#define PIR_PIN 14
#define TRIG_PIN 5
#define ECHO_PIN 18

#define LED1_PIN 13          // Đèn phòng khách
#define LED2_PIN 12          // Đèn phòng ngủ
#define DOOR_PIN 25          // Cấp nguồn điều khiển Cửa trực tiếp từ GPIO 25

// --- 3. KHỞI TẠO ĐỐI TƯỢNG ---
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;

// --- 4. HÀM ĐO KHOẢNG CÁCH SIÊU ÂM ---
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

// --- 5. HÀM XỬ LÝ KHI NHẬN LỆNH TỪ OPENHAB ---
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Nhan lenh tu topic [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // Lệnh điều khiển Đèn 1 (Phòng khách)
  if (String(topic) == "home/esp32/relay_light") {
    if (message == "ON") {
      digitalWrite(LED1_PIN, HIGH);
      lcd.setCursor(0, 1);
      lcd.print("LED1: ON        ");
    } else if (message == "OFF") {
      digitalWrite(LED1_PIN, LOW);
      lcd.setCursor(0, 1);
      lcd.print("LED1: OFF       ");
    }
  }

  // Lệnh điều khiển Đèn 2 (Phòng ngủ)
  if (String(topic) == "home/esp32/relay_light_bedroom") {
    if (message == "ON") {
      digitalWrite(LED2_PIN, HIGH);
      lcd.setCursor(0, 1);
      lcd.print("LED2: ON        ");
    } else if (message == "OFF") {
      digitalWrite(LED2_PIN, LOW);
      lcd.setCursor(0, 1);
      lcd.print("LED2: OFF       ");
    }
  }

  // Lệnh điều khiển Cửa (Cấp nguồn trực tiếp từ GPIO 25)
  if (String(topic) == "home/door") {
    if (message == "OPEN") {
      digitalWrite(DOOR_PIN, HIGH); // Bật nguồn mở cửa
      lcd.setCursor(0, 1);
      lcd.print("DOOR: OPENED    ");
    } else if (message == "CLOSE") {
      digitalWrite(DOOR_PIN, LOW);  // Tắt nguồn đóng cửa
      lcd.setCursor(0, 1);
      lcd.print("DOOR: CLOSED    ");
    }
  }
}

// --- 6. KẾT NỐI WIFI ---
void setup_wifi() {
  delay(10);
  Serial.print("Dang ket noi WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi da ket noi!");
  Serial.print("IP ESP32: ");
  Serial.println(WiFi.localIP());
}

// --- 7. KẾT NỐI MQTT BROKER ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("Dang ket noi MQTT Broker...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("Thanh cong!");
      client.subscribe("home/esp32/relay_light");          // Topic Đèn phòng khách
      client.subscribe("home/esp32/relay_light_bedroom");  // Topic Đèn phòng ngủ
      client.subscribe("home/door");                       // Topic Cửa
    } else {
      Serial.print("Loi, rc=");
      Serial.print(client.state());
      Serial.println(" Thu lai sau 5 giay");
      delay(5000);
    }
  }
}

// --- 8. HÀM KHỞI TẠO (SETUP) ---
void setup() {
  Serial.begin(115200);

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(DOOR_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Mặc định ban đầu tắt hết thiết bị
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(DOOR_PIN, LOW);

  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Home");

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// --- 9. VÒNG LẶP CHÍNH (LOOP) ---
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Đọc và gửi dữ liệu cảm biến mỗi 3 giây
  unsigned long now = millis();
  if (now - lastMsg > 3000) {
    lastMsg = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int pir = digitalRead(PIR_PIN);
    float dist = readDistance();

    if (isnan(h) || isnan(t)) {
      h = 0;
      t = 0;
    }

    // Đóng gói JSON gửi lên openHAB
    StaticJsonDocument<200> doc;
    doc["temp"] = t;
    doc["humi"] = h;
    doc["pir"] = pir;
    doc["distance"] = dist;

    char buffer[256];
    serializeJson(doc, buffer);

    client.publish("home/esp32/sensors", buffer);
    Serial.print("Gui du lieu: ");
    Serial.println(buffer);

    // Hiển thị nhiệt độ / độ ẩm lên dòng 1 LCD
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print((int)t);
    lcd.print("C H:");
    lcd.print((int)h);
    lcd.print("%    ");
  }
}