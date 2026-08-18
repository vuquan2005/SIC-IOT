#ifndef SENSORS_H
#define SENSORS_H

#include "Module.h"
#include <DHT.h>

/**
 * @brief Module cảm biến nhiệt độ & độ ẩm DHT.
 */
class DhtSensor : public Sensor {
private:
  DHT* dht;
public:
  DhtSensor(String name, int pin);
  ~DhtSensor();
  virtual void begin() override;
  virtual void read(JsonDocument& doc) override;
};

/**
 * @brief Module cảm biến chuyển động hồng ngoại PIR.
 */
class PirSensor : public Sensor {
public:
  PirSensor(String name, int pin);
  virtual void begin() override;
  virtual void read(JsonDocument& doc) override;
};

/**
 * @brief Module cảm biến ánh sáng LDR.
 * Tích hợp bộ lọc trung bình trượt (Moving Average Filter) làm mịn dữ liệu.
 */
class LdrSensor : public Sensor {
private:
  static const int FILTER_SIZE = 5;
  int readings[FILTER_SIZE];
  int readIndex;
  int total;
public:
  LdrSensor(String name, int pin);
  virtual void begin() override;
  virtual void read(JsonDocument& doc) override;
};

/**
 * @brief Module cảm biến siêu âm đo khoảng cách.
 * Tích hợp chế độ timeout không chặn để tránh treo hệ thống khi cảm biến mất kết nối.
 */
class UltrasonicSensor : public Sensor {
private:
  int echoPin;
public:
  UltrasonicSensor(String name, int trigPin, int echoPin);
  virtual void begin() override;
  virtual void read(JsonDocument& doc) override;
};

#endif // SENSORS_H
