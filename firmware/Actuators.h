#ifndef ACTUATORS_H
#define ACTUATORS_H

#include "Module.h"
#include <LiquidCrystal_I2C.h>

/**
 * @brief Module Relay / Đèn.
 * Chỉ nhận lệnh từ server và phản hồi trạng thái hiện tại.
 */
class RelayActuator : public Actuator {
private:
  bool state;
public:
  RelayActuator(String name, int pin);
  virtual void begin() override;
  virtual void handleCommand(String payload) override;
  virtual String getState() override;
};

/**
 * @brief Module màn hình LCD I2C.
 * Lắng nghe và hiển thị nội dung tùy chọn do server MQTT chỉ định.
 */
class LcdModule : public Actuator {
private:
  LiquidCrystal_I2C* lcd;
  String line0;
  String line1;
public:
  LcdModule(String name, int addr);
  ~LcdModule();
  virtual void begin() override;
  virtual void handleCommand(String payload) override;
  virtual String getState() override;
  
  /**
   * @brief Hiển thị trạng thái/nội dung thủ công trên màn hình.
   * @param l0 Nội dung dòng đầu tiên.
   * @param l1 Nội dung dòng thứ hai.
   */
  void showStatus(String l0, String l1);
  
  /**
   * @brief Cập nhật thông tin lưu trong buffer hiển thị lên màn hình LCD.
   */
  void updateDisplay();
};

#endif // ACTUATORS_H
