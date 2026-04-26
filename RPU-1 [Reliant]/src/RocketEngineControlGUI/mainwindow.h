#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPortInfo>
#include <QStringList>

#include "communications/serialworker.h"
#include "logging/loghandler.h"
#include "components/alertdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

#define EVENT_POLL_DURATION_MS 10
#define COUNTDOWN_LENGTH_MS -60000 // Negative for Countdown
#define PRESURIZE_FUEL_POINT_MS -30000 // Fuel Pressurization at T-30s
#define AUTO_HOLD_POINT_MS -20000 // Holds at T-30s
#define INERT_FLUSH_POINT_MS -10000 // Inert Flush at T-10s
#define IGNITION_START_POINT_MS -250 // Ignition Starts at T-1.5s

enum class EngineStates : int8_t {
    NO_CONNECTION = -1,
    CONNECTION_ESTABLISHED,
    COUNTDOWN_STARTED,
    PRESSUREIZED_FUEL,
    NITROGEN_FLUSH_STARTED,
    NITROGEN_FLUSH_DONE,
    HOLDING,
    IGNITION,
    PENDING_SHUTDOWN,
    SHUTDOWN_STARTED,
    SHUTDOWN_COMPLETE,
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();   

private slots:
    void handleStartCountdown();
    void handleCountdownUpdate();

    void handlePingCheck();

    void handleShutdown();
    void handleSerialPortRefresh();
    void handleSerialPortSelection(int portIndex);

    void handleCommandAttempt(const QString & command);
    void handleCommandFailed(const QString & command);
    void hanldleSignalReceived(const QString & signal);

    void handleDataAvailable(const SensorData & data);
    void handleCorruptedData(const QByteArray & data);

    void handlePortOpenFailed();
    void handlePortOpenSuccess();

    void handleSerialError(QSerialPort::SerialPortError error, const QString & errorString);

    void handleTelemetryUpdate();

signals:
    void issueCommand(const QString & command);
    void setPings(bool value);
    void serialPortChanged(QSerialPortInfo port);

protected:
    void configureCharts();
    QStringList getSerialPorts();
    void keyPressEvent(QKeyEvent* keyEvent) override;
    void setupConnections();
    void updateUIWithSensorData(const SensorData & data);
    void resetCharts();

private:
    Ui::MainWindow *ui;
    SerialWorker * commsCenter;
    QThread * commsThread;
    AlertDialog * userAlert;
    QTimer * countdown;
    QTimer * pingCheck;
    QTimer * telemetry;
    LogHandler logger;
    EngineStates currentState = EngineStates::NO_CONNECTION;
    EngineStates beforeHoldState = currentState;
    QList<QSerialPortInfo> availableSerialPorts;
    bool pastAutoHold = false;
    int countdownMs = COUNTDOWN_LENGTH_MS;
    int burnDurationMs = -1;
    int timeSinceLastPing = 0;
    int totalDataPacketsReceived = 0;
    float timeSinceLogStart = 0;

    // Communications Telemetry Variables
    int okBitsRecieved = 0;
    int pingsReceived = 0;
    int packetsReceived = 0;
    int invalidCommandReceived = 0;
    int corruptedDataBitsRecieved = 0;
};
#endif // MAINWINDOW_H
