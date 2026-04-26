# RPU Systems Design and Communications Protocol V1.1.0

> [!IMPORTANT]
> Last Updated: 10/8/2024

Below is a brief overview of the design and communications protocol between our laptop acting as our remote interface (i.e. the control room) and the Rocket Engine Test Stand itself.

## Table of Contents

- [Control Room](#control-room)
- [Test Stand Controller](#test-stand-controller)
- [Powering the Test Stand](#powering-the-test-stand)
- [Sensors](#sensors)
- [Valves](#valves)
- [Communications](#communications)
- [Command and Control](#command-and-control)
- [Sensor Data Transfer](#sensor-data-transfer)
- [Control Flow](#control-flow)
- [Arduino Pinout](#arduino-pinout)
- [Additional Info](#additional-info)

## Control Room

The software that remotely monitors the engine and allows us to send commands to the engine. This software is responsible for:

- Countdown
- Receiving and Saving Data
- Hold and Abort Signals
- Displaying the Data in a Human Friendly Way
  - Charts
  - Text Outputs
  - Color coding for if data is out of spec

### Technologies

We are using C++ and Qt 6.7.2 to build the control room software. You can install the Qt framework for Open Source Usage from https://www.qt.io/download-open-source.

## Test Stand Controller

The rocket engine controller is an Arduino based platform that controls the physical aspects of the rocket engine. This was the primary focus for the semester and is still the primary focus for this summer. This software is responsible for:

- Reading Sensors
- Opening and Closing Valves
- Sending Sensor Data back to the Control Room
- Ignition and Shutoff Sequence
- Processing Control Signals

## Powering the Test Stand

We have a variety of electronics that need power:

- Solenoid Valves, 48W (12vdc @4A)
- Arduino, 1W (12vdc @0.08333)
- Sensors, 5W (5v @1A)

The sensors need 5v, but everything else needs 12v, fortunately, the Arduino's power regulation will step the power down to 5v for the sensors.

Its really hard to send 60w @ 12vdc over 300ft, so we decided that battery power is our best bet for powering the test stand. Lithium Iron Phosphate (LiFePO4) batteries show the best potential for delivering both the voltage and current that the test stand will need.

The use of battery power allows us to set our downrange distance anywhere from 300ft (100m) - 3400ft (1000m).

### Technologies

This is built on the Arduino platform for easy and plentiful I/O. Arduino provides plenty of digital and analog pins for reading sensors and controlling valves. It also provides simple serial communications over UART.

> [!NOTE]
> Originally the rocket engine was supposed to be controlled via Raspberry Pi, we switched to the arduino platform for a couple reasons.
>
> - Setup is easier as there is no need to ssh into the arduino and run the control program, it starts up automatically on power on.
> - The Raspberry Pi has no Analog pin headers, the pressure transducers are analog sensors, switching to Arduino removes the need for analog-digital converters.
> - The Arduino takes less power, we can power it really easily via the VIN and GND pins compared to RasPI which is much harder to power
> - Arduino has first class support for our serial communications, where as raspberry pi has support but is slightly more difficult to implement
> - Arduino has no OS that is also has to run, it just runs our code and our code only.

## Sensors

The rocket engine has 8 sensors:

- 1 Load Cell for measuring thrust
- 4 Thermocouples
    - 1 At the Nozzle exit
    - 1 On the Kerosene Inlet and Injector Plate
    - 1 On the N2O inlet and injector Plate
    - On the combustion chamber at the throat of the nozzle (the constriction)
- 6 Pressure Transducers
  - 1 Inside the Combustion Chamber
    - According to David, the back of the combustion chamber should be cool enough to not destroy this sensor
  - 1 Down Line of the Inert Gas System
  - 2 Before the injector plate
    - 1 for fuel
    - 1 for oxidizer
  - 2 After the valves for fuel and oxidizer

The pressure transducers are analog sensors that vary their voltage depending on the pressure they are detecting.

The thermocouple and load cell both have amplifiers that act as analog-digital converters. The signals we take in will be digital signals. The thermocouple unfortunately does work on SPI (sorry Ashton).

## Valves

We are using three 12v solenoid valves that operate at 1.33 amps. This is simply too much power to send over a wire with our budget. See the power section for more information.

Power will be delivered to the solenoids via solid-state relays, this allows the
5v logic level of the Arduino power the relays reliably.

## Communications

The goal is to not be anywhere close to the engine when it is firing, the team is still deciding on an acceptable distance, but the current number is between 50 - 100m (170 - 340ft). To achieve this range with easy and reliable communications we are using UART serial communications via ethernet. This provides a well supported, and robust communication method.

> [!IMPORTANT]
> The Baud Rate for the serial communications has yet to be finalized, for testing we are using 9600 but this may change in the future.

Specifically the serial data will be converted to RS485 and sent over Cat 5 ethernet cable, this is supposedly an effective way to handle long range wired communication. [Source](https://electronics.stackexchange.com/questions/113188/long-distance-communication)

The Arduino's serial communication library automatically handles parity, start, and stop bits, so we don't need to handle this necessarily. The default is 8 data, 1 start and 1 stop bit with no parity, but we may be able to change this if needed.

Communications should be handled over the the Serial1 Port on the Arduino, Serial0 (aka just "Serial) is reserved for debugging via the Serial Monitor.

## Command and Control

The control room and the test stand use 64 bit (8 byte) signals to communicate different actions / states to each other the signals are as follows. When the test stand receives a command it should send an acknowledgement. If the control room fails to receive an acknowledgement it should reattempt sending the command up to **5** times. If after these
attempts there still is no acknowledgement, then the control room should assume connection
is lost and notify the user. If the test stand receives a command that it has already received then it should just send an acknowledgement instead of rerunning code.

If test stand connection is lost, then the test stand will automatically go into shutdown procedure.

The acknowledgement is just a repeat of the command received.

The following sequences are the different commands sent by the control computer and what
they tell the test stand to do.

- `CtrlActi` Control Computer Active, tells the test stand that a communication link is active.
- `LogStart` Tells the test stand to start the logging of sensor data and communicating that down the line.
- `InrtFlsh` Tells the test stand to perform an inert gas flush
- `PresFuel` Tells the test stand to open the pressure valve to the Kerosene
- `Ignition` Tells the test stand to open the valves and light the Steel Wool following the ignition protocol
- `Shutdown` Tells the test stand to close the oxidizer valve first, then the fuel valve.

On Shutdown, the test stand should send a message that the engine was shutdown.

## Sensor Data Transfer

Sensor data is sent in packets of 64 bytes of binary data with the following structure:

// The order of sensors is:
/*
 * thermocouple[0] = injector plate & kerosene inlet
 * thermocouple[1] = injector plate & oxidizer inlet
 * thermocouple[2] = outside the cc at the throat
 * thermocouple[3] = on the nozzle near the outlet
 *
 * pressureTransducer[0] = combustion chamber
 * pressureTransducer[1] = kerosene feed-line pressure
 * pressureTransducer[2] = kerosene tank pressure
 * pressureTransducer[3] = kerosene line pressure
 * pressureTransducer[4] = oxidizer tank pressure
 * pressureTransducer[5] = oxidizer line pressure
*/

```cpp
typedef struct
{
  char header[8] = "DataPack"; // Data recieved should be "DataPack" but cannot be assumed to be
  float loadCell;
  float thermocouple[4];
  float pressureTransducer[6];
  char checksum[12];
} SensorData;
```
The checksum is a 12-byte XOR hash that verifies all the data is solid and can be found below:

```cpp
void checksum12(void *checksum, const void *data, int n) {
    uint8_t* checksumPtr = (uint8_t *) checksum;
    const uint8_t* dataPtr = (uint8_t *) data;
    memset(checksum, 0, 12);
    for (int i = 0; i < n; ++i) {
        checksumPtr[i % 12] ^= dataPtr[i];
        checksumPtr[(i + 1) % 12] ^= (dataPtr[i] >> 4) | (dataPtr[i] << 4);  // Simple mixing
    }
}
```


## Control Flow

1. Test Stand comes online, sets up all the pins for the engine
1. Control Room comes online, and sends the `CtrlActi` signal while waiting for a `LogStart` signal
1. Control Room sends the `LogStart` signal, and the Test Stand begins sending Sensor Data every quarter second, and the `LogStart` signal.
1. Control Room sees the `LogStart` signal, and sends the `PressFuel` signal, which once seen by the Test Stand is sent back after the Fuel Tank is pressurized
1. Control Room sees the `PressFuel` signal, and sends the `InertFlush` signal, which once seen by the Test Stand is returned after a brief Inert Flush
1. Control Room sees the `InertFlush` signal and sends the `Ignite` signal, which once seen by the Test Stand is returned after the ignition sequence begins
1. After Ignition, the Test Stand sends sensor data as fast as it can for the duration of operation, whilst also checking if conditions are nominal
1. Assuming conditions are nominal, the Test Stand and the Control Room send the `Ping` Signal back and forth to ensure a stable connection.
1. If Conditions were unstable, there has been a significant amount of time since the Test Stand has received a `Ping`, or the Control Room has sent the `Shutdown` signal, the Test Stand intiates a Shutdown Sequence

## Arduino Pinout

See the Schematic using KiCad for the most up to date Arduino Pins.


## Additional Info

For additional info into how to build the Control Room software, configure the Arduino, and other parts of the project, consult the docs folder of the [GitHub Repository](https://github.com/Rensselaer-Spaceflight-Society/Rocket-Engine)
