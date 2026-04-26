#include "Arduino.h"
#ifndef __PINS_H_
#define __PINS_H_
#include "HX711.h"
#include <Servo.h>

// Pin Values (Digital)
constexpr int NITROGENVALVE = 28;
constexpr int PRESFUELVALVE = 26;
constexpr int FUELVALVE = 27;
constexpr int OXIDIZERVALVE = A0;
constexpr int IGNITERVALVE = 29;
constexpr int SHUTDOWN_VALVE = 30;

// Analog Pins for Pressure Transducers
// constexpr int PRESSSEN1 = A0;
constexpr int PRESSSEN2 = A1;
constexpr int PRESSSEN3 = A2;
constexpr int PRESSSEN4 = A3;
constexpr int PRESSSEN5 = A4;
constexpr int PRESSSEN6 = A5;

// Digital Pin for Load Cell
constexpr int LOADCELL = 24;

//SCK Pin for Load Cell
constexpr int LOADCELL_SCK = 23;

//Creating scale object for Load Cell
HX711 scale;
Servo oxidizerValve;


// Thermocouple Analog Pins
constexpr int THERMOCPL1 = A6;
constexpr int THERMOCPL2 = A7;
constexpr int THERMOCPL3 = A8;
constexpr int THERMOCPL4 = A9;

// LED Ports (Digital)
constexpr int LED1 = 6;
constexpr int LED2 = 5;
constexpr int LED3 = 4;

void setupPins() {
  pinMode(OXIDIZERVALVE, OUTPUT);
  digitalWrite(OXIDIZERVALVE, LOW);
  scale.begin(LOADCELL, LOADCELL_SCK);
  pinMode(PRESSSEN2, INPUT);
  pinMode(PRESSSEN3, INPUT);
  pinMode(PRESSSEN4, INPUT);
  pinMode(PRESSSEN5, INPUT);
  pinMode(PRESSSEN6, INPUT);
  pinMode(THERMOCPL1, INPUT);
  pinMode(THERMOCPL2, INPUT);
  pinMode(THERMOCPL3, INPUT);
  pinMode(THERMOCPL4, INPUT);

  pinMode(NITROGENVALVE, OUTPUT);
  pinMode(FUELVALVE, OUTPUT);
  pinMode(IGNITERVALVE, OUTPUT);
  pinMode(PRESFUELVALVE, OUTPUT);
  pinMode(SHUTDOWN_VALVE, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  Serial.println("Setup Pins");
}

void resetPins() {
  Serial.println("Resetting Pins. ");
  digitalWrite(OXIDIZERVALVE, LOW);
  digitalWrite(NITROGENVALVE, LOW);
  digitalWrite(FUELVALVE, LOW);
  digitalWrite(IGNITERVALVE, LOW);
  digitalWrite(PRESFUELVALVE, LOW);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);
}

#endif  // __PINS_H_