#ifndef LOGHANDLER_H
#define LOGHANDLER_H

#include <QFile>
#include <QDir>
#include <QSharedPointer>
#include <QDateTime>
#include <QException>


#include "communications/sensordata.h"

enum class EventType: uint8_t {
    CommandSent = 0,
    SignalReceived,
    CommandFailed,
    SerialError,
    CorruptedData,
    Debug,
    Info,
    Warning,
    Error
};

class LogHandler
{
public:
    LogHandler(
        QString outputPath
    );
    ~LogHandler();
    bool initialize();
    static QString formatCountdown(int countdownMS);
    bool restartLogs();

    // While deferred, log rows are held in memory instead of being written to disk.
    // They are written out when restartLogs() is called (or on destruction).
    void beginDeferredWrites();

public slots:
    void logData(int countdownClockMS, const SensorData & data);
    void logEvent(int countdownClockMS, EventType eventType, const QString & message);
    void logCorruptedData(int countdownClockMS, const QByteArray & corruptedData);

protected:
    bool openFile(QFile & fileToOpen);

private:
    void writeOrDefer(QTextStream & stream, QStringList & pending, const QString & line);
    void flushDeferredWrites();
    void closeLogs();

    QString outputPath;
    QFile dataLog, eventLog, corruptionLog;
    QTextStream dataStream, eventStream, corruptionStream;

    bool deferWrites = false;
    QStringList pendingDataLines, pendingEventLines, pendingCorruptionLines;
};

#endif // LOGHANDLER_H
