#include<Servo.h>

#define OXIDIZER_VALVE_SERVO_PIN 8
#define OXIDIZER_VALVE_OPEN_POSITION 10
#define OXIDIZER_VALVE_CLOSED_POSITION 105;
#define SERVO_STATUS_PIN 3

Servo oxidizer_valve;

void setup() {
  oxidizer_valve.attach(OXIDIZER_VALVE_SERVO_PIN);
  pinMode(SERVO_STATUS_PIN, INPUT);
}

void loop() {
  // If Servo Status Pin is High then we open the valve, otherwise close it
  if (digitalRead(SERVO_STATUS_PIN)){
    oxidizer_valve.write(OXIDIZER_VALVE_OPEN_POSITION);
  }else{
    oxidizer_valve.write(OXIDIZER_VALVE_CLOSED_POSITION);
  }
}