#include "Arduino.h"
#ifndef __COMMANDS_H_
#define __COMMANDS_H_

#include "pins.h"
#include "sensors.h"
#include <string.h>
#include <Servo.h>

#define COMMAND_SIZE_BYTES 8
#define MAX_PING_DELAY_MS 250
#define MAX_COMMAND_REPEATS 3
#define NITROGEN_FLUSH_DELAY_MS 5000
#define FUEL_VALVE_OPEN_DELAY_MS 250
#define SERVO_VALVE_OPEN_POSITION 180
#define SERVO_VALVE_CLOSED_POSITION 0


constexpr char COMMS_ESTABLISHED[] = "CtrlActi";
constexpr char LOG_START[] = "LogStart";
constexpr char INERT_FLUSH[] = "InrtFlsh";
constexpr char PRES_FUEL[] = "PresFuel";
constexpr char IGNITION[] = "Ignition";
constexpr char SHUTDOWN[] = "Shutdown";
constexpr char PING_COMMAND[] = "PingPong";

constexpr char NITROGEN_FLUSH_FINISHED[] = "FlushFin";
constexpr char SHUTDOWN_CONFIRMED[] = "ShutConf";
constexpr char INVALID_COMMAND[] = "InvldCom";
constexpr char CONNECTION_LOST[] = "ConnLost";

enum class EngineStates {
  CONNECTION_LOST = -1,
  NO_CONNECTION,
  CONNECTION_ESTABLISHED,
  LOG_START,
  PRE_BURN_NITROGEN_FLUSH_STARTED,
  PRE_BURN_NITROGEN_FLUSH_FINISHED,
  FUEL_PRESSURIZATION,
  OXIDIZER_OPEN,
  IGNITION,
  SHUTDOWN_NITROGEN_FLUSH_STARTED,
  SHUTDOWN_CONFIRMED
};

char dataOut[sizeof(Sensors)];
Sensors sensorData = { 0 };

void sendSensorData(Sensors &sensorData) {
  memcpy(dataOut, &sensorData, sizeof(Sensors));
  SerialPort.write(dataOut, sizeof(Sensors));
  SerialPort.flush();
}

void openValve(int pin) {
  digitalWrite(pin, HIGH);
}

void openServoValve(Servo &valve) {
  // valve.write(SERVO_VALVE_OPEN_POSITION);
  digitalWrite(OXIDIZERVALVE, HIGH);
}

void closeValve(int pin) {
  digitalWrite(pin, LOW);
}

void closeServoValve(Servo &valve) {
  // valve.write(SERVO_VALVE_CLOSED_POSITION);
  digitalWrite(OXIDIZERVALVE, LOW);
}

void startShutdown() {
  Serial.println("Shutdown Reset Pins");
  resetPins();
  openValve(NITROGENVALVE);
  closeValve(SHUTDOWN_VALVE);  // Closing the SHUTDOWN_VALVE actually opens it since it is normally open
}

