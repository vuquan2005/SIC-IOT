#include "Actuators.h"

RelayActuator::RelayActuator(String name, int pin) : Actuator(name, pin), state(false) {}

void RelayActuator::begin() {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  state = false;
}

void RelayActuator::handleCommand(String payload) {
  if (payload == "ON") {
    digitalWrite(pin, HIGH);
    state = true;
  } else if (payload == "OFF") {
    digitalWrite(pin, LOW);
    state = false;
  }
}

String RelayActuator::getState() {
  return state ? "ON" : "OFF";
}

LcdModule::LcdModule(String name, int addr) : Actuator(name, addr), lcd(nullptr), line0(""), line1("") {}

LcdModule::~LcdModule() {
  delete lcd;
}

void LcdModule::begin() {
  lcd = new LiquidCrystal_I2C(pin, 16, 2);
  lcd->init();
  lcd->backlight();
  lcd->clear();
  lcd->print("SmartHome Node");
}

void LcdModule::handleCommand(String payload) {
  int newlineIdx = payload.indexOf('\n');
  if (newlineIdx != -1) {
    line0 = payload.substring(0, newlineIdx);
    line1 = payload.substring(newlineIdx + 1);
  } else {
    line0 = payload;
    line1 = "";
  }
  updateDisplay();
}

void LcdModule::showStatus(String l0, String l1) {
  line0 = l0;
  line1 = l1;
  updateDisplay();
}

void LcdModule::updateDisplay() {
  if (lcd == nullptr) return;
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print(line0.substring(0, 16));
  lcd->setCursor(0, 1);
  lcd->print(line1.substring(0, 16));
}

String LcdModule::getState() {
  return line0 + " | " + line1;
}
