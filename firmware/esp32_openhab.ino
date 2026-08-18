#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <Preferences.h>

#include "Config.h"
#include "SmartHomeNode.h"

WiFiClient espClient;
PubSubClient client(espClient);

/**
 * @brief Hàm nhận dữ liệu (MQTT Callback) được đăng ký với MQTT Client.
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.printf("MQTT nhan [%s]: %s\n", topic, message.c_str());
  node.handleMqttMessage(String(topic), message);
}

/**
 * @brief Thực hiện việc kết nối lại MQTT Broker khi mất kết nối.
 * @return true nếu kết nối lại thành công, ngược lại là false.
 */
bool connectMQTT() {
  Serial.printf("Dang ket noi MQTT Broker: %s...\n", config.mqtt_server);
  
  String clientId = "ESP32Client-" + WiFi.macAddress();
  clientId.replace(":", "");
  
  if (client.connect(clientId.c_str())) {
    Serial.println("MQTT connected successfully!");
    
    for (int i = 0; i < node.getActuatorCount(); i++) {
      Actuator* act = node.getActuator(i);
      String subTopic;
      act->getSubscribeTopic(subTopic, config.room_name);
      client.subscribe(subTopic.c_str());
      
      String stateTopic;
      act->getStateTopic(stateTopic, config.room_name);
      client.publish(stateTopic.c_str(), act->getState().c_str(), true);
      
      Serial.printf("Subscribed to: %s\n", subTopic.c_str());
    }
    return true;
  } else {
    Serial.printf("Loi ket noi MQTT, rc=%d. Thu lai sau.\n", client.state());
    return false;
  }
}

unsigned long lastMqttRetry = 0;
const unsigned long mqttRetryInterval = 5000;

/**
 * @brief Điều phối kết nối MQTT bằng cơ chế non-blocking (không chặn luồng).
 */
void handleMQTTConnection() {
  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastMqttRetry >= mqttRetryInterval) {
      lastMqttRetry = now;
      connectMQTT();
    }
  } else {
    client.loop();
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  
  loadConfiguration();
  setupWiFiManager();
  
  if (config.pin_dht != -1) {
    node.registerSensor(new DhtSensor("dht", config.pin_dht));
  }
  if (config.pin_pir != -1) {
    node.registerSensor(new PirSensor("pir", config.pin_pir));
  }
  if (config.pin_ldr != -1) {
    node.registerSensor(new LdrSensor("ldr", config.pin_ldr));
  }
  if (config.pin_trig != -1 && config.pin_echo != -1) {
    node.registerSensor(new UltrasonicSensor("ultrasonic", config.pin_trig, config.pin_echo));
  }
  
  if (config.pin_led1 != -1) {
    node.registerActuator(new RelayActuator("light1", config.pin_led1));
  }
  if (config.pin_led2 != -1) {
    node.registerActuator(new RelayActuator("light2", config.pin_led2));
  }
  if (config.pin_door != -1) {
    node.registerActuator(new RelayActuator("door", config.pin_door));
  }
  
  if (config.lcd_addr != 0) {
    LcdModule* l = new LcdModule("lcd", config.lcd_addr);
    node.registerActuator(l);
    node.setLcd(l);
  }
  
  node.setMqttClient(&client);
  node.beginModules();
  
  client.setServer(config.mqtt_server, config.mqtt_port);
  client.setCallback(mqttCallback);
  
  connectMQTT();
}

unsigned long lastSensorRead = 0;
const unsigned long sensorReadInterval = 3000;

void loop() {
  checkResetConfigButton();
  handleMQTTConnection();
  
  unsigned long now = millis();
  if (now - lastSensorRead >= sensorReadInterval) {
    lastSensorRead = now;
    
    if (node.getSensorCount() > 0) {
      StaticJsonDocument<300> doc;
      node.readAllSensors(doc);
      
      if (client.connected()) {
        char buffer[300];
        serializeJson(doc, buffer);
        
        String pubTopic = String("home/") + config.room_name + "/sensors";
        client.publish(pubTopic.c_str(), buffer);
        
        Serial.printf("MQTT publish [%s]: %s\n", pubTopic.c_str(), buffer);
      }
      
      if (node.getLcd() != nullptr) {
        float temp = doc["dht_temp"] | -999.0;
        float humi = doc["dht_humi"] | -999.0;
        if (temp != -999.0 && humi != -999.0) {
          char displayStr[17];
          snprintf(displayStr, sizeof(displayStr), "T:%dC H:%d%%", (int)temp, (int)humi);
          node.getLcd()->showStatus("Node: " + String(config.room_name), displayStr);
        } else {
          node.getLcd()->showStatus("Node: " + String(config.room_name), "Running...");
        }
      }
    }
  }
}
