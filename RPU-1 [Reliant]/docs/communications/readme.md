# RPU Systems Design and Communications Protocol V1.0.0

> [!IMPORTANT]
> Last Updated: 7/22/2024

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

All of these technologies are open for discussion, the main focus of systems this semester was the Rocket Engine side of things and developing systems to physically control the engine and read data. 

There may be an argument to switch to something like Python and use something like PyQt to do the GUI to make this cross-platform and possibly simplify development. As of right now this is the technology stack:

- C++ 
- x86-64 Linux (Ubuntu 22.04 but should work generally on most systems)
    - Linux makes C++ development and Serial port access easier
- FTXUI
    - A TUI (Terminal UI) for displaying data in a human readable fashion
- CMake for building the project and managing deps


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
> - Setup is easier as there is no need to ssh into the arduino and run the control program, it starts up automatically on power on.
> - The Raspberry Pi has no Analog pin headers, the pressure transducers are analog sensors, switching to Arduino removes the need for analog-digital converters.
> - The Arduino takes less power, we can power it really easily via the VIN and GND pins compared to RasPI which is much harder to power
> - Arduino has first class support for our serial communications, where as raspberry pi has support but is slightly more difficult to implement
> - Arduino has no OS that is also has to run, it just runs our code and our code only.

## Sensors 

The rocket engine has 8 sensors:
- 1 Load Cell for measuring thrust
- 1 Thermocouple placed before the injector plate
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

The control room and the test stand use 64 bit (8 byte) signals to communicate different actions / states to each other the signals are as follows.


- Control Room Signals (sent from Control Room to Test Stand)
    - CtrlActi
        - Short for Control Room Active, sent as a response to the `TestActi` signal to indicate a connection between Control Room and Test Stand
    - LogStart
        - Short for Logging Start, indicates to the test stand to start reading and sending sensor data
    - InertGas
        - Triggers the Inert Gas Flush before / after firing
    - Ignition
        - Triggers the ignition and startup sequence
    - ShutDown
        - Triggers the shutdown sequence, closing the valves and performing an inert gas flush.
    - StopLogs
        - Tells the stand to terminate data logging, stops reading from the sensors and stops data transmission. The test stand must sent its `ShutDown` signal indicating the test stand is in the shutdown state.

- Test Stand Signals (sent from Test Stand to Control Room)
    - TestActi
        - Short for Test Stand Active, sent from the test stand once every second on test stand startup to indicate that the test stand is online and waiting for connection from the Control Room
    - CommsEst
        - Short for Communications Established, indicates that the Test Stand has received the Control Room's `CtrlActi` signal, and that a connection has been established.
    - InertFlw
        - Short for Inert Gas Flowing, indicates that the Test Stand is starting the Inert Gas flush.
    - InertStp
        - Short for Inert Gas Stop, indicates that the inert gas valves are closed, terminating the inert gas flush.
    - IgnStart
        - Short for Ignition Start, indicates that the test stand is attempting ignition of the engine.
    - HangFire
        - Indicates that there was no thrust produced or there was no oxidizer flow 1s after ignition so valves were automatically closed. Engine will start shutdown procedure.
    - AbrtFire
        - Short for Abort Fire, indicates some sensor is outside a safe window and fire is being aborted. Engine will start the shutdown procedure.
    - ShtDnRun
        - Short for Shutdown Running, indicates, that the test stand is running the shutdown procedure.
    - ShutDown
        - Sent after the test stand has shut all the valves and is done its shutdown procedure. The test stand will continue to send sensor data until it receives the `StopLogs` signal.
    - StopLogs
        - Sent as a response to the Control Room's `StopLogs` signal to indicate that the Test Stand is terminating logging.
        

## Sensor Data Transfer

Sensor data is sent in packets of 32 bytes (256 bits) of binary data with the following structure:

```cpp
typedef struct
{
  float loadCell;
  float thermocouple;
  float pressure[6];
} SensorData;
```


## Control Flow

1. Test Stand comes online, sets up all the pins for the engine, and starts sending the `TestActi` signal on the Serial1 port once every second.
1. Control Room comes online, and sends the `CtrlActi` signal, this signal will be sent again if the control room sees the `TestActi` signal again.
1. Test Stand sees the `CtrlActi` signal and responds with the `CommsEst` signal to indicate successful connection
1. The test stand will wait to start logging until it sees the `LogStart` signal from the control room. Upon receiving that signal, it will start reading from the sensors and sending data.
1. Upon receiving data, the control room will store that data into a CSV for later processing and then displaying that data in the chart
1. The control room will handle the countdown, performing an automatic hold at T-30s to allow us to perform 1 last check before ignition. 
    1. If we decide to continue with the countdown, the Control Room will send the `InertGas` signal at T-10s **(needs verification)** for an inert gas flush, and then send the `Ignition` at T-3s to ensure ignition is started by T-0s.
    1. If we decide to stop and abort, then the `ShutDown` signal is sent shutting down the test stand and ensuring all valves are closed.
1. On receive of the `InertGas` the Test Stand will respond with `InertFlw` and open the inert gas valves for some time **still to be decided**. Then after the valves are closed it will send an `InertStp` signal to indicate that the inert gas is done flowing.
1. On receiving the `Ignition` signal, the test stand will respond with `IgnStart` and then open fuel valve, ignite the igniter, and then open the oxidizer valve.
1. The test stand will monitor the sensors and will send either the `HangFire` signal if it detects a hang fire or `AbrtFire` signals if anything is outside the safe window.
1. If everything is ok, then ignition will continue until the fuel runs out, in which the `ShutDown` signal will be sent by the control room, the test stand will reply with `ShtDnRun` close valves, and perform any additional shutdown steps, then send its `ShutDown` signal.
1. Once the team is happy with the data collected, the `StopLogs` signal will be sent from the control room and the test stand will terminate its logging.


## Arduino Pinout

The Pinout is still being developed, the only finalized pins at the moment are:

- PIN 18: Output
    - Serial1 TX pin
- PIN 19: Input
    - Serial1 RX pin

## Additional Info

For additional info into how to build the Control Room software, configure the Arduino, and other parts of the project, consult the docs folder of the [GitHub Repository](https://github.com/Rensselaer-Spaceflight-Society/Rocket-Engine)

 