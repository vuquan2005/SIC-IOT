#ifndef MODULE_H
#define MODULE_H

#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * @brief Lớp cơ sở đại diện cho một module bất kỳ (Base Module).
 */
class Module {
protected:
  String name;
  int pin;
public:
  Module(String name, int pin) : name(name), pin(pin) {}
  virtual ~Module() {}
  virtual void begin() = 0;
  String getName() const { return name; }
  int getPin() const { return pin; }
};

/**
 * @brief Lớp trừu tượng đại diện cho các loại Cảm biến (Sensor).
 * Cảm biến chỉ có nhiệm vụ đọc dữ liệu vật lý và lọc nếu cần, sau đó lưu vào JSON.
 */
class Sensor : public Module {
public:
  Sensor(String name, int pin) : Module(name, pin) {}
  virtual void read(JsonDocument& doc) = 0;
};

/**
 * @brief Lớp trừu tượng đại diện cho các Cơ cấu chấp hành (Actuator).
 * Cơ cấu chấp hành chỉ chấp hành lệnh từ server gửi xuống và phản hồi trạng thái.
 */
class Actuator : public Module {
public:
  Actuator(String name, int pin) : Module(name, pin) {}
  virtual void handleCommand(String payload) = 0;
  virtual String getState() = 0;
  
  /**
   * @brief Lấy topic nhận lệnh điều khiển.
   * @param topic Chuỗi lưu topic kết quả.
   * @param roomName Tên phòng của thiết bị.
   */
  void getSubscribeTopic(String& topic, const String& roomName) {
    topic = "home/" + roomName + "/" + name + "/set";
  }
  
  /**
   * @brief Lấy topic gửi trả trạng thái hiện tại.
   * @param topic Chuỗi lưu topic kết quả.
   * @param roomName Tên phòng của thiết bị.
   */
  void getStateTopic(String& topic, const String& roomName) {
    topic = "home/" + roomName + "/" + name + "/state";
  }
};

#endif // MODULE_H
