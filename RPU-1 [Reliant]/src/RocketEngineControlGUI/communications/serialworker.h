#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <cmath>

#include <QThread>
#include <QDebug>
#include <QSerialPort>
#include <QTimer>

#include "sensordata.h"

#define COMMAND_SEND_TIMEOUT_DURRATION_MS 40
#define BYTES_IN_COMMAND 8
#define MAX_COMMAND_RETRIES 5
#define MAX_PING_NON_RESPONSE_DELAY_MS COMMAND_SEND_TIMEOUT_DURRATION_MS * MAX_COMMAND_RETRIES

// IMPORTANT: ALL COMMANDS MUST BE BYTES_IN_COMMAND NUMBER OF CHARS
// OTHERWISE WE COULD HAVE FUN BUFFER OVERFLOWS
constexpr char CONTROL_ACTIVE_COMMAND[] = "CtrlActi";
constexpr char PING_COMMAND[] = "PingPong";
constexpr char LOG_START_COMMAND[] = "LogStart";
constexpr char INERT_GAS_FLUSH_COMMAND[] = "InrtFlsh";
constexpr char PRESURIZE_FUEL_COMMAND[] = "PresFuel";
constexpr char IGNITION_COMMAND[] = "Ignition";
constexpr char SHUTDOWN_COMMAND[] = "Shutdown";

// Signals that define state of engine
// These are never sent to the engine, they are only recieved
constexpr char INERT_FLUSH_FINISHED[] = "FlushFin";
constexpr char SHUTDOWN_CONFIRMED[] = "ShutConf";
constexpr char INVALID_COMMAND[] = "InvldCom";
constexpr char CONNECTION_LOST[] = "ConnLost";

const QString EngineSignals[] = {
    CONTROL_ACTIVE_COMMAND,
    PING_COMMAND,
    LOG_START_COMMAND,
    INERT_GAS_FLUSH_COMMAND,
    PRESURIZE_FUEL_COMMAND,
    IGNITION_COMMAND,
    SHUTDOWN_COMMAND,
    INERT_FLUSH_FINISHED,
    SHUTDOWN_CONFIRMED,
    INVALID_COMMAND,
    CONNECTION_LOST,
};

#define NUM_ENGINE_SIGNALS sizeof(EngineSignals) / sizeof(EngineSignals[0])

// Forward Declaration of the MainWindow to avoid circular include dependencies
class MainWindow;

class SerialWorker : public QObject
{
    Q_OBJECT
public:
    explicit SerialWorker(MainWindow * window, QObject *parent = nullptr);
    ~SerialWorker();

protected:
    void checksum12(void * checksum, const void * data, int n);
    void processSensorData(QByteArray && data);

public slots:
    void onPortNameChange(const QSerialPortInfo & port);
    void setStartPings(bool value);
    void issueCommand(const QString & command);

private slots:
    void handleReadReady();
    void handleSerialError(QSerialPort::SerialPortError error);
    void handleTimeout();

signals:
    void commandAttempt(const QString & command);
    void commandFailed(const QString & command);

    void signalReceived(const QString & command);

    void dataAvailable(const SensorData &data);
    void corruptedData(const QByteArray &data);

    void portOpenFailed();
    void portOpenSuccess();
    void serialErrorOccurred(QSerialPort::SerialPortError &error, const QString &message);

private:
    MainWindow * mainWindow;
    QSerialPort * serialPort;
    QByteArray * dataBuffer;
    QTimer* commandTimer;
    QString commandToSend;
    QString mostRecentlySentCommand;
    bool startPings = false;
    size_t commandRetries = 0;
};

#endif // SERIALWORKER_H
