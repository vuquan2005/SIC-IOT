#include "Sensors.h"

DhtSensor::DhtSensor(String name, int pin) : Sensor(name, pin), dht(nullptr) {}

DhtSensor::~DhtSensor() {
  delete dht;
}

void DhtSensor::begin() {
  dht = new DHT(pin, DHT22);
  dht->begin();
}

void DhtSensor::read(JsonDocument& doc) {
  if (dht == nullptr) return;
  float t = dht->readTemperature();
  float h = dht->readHumidity();
  if (!isnan(t) && !isnan(h)) {
    doc[name + "_temp"] = t;
    doc[name + "_humi"] = h;
  } else {
    doc[name + "_temp"] = 0.0;
    doc[name + "_humi"] = 0.0;
  }
}

PirSensor::PirSensor(String name, int pin) : Sensor(name, pin) {}

void PirSensor::begin() {
  pinMode(pin, INPUT);
}

void PirSensor::read(JsonDocument& doc) {
  doc[name] = digitalRead(pin);
}

LdrSensor::LdrSensor(String name, int pin) : Sensor(name, pin), readIndex(0), total(0) {
  memset(readings, 0, sizeof(readings));
}

void LdrSensor::begin() {
  pinMode(pin, INPUT);
  int raw = analogRead(pin);
  for (int i = 0; i < FILTER_SIZE; i++) {
    readings[i] = raw;
  }
  total = raw * FILTER_SIZE;
}

void LdrSensor::read(JsonDocument& doc) {
  int raw = analogRead(pin);
  
  total = total - readings[readIndex];
  readings[readIndex] = raw;
  total = total + readings[readIndex];
  readIndex = (readIndex + 1) % FILTER_SIZE;
  
  doc[name] = total / FILTER_SIZE;
}

UltrasonicSensor::UltrasonicSensor(String name, int trigPin, int echoPin) 
  : Sensor(name, trigPin), echoPin(echoPin) {}

void UltrasonicSensor::begin() {
  pinMode(pin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void UltrasonicSensor::read(JsonDocument& doc) {
  digitalWrite(pin, LOW);
  delayMicroseconds(2);
  digitalWrite(pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) {
    doc[name] = -1.0;
  } else {
    doc[name] = duration * 0.034 / 2.0;
  }
}
