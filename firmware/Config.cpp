#include "Config.h"
#include <Preferences.h>
#include <WiFiManager.h>
#include <WiFi.h>

DeviceConfig config;
Preferences preferences;
const int TRIGGER_PIN = 0;
static bool shouldSaveConfig = false;

/**
 * @brief Callback được gọi khi người dùng lưu cấu hình từ Web Portal.
 */
static void saveConfigCallback() {
  Serial.println("Nhan tin hieu luu cau hinh tu Web Portal...");
  shouldSaveConfig = true;
}

void loadConfiguration() {
  preferences.begin("smarthome", true);
  
  preferences.getString("mqtt_server", config.mqtt_server, sizeof(config.mqtt_server), "192.168.100.142");
  config.mqtt_port = preferences.getInt("mqtt_port", 1883);
  preferences.getString("room_name", config.room_name, sizeof(config.room_name), "living_room");
  
  config.pin_dht = preferences.getInt("pin_dht", 4);
  config.pin_pir = preferences.getInt("pin_pir", 14);
  config.pin_trig = preferences.getInt("pin_trig", -1);
  config.pin_echo = preferences.getInt("pin_echo", -1);
  config.pin_led1 = preferences.getInt("pin_led1", 13);
  config.pin_led2 = preferences.getInt("pin_led2", 12);
  config.pin_door = preferences.getInt("pin_door", 25);
  config.pin_ldr = preferences.getInt("pin_ldr", 34);
  config.lcd_addr = preferences.getInt("lcd_addr", 0x27);
  
  preferences.end();
}

void saveConfiguration() {
  preferences.begin("smarthome", false);
  
  preferences.putString("mqtt_server", config.mqtt_server);
  preferences.putInt("mqtt_port", config.mqtt_port);
  preferences.putString("room_name", config.room_name);
  
  preferences.putInt("pin_dht", config.pin_dht);
  preferences.putInt("pin_pir", config.pin_pir);
  preferences.putInt("pin_trig", config.pin_trig);
  preferences.putInt("pin_echo", config.pin_echo);
  preferences.putInt("pin_led1", config.pin_led1);
  preferences.putInt("pin_led2", config.pin_led2);
  preferences.putInt("pin_door", config.pin_door);
  preferences.putInt("pin_ldr", config.pin_ldr);
  preferences.putInt("lcd_addr", config.lcd_addr);
  
  preferences.end();
  Serial.println("-> Da luu cau hinh moi vao Flash.");
}

