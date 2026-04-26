#define PRODUCTION false
#define ENABLE_LOGGING true
#define READ_SENSORS true
#define ENABLE_PINGS false

#if PRODUCTION
#define BAUD_RATE 38400
#define ENABLE_LOGGING true  // Production should always have logging enabled
#define ENABLE_PINGS true    // Production should always have pings
#define READ_SENSORS true    // Production should always read from sensors
#define SerialPort Serial1   // Use the production serial pins
#else
#define BAUD_RATE 76800
#define SerialPort Serial1  // Use the USB port for debugging
#endif

#include "commands.h"
#include "pins.h"
#include "sensors.h"
#include <Servo.h>

char dataIn[COMMAND_SIZE_BYTES + 1];
EngineStates currentState = EngineStates::NO_CONNECTION;
unsigned int lastPingTime = 0;
unsigned int lastEventTime = 0;
int commandRepeats = 0;

uint64_t lastDataTransmissionTime = 0;


void setup() {
  Serial.begin(9600);
  SerialPort.begin(BAUD_RATE);
  while (!SerialPort) {
    ;
  }
  setupPins();
  Serial.println("Setup Resetting Pins");
  resetPins();
  lastPingTime = millis();
  srand(analogRead(15));
}

void loop() {
  // Handle any incomming commands
  if (SerialPort.available() >= COMMAND_SIZE_BYTES) {
    SerialPort.readBytes(dataIn, COMMAND_SIZE_BYTES);
    // SerialPort.println(SerialPort.available());
    dataIn[COMMAND_SIZE_BYTES] = 0; // Null Terminator
    handleCommand(dataIn, currentState, lastPingTime, lastEventTime, commandRepeats);
  }

  // Read Sensor Data, Send it back, and validate it
  if (currentState >= EngineStates::LOG_START && currentState != EngineStates::SHUTDOWN_CONFIRMED && ENABLE_LOGGING) {
    uint64_t currentTime = millis();
    if ((currentTime - lastDataTransmissionTime) >= MINIMUM_DATA_TRANSMIT_TIME_MS) {
      readSensorData(sensorData);
      sendSensorData(sensorData);
      lastDataTransmissionTime = currentTime;
    }
  }

  // If we should be validating data, then start validating that data
  if (currentState >= EngineStates::FUEL_PRESSURIZATION && currentState < EngineStates::SHUTDOWN_NITROGEN_FLUSH_STARTED && ENABLE_LOGGING) {
    if (!nominalDataCheck(sensorData)) {
      startShutdown();
      currentState = EngineStates::SHUTDOWN_NITROGEN_FLUSH_STARTED;
      lastEventTime = millis();
      SerialPort.write(SHUTDOWN, COMMAND_SIZE_BYTES);
      SerialPort.flush();
    }
  }

  // Handle Lost Connection
  if (commandRepeats > MAX_COMMAND_REPEATS) {
    if (currentState >= EngineStates::OXIDIZER_OPEN) {
      startShutdown();
      lastEventTime = millis();
      commandRepeats = 0;
      currentState = EngineStates::CONNECTION_LOST;
      SerialPort.write(CONNECTION_LOST, COMMAND_SIZE_BYTES);
      SerialPort.flush();
    } else {
      Serial.println("Max Repeat Resetting Pins");
      resetPins();

      SerialPort.write(CONNECTION_LOST, COMMAND_SIZE_BYTES);
      SerialPort.flush();
      digitalWrite(LED3, HIGH);
      currentState = EngineStates::NO_CONNECTION;
    }
  }

  // Check for pings
  if (currentState >= EngineStates::CONNECTION_ESTABLISHED && ENABLE_PINGS) {
    int currentTime = millis();
    // If there has been too much time between pings, then attempt shutting down.
    if ((currentTime - lastPingTime) > MAX_PING_DELAY_MS) {
      if (currentState >= EngineStates::OXIDIZER_OPEN) {
        startShutdown();
        lastEventTime = millis();
        commandRepeats = 0;
        currentState = EngineStates::CONNECTION_LOST;
        SerialPort.write(CONNECTION_LOST, COMMAND_SIZE_BYTES);
        SerialPort.flush();
      } else {
        Serial.println("Pings Lost Resetting Pins");
        resetPins();
        SerialPort.write(CONNECTION_LOST, COMMAND_SIZE_BYTES);
        SerialPort.flush();
        digitalWrite(LED3, HIGH);
        currentState = EngineStates::NO_CONNECTION;
      }
    }
  }

  // Handle any multi-step commands (inert flush, ignition, shutdown)

  // Handling finishing the pre-burn inert flush
  if (currentState == EngineStates::PRE_BURN_NITROGEN_FLUSH_STARTED) {
    int currentTime = millis();
    if ((currentTime - lastEventTime) > NITROGEN_FLUSH_DELAY_MS) {
      closeValve(NITROGENVALVE);
      currentState = EngineStates::PRE_BURN_NITROGEN_FLUSH_FINISHED;
      SerialPort.write(NITROGEN_FLUSH_FINISHED, COMMAND_SIZE_BYTES);
      SerialPort.flush();
    }
  }

  // Handling lighting the igniter after fuel flow
  if (currentState == EngineStates::OXIDIZER_OPEN) {
    int currentTime = millis();
    if ((currentTime - lastEventTime) > FUEL_VALVE_OPEN_DELAY_MS) {
      openValve(IGNITERVALVE);
      openValve(FUELVALVE);
      currentState = EngineStates::IGNITION;
      lastEventTime = millis();
    }
  }

  // Handle the Shutdown Inert Flush
  if (currentState == EngineStates::SHUTDOWN_NITROGEN_FLUSH_STARTED) {
    int currentTime = millis();
    if ((currentTime - lastEventTime) > NITROGEN_FLUSH_DELAY_MS) {
      closeValve(NITROGENVALVE);
      currentState = EngineStates::SHUTDOWN_CONFIRMED;
      SerialPort.write(SHUTDOWN_CONFIRMED, COMMAND_SIZE_BYTES);  // Indicate to the control computer the shutdown is done
      SerialPort.flush();
      digitalWrite(LED1, HIGH);
    }
  }

  // Handle the Shutdown Inert Flush when connection is lost
  if (currentState == EngineStates::CONNECTION_LOST) {
    int currentTime = millis();
    if ((currentTime - lastEventTime) > NITROGEN_FLUSH_DELAY_MS) {
      closeValve(NITROGENVALVE);
      currentState = EngineStates::NO_CONNECTION;
      SerialPort.write(SHUTDOWN_CONFIRMED, COMMAND_SIZE_BYTES);  // Indicate to the control computer the shutdown is done
      SerialPort.flush();
      digitalWrite(LED3, HIGH);
    }
  }
}
