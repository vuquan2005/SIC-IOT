#include "SmartHomeNode.h"
#include "Config.h"

SmartHomeNode node;

SmartHomeNode::SmartHomeNode() : sensorCount(0), actuatorCount(0), lcdMod(nullptr), mqttClient(nullptr) {}

SmartHomeNode::~SmartHomeNode() {
  for (int i = 0; i < sensorCount; i++) delete sensors[i];
  for (int i = 0; i < actuatorCount; i++) delete actuators[i];
}

void SmartHomeNode::registerSensor(Sensor* s) {
  if (sensorCount < MAX_SENSORS) {
    sensors[sensorCount++] = s;
  }
}

void SmartHomeNode::registerActuator(Actuator* a) {
  if (actuatorCount < MAX_ACTUATORS) {
    actuators[actuatorCount++] = a;
  }
}

void SmartHomeNode::setLcd(LcdModule* l) {
  lcdMod = l;
}

LcdModule* SmartHomeNode::getLcd() {
  return lcdMod;
}

Actuator* SmartHomeNode::getActuator(int index) {
  if (index >= 0 && index < actuatorCount) return actuators[index];
  return nullptr;
}

void SmartHomeNode::beginModules() {
  for (int i = 0; i < sensorCount; i++) {
    sensors[i]->begin();
  }
  for (int i = 0; i < actuatorCount; i++) {
    actuators[i]->begin();
  }
}

void SmartHomeNode::readAllSensors(JsonDocument& doc) {
  for (int i = 0; i < sensorCount; i++) {
    sensors[i]->read(doc);
  }
}

void SmartHomeNode::handleMqttMessage(String topic, String payload) {
  if (mqttClient == nullptr) return;
  
  for (int i = 0; i < actuatorCount; i++) {
    String subTopic;
    actuators[i]->getSubscribeTopic(subTopic, config.room_name);
    if (topic == subTopic) {
      actuators[i]->handleCommand(payload);
      
      String stateTopic;
      actuators[i]->getStateTopic(stateTopic, config.room_name);
      mqttClient->publish(stateTopic.c_str(), actuators[i]->getState().c_str(), true);
      
      if (lcdMod != nullptr && actuators[i]->getName() != "lcd") {
        lcdMod->showStatus("Node: " + String(config.room_name), 
                           actuators[i]->getName() + ": " + actuators[i]->getState());
      }
      break;
    }
  }
}