void setupWiFiManager() {
  WiFiManager wm;
  wm.setSaveConfigCallback(saveConfigCallback);
  
  char portStr[6], dhtStr[4], pirStr[4], trigStr[4], echoStr[4];
  char led1Str[4], led2Str[4], doorStr[4], ldrStr[4], lcdStr[6];
  
  itoa(config.mqtt_port, portStr, 10);
  itoa(config.pin_dht, dhtStr, 10);
  itoa(config.pin_pir, pirStr, 10);
  itoa(config.pin_trig, trigStr, 10);
  itoa(config.pin_echo, echoStr, 10);
  itoa(config.pin_led1, led1Str, 10);
  itoa(config.pin_led2, led2Str, 10);
  itoa(config.pin_door, doorStr, 10);
  itoa(config.pin_ldr, ldrStr, 10);
  sprintf(lcdStr, "0x%02X", config.lcd_addr);
  
  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server IP", config.mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", portStr, 6);
  WiFiManagerParameter custom_room_name("room", "Room Name / Topic base", config.room_name, 30);
  
  WiFiManagerParameter custom_pin_dht("pin_dht", "DHT Pin (-1 to disable)", dhtStr, 4);
  WiFiManagerParameter custom_pin_pir("pin_pir", "PIR Pin (-1 to disable)", pirStr, 4);
  WiFiManagerParameter custom_pin_trig("pin_trig", "Ultrasonic Trig (-1 to disable)", trigStr, 4);
  WiFiManagerParameter custom_pin_echo("pin_echo", "Ultrasonic Echo (-1 to disable)", echoStr, 4);
  WiFiManagerParameter custom_pin_led1("pin_led1", "Relay Light 1 (-1 to disable)", led1Str, 4);
  WiFiManagerParameter custom_pin_led2("pin_led2", "Relay Light 2 (-1 to disable)", led2Str, 4);
  WiFiManagerParameter custom_pin_door("pin_door", "Relay Door (-1 to disable)", doorStr, 4);
  WiFiManagerParameter custom_pin_ldr("pin_ldr", "LDR Pin (-1 to disable)", ldrStr, 4);
  WiFiManagerParameter custom_lcd_addr("lcd_addr", "LCD Address (e.g. 0x27, 0 to disable)", lcdStr, 6);

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_room_name);
  wm.addParameter(&custom_pin_dht);
  wm.addParameter(&custom_pin_pir);
  wm.addParameter(&custom_pin_trig);
  wm.addParameter(&custom_pin_echo);
  wm.addParameter(&custom_pin_led1);
  wm.addParameter(&custom_pin_led2);
  wm.addParameter(&custom_pin_door);
  wm.addParameter(&custom_pin_ldr);
  wm.addParameter(&custom_lcd_addr);

  String portalSsid = "ESP32_SmartHome_" + WiFi.macAddress().substring(12);
  portalSsid.replace(":", "");
  
  wm.setConfigPortalTimeout(180);
  if (!wm.autoConnect(portalSsid.c_str(), "admin123")) {
    Serial.println("Ket noi WiFi that bai. Khoi dong lai...");
    delay(3000);
    ESP.restart();
  }

  if (shouldSaveConfig) {
    strcpy(config.mqtt_server, custom_mqtt_server.getValue());
    config.mqtt_port = atoi(custom_mqtt_port.getValue());
    strcpy(config.room_name, custom_room_name.getValue());
    
    config.pin_dht = atoi(custom_pin_dht.getValue());
    config.pin_pir = atoi(custom_pin_pir.getValue());
    config.pin_trig = atoi(custom_pin_trig.getValue());
    config.pin_echo = atoi(custom_pin_echo.getValue());
    config.pin_led1 = atoi(custom_pin_led1.getValue());
    config.pin_led2 = atoi(custom_pin_led2.getValue());
    config.pin_door = atoi(custom_pin_door.getValue());
    config.pin_ldr = atoi(custom_pin_ldr.getValue());
    
    String addrStr = custom_lcd_addr.getValue();
    if (addrStr.startsWith("0x") || addrStr.startsWith("0X")) {
      config.lcd_addr = strtol(addrStr.c_str(), NULL, 16);
    } else {
      config.lcd_addr = atoi(addrStr.c_str());
    }
    
    saveConfiguration();
  }
}

void checkResetConfigButton() {
  if (digitalRead(TRIGGER_PIN) == LOW) {
    delay(50);
    if (digitalRead(TRIGGER_PIN) == LOW) {
      Serial.println("Phat hien nhan nut. Giu 5 giay de RESET toan bo cau hinh...");
      unsigned long pressStart = millis();
      bool held = true;
      
      while (millis() - pressStart < 5000) {
        if (config.pin_led1 != -1) {
          digitalWrite(config.pin_led1, (millis() / 100) % 2);
        }
        if (digitalRead(TRIGGER_PIN) == HIGH) {
          held = false;
          break;
        }
        delay(10);
      }
      
      if (held) {
        WiFiManager wm;
        wm.resetSettings();
        
        preferences.begin("smarthome", false);
        preferences.clear();
        preferences.end();
        
        Serial.println("Hoan tat! Khoi dong lai...");
        delay(1000);
        ESP.restart();
      } else {
        if (config.pin_led1 != -1) {
          digitalWrite(config.pin_led1, LOW);
        }
        Serial.println("Da huy lenh reset.");
      }
    }
  }
}
