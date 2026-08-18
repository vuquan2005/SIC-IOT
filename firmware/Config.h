#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

/**
 * @brief Struct cấu hình toàn cục lưu trữ trong flash NVS.
 */
struct DeviceConfig {
  char mqtt_server[40];
  int mqtt_port;
  char room_name[30];
  
  int pin_dht;
  int pin_pir;
  int pin_trig;
  int pin_echo;
  int pin_led1;
  int pin_led2;
  int pin_door;
  int pin_ldr;
  
  int lcd_addr;
};

extern DeviceConfig config;
extern const int TRIGGER_PIN;

/**
 * @brief Tải cấu hình từ bộ nhớ Flash Preferences.
 */
void loadConfiguration();

/**
 * @brief Lưu cấu hình hiện tại vào bộ nhớ Flash Preferences.
 */
void saveConfiguration();

/**
 * @brief Thiết lập WiFiManager và hiển thị cổng cấu hình WiFi/MQTT khi cần.
 */
void setupWiFiManager();

/**
 * @brief Kiểm tra nút nhấn reset (BOOT pin) để khôi phục cấu hình mặc định.
 */
void checkResetConfigButton();

#endif // CONFIG_H