void handleCommand(char *command, EngineStates &currentState, unsigned int &lastPingTime, unsigned int &lastEventTime, int &commandRepeats) {

  lastPingTime = millis();  // Any command (even invalid ones) can act as a ping since the ping is just meant to make
  // sure that the comms link is still solid in both direction, and any response will allow us to understand this.

  if (!strcmp(command, PING_COMMAND)) {
    SerialPort.write(PING_COMMAND, COMMAND_SIZE_BYTES);
    SerialPort.flush();
    return;
  }

  if (!strcmp(command, COMMS_ESTABLISHED)) {
    if (currentState == EngineStates::CONNECTION_ESTABLISHED) {
      SerialPort.write(COMMS_ESTABLISHED, COMMAND_SIZE_BYTES);
      commandRepeats++;
      SerialPort.flush();
      return;
    }

    //If we are in the startup (No Connection) state, then we should transition to the comms established state
    if (currentState == EngineStates::NO_CONNECTION) {
      commandRepeats = 0;
      resetPins();
      SerialPort.write(COMMS_ESTABLISHED, COMMAND_SIZE_BYTES);
      currentState = EngineStates::CONNECTION_ESTABLISHED;
      SerialPort.flush();
      return;
    }

    // Otherwise, this indicates that comms may have been lost and we should re-start
    if (currentState >= EngineStates::OXIDIZER_OPEN) {
      startShutdown();
    } else {
      resetPins();
    }
    currentState = EngineStates::CONNECTION_LOST;
    lastEventTime = millis();
    SerialPort.write(CONNECTION_LOST, COMMAND_SIZE_BYTES);
    SerialPort.flush();
    return;
  }

  if (!strcmp(command, LOG_START)) {
    // This is a repeat command of a command we have seen before
    if (currentState == EngineStates::LOG_START) {
      commandRepeats++;
      SerialPort.write(LOG_START, COMMAND_SIZE_BYTES);
      resetPins();
      SerialPort.flush();
      return;
    }

    if (currentState == EngineStates::SHUTDOWN_CONFIRMED) {
      SerialPort.write(LOG_START, COMMAND_SIZE_BYTES);
      resetPins();
      currentState = EngineStates::LOG_START;
      commandRepeats = 0;
      SerialPort.flush();
      return;
    }

    // If we are in the Connection Established State then transition to the log start state
    if (currentState == EngineStates::CONNECTION_ESTABLISHED) {
      SerialPort.write(LOG_START, COMMAND_SIZE_BYTES);
      currentState = EngineStates::LOG_START;
      commandRepeats = 0;
      SerialPort.flush();
      return;
    }

    // In any other state, this is an invalid command.
    SerialPort.write(INVALID_COMMAND, COMMAND_SIZE_BYTES);
    SerialPort.flush();
    return;
  }

  if (!strcmp(command, PRES_FUEL)) {
    if (currentState == EngineStates::FUEL_PRESSURIZATION) {
      commandRepeats++;
      SerialPort.write(PRES_FUEL, COMMAND_SIZE_BYTES);
      SerialPort.flush();
      return;
    }

    if (currentState == EngineStates::LOG_START) {
      commandRepeats = 0;
      SerialPort.write(PRES_FUEL, COMMAND_SIZE_BYTES);
      openValve(PRESFUELVALVE);
      openValve(SHUTDOWN_VALVE);
      currentState = EngineStates::FUEL_PRESSURIZATION;
      SerialPort.flush();
      return;
    }

    // In any other state, this is an invalid command.
    SerialPort.write(INVALID_COMMAND, COMMAND_SIZE_BYTES);
    SerialPort.flush();
    return;
  }

  if (!strcmp(command, INERT_FLUSH)) {
    if (currentState == EngineStates::PRE_BURN_NITROGEN_FLUSH_STARTED
        || currentState == EngineStates::PRE_BURN_NITROGEN_FLUSH_FINISHED) {
      commandRepeats++;
      SerialPort.write(INERT_FLUSH, COMMAND_SIZE_BYTES);
      SerialPort.flush();
      return;
    }

    // if (currentState == EngineStates::LOG_START || currentState == EngineStates::SHUTDOWN_CONFIRMED) {
    if (currentState == EngineStates::FUEL_PRESSURIZATION) {
      commandRepeats = 0;
      SerialPort.write(INERT_FLUSH, COMMAND_SIZE_BYTES);
      openValve(NITROGENVALVE);
      lastEventTime = millis();
      currentState = EngineStates::PRE_BURN_NITROGEN_FLUSH_STARTED;
      SerialPort.flush();
      return;
    }

    // In any other state, this is an invalid command.
    SerialPort.write(INVALID_COMMAND, COMMAND_SIZE_BYTES);
    SerialPort.flush();
    return;
  }

  if (!strcmp(command, IGNITION)) {
    if (currentState == EngineStates::OXIDIZER_OPEN
        || currentState == EngineStates::IGNITION) {
      commandRepeats++;
      SerialPort.write(IGNITION, COMMAND_SIZE_BYTES);
      SerialPort.flush();
      return;
    }

    if (currentState == EngineStates::PRE_BURN_NITROGEN_FLUSH_FINISHED) {
      commandRepeats = 0;
      SerialPort.write(IGNITION, COMMAND_SIZE_BYTES);
      openServoValve(oxidizerValve);
      Serial.println("Opening");
      digitalWrite(LED2, HIGH);
      lastEventTime = millis();
      currentState = EngineStates::OXIDIZER_OPEN;
      return;
    }

    // In any other state, this is an invalid command.
    SerialPort.write(INVALID_COMMAND, COMMAND_SIZE_BYTES);
    SerialPort.flush();
    return;
  }

  if (!strcmp(command, SHUTDOWN)) {
    if (currentState == EngineStates::SHUTDOWN_NITROGEN_FLUSH_STARTED) {
      SerialPort.write(SHUTDOWN, COMMAND_SIZE_BYTES);
      SerialPort.flush();
      return;
    }

    if (currentState == EngineStates::SHUTDOWN_CONFIRMED) {
      SerialPort.write(SHUTDOWN, COMMAND_SIZE_BYTES);
      SerialPort.write(SHUTDOWN_CONFIRMED, COMMAND_SIZE_BYTES);
      SerialPort.flush();
      return;
    }
    // If we are in the ignition state or later then we should flush
    // Otherwise we should just go into the shutdown confirmed state.

    if (currentState >= EngineStates::OXIDIZER_OPEN) {
      startShutdown();
      currentState = EngineStates::SHUTDOWN_NITROGEN_FLUSH_STARTED;
      lastEventTime = millis();
      SerialPort.write(SHUTDOWN, COMMAND_SIZE_BYTES);
      commandRepeats = 0;
      return;
    }

    // If we have not yet gotten to the state where fuel / oxidizer has flown into the
    // lines then we can just directly transition into the shutdown confirmed state.

    currentState = EngineStates::SHUTDOWN_CONFIRMED;
    SerialPort.write(SHUTDOWN, COMMAND_SIZE_BYTES);
    SerialPort.write(SHUTDOWN_CONFIRMED, COMMAND_SIZE_BYTES);  // Indicate to the control computer the shutdown is done
    digitalWrite(LED1, HIGH);
    commandRepeats = 0;
    SerialPort.flush();
    return;
  }

  SerialPort.write(INVALID_COMMAND, COMMAND_SIZE_BYTES);
  SerialPort.write(command, COMMAND_SIZE_BYTES);
  // Flush the input buffer if we get an unknown command
  SerialPort.end();
  SerialPort.begin(BAUD_RATE);
  while (!Serial) {}
}


#endif  // __COMMANDS_H_