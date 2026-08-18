#ifndef SMART_HOME_NODE_H
#define SMART_HOME_NODE_H

#include "Sensors.h"
#include "Actuators.h"
#include <PubSubClient.h>

/**
 * @brief Lớp điều phối trung tâm thiết bị (SmartHomeNode).
 * Quản lý vòng đời khởi tạo, luồng cập nhật cảm biến và chuyển hướng tin nhắn MQTT.
 */
class SmartHomeNode {
private:
  static const int MAX_SENSORS = 8;
  static const int MAX_ACTUATORS = 8;
  
  Sensor* sensors[MAX_SENSORS];
  int sensorCount;
  
  Actuator* actuators[MAX_ACTUATORS];
  int actuatorCount;
  
  LcdModule* lcdMod;
  PubSubClient* mqttClient;
public:
  SmartHomeNode();
  ~SmartHomeNode();
  
  /**
   * @brief Gán đối tượng MQTT Client điều hướng.
   * @param client Con trỏ đối tượng PubSubClient toàn cục.
   */
  void setMqttClient(PubSubClient* client) { mqttClient = client; }
  
  /**
   * @brief Lấy đối tượng MQTT Client.
   * @return Con trỏ tới PubSubClient.
   */
  PubSubClient* getMqttClient() { return mqttClient; }
  
  /**
   * @brief Đăng ký thêm cảm biến vào hệ thống.
   * @param s Con trỏ đối tượng Sensor.
   */
  void registerSensor(Sensor* s);
  
  /**
   * @brief Đăng ký thêm actuator vào hệ thống.
   * @param a Con trỏ đối tượng Actuator.
   */
  void registerActuator(Actuator* a);
  
  /**
   * @brief Gán module màn hình LCD chính của node.
   * @param l Con trỏ LCD Module.
   */
  void setLcd(LcdModule* l);
  
  /**
   * @brief Lấy module màn hình LCD của node.
   * @return Con trỏ LcdModule.
   */
  LcdModule* getLcd();
  
  int getSensorCount() const { return sensorCount; }
  int getActuatorCount() const { return actuatorCount; }
  
  /**
   * @brief Lấy cơ cấu chấp hành tại vị trí lưu trữ.
   * @param index Vị trí trong mảng.
   * @return Con trỏ Actuator tương ứng.
   */
  Actuator* getActuator(int index);
  
  /**
   * @brief Gọi hàm khởi chạy phần cứng ban đầu của tất cả các module đã đăng ký.
   */
  void beginModules();
  
  /**
   * @brief Đọc thông tin từ tất cả các cảm biến và lưu vào JSON payload.
   * @param doc Tham chiếu tới JsonDocument.
   */
  void readAllSensors(JsonDocument& doc);
  
  /**
   * @brief Xử lý và định tuyến tin nhắn lệnh nhận được từ MQTT Broker.
   * @param topic Topic MQTT nhận lệnh.
   * @param payload Nội dung lệnh.
   */
  void handleMqttMessage(String topic, String payload);
};

extern SmartHomeNode node;

#endif // SMART_HOME_NODE_H
