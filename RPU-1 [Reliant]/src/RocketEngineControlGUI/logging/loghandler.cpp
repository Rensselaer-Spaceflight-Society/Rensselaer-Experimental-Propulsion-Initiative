#include "loghandler.h"

LogHandler::LogHandler(
    QString outputPath
)
{
    this->outputPath = outputPath;
}

LogHandler::~LogHandler()
{
    // Don't lose any rows that were held back waiting for a reset
    flushDeferredWrites();
    closeLogs();
}

bool LogHandler::initialize()
{
    QString datetime = QDateTime::currentDateTime().toString("yyyy-MM-dd-HH:mm:ss");
    QString formattedOutputPath = outputPath+"/%1";
    formattedOutputPath = formattedOutputPath.arg(datetime);
    QString dataFileName = QString("sensor-data.csv");
    QString eventFileName = QString("event-log.csv");
    QString corruptedDataFileName = QString("corrupted-data.csv");

    // Make the folder if it doesn't already exist

    QDir dir(formattedOutputPath);
    if (!dir.exists()) {
        dir.mkpath(".");  // Create the directory and any necessary parent directories
    }

    QString dataFilePath = formattedOutputPath + "/" + dataFileName;
    QString eventFilePath = formattedOutputPath + "/" + eventFileName;
    QString corruptedDataFilePath = formattedOutputPath + "/" + corruptedDataFileName;

    dataLog.setFileName(dataFilePath);
    eventLog.setFileName(eventFilePath);
    corruptionLog.setFileName(corruptedDataFilePath);

    bool fileOpened = openFile(this->dataLog);

    if(!fileOpened)
    {
        qDebug() << "The sensor data log failed to open for writing";
        return false;
    }

    fileOpened = openFile(this->eventLog);

    if(!fileOpened)
    {
        qDebug() << "The event log file failed to open for writing";
        return false;
    }

    fileOpened = openFile(this->corruptionLog);

    if(!fileOpened)
    {
        qDebug() << "The corrupted data log file failed to open for writting";
        return false;
    }

    dataStream.setDevice(&dataLog);
    eventStream.setDevice(&eventLog);
    corruptionStream.setDevice(&corruptionLog);

    // Setup Log File headers

    dataStream << "UNIX Time (ms), "<< "Local Time, " << "Countdown Time, " << "Load Cell (N), "
               << "Kerosene Inlet (c), " << "Oxidizer Inlet (c), "
               << "Throat (c), " << "Nozzle Outlet (c), "
               << "Combustion Chamber (PSI), " << "Kerosene Feed Line (PSI), "
               << "Kerosene Tank (PSI), " << "Kerosene Line (PSI), "
               << "Oxidizer Tank (PSI), " << "Oxidizer Line (PSI)"
               << "\n";

    eventStream << "UNIX Time (ms), " << "Local Time, " << "Countdown Time, " << "Event Type, "
                << "Message " << "\n";

    corruptionStream << "UNIX Time (ms), " << "Local Time, " << "Countdown Time, " << "Byte Length, "
                     << "Data" << "\n";

    return true;
}

QString LogHandler::formatCountdown(int countdownMS)
{
    // Determine if it's T- or T+
    QString prefix = (countdownMS < 0) ? "T-" : "T+";

    // Get absolute value for formatting
    qint64 absMilliseconds = std::abs(countdownMS);

    // Calculate minutes, seconds, and milliseconds
    qint64 totalSeconds = absMilliseconds / 1000;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    int remainingMilliseconds = absMilliseconds % 1000;

    // Format the output with zero-padding
    return QString("%1%2:%3.%4")
        .arg(prefix)
        .arg(minutes, 2, 10, QChar('0'))        // 2 digits for minutes, zero-padded
        .arg(seconds, 2, 10, QChar('0'))        // 2 digits for seconds, zero-padded
        .arg(remainingMilliseconds, 3, 10, QChar('0')); // 3 digits for milliseconds, zero-padded
}

bool LogHandler::restartLogs()
{
    // Write out anything held back during the shutdown-complete period into the
    // current log files before they are closed and replaced
    flushDeferredWrites();
    closeLogs();

    return initialize();
}

void LogHandler::beginDeferredWrites()
{
    deferWrites = true;
}

void LogHandler::closeLogs()
{
    if(dataLog.isOpen())
    {
        dataStream.flush();
        dataLog.close();
    }
    if(eventLog.isOpen())
    {
        eventStream.flush();
        eventLog.close();
    }
    if(corruptionLog.isOpen())
    {
        corruptionStream.flush();
        corruptionLog.close();
    }
}

void LogHandler::flushDeferredWrites()
{
    deferWrites = false;

    if(dataLog.isOpen())
    {
        for(const QString & line: pendingDataLines) dataStream << line;
        dataStream.flush();
    }
    if(eventLog.isOpen())
    {
        for(const QString & line: pendingEventLines) eventStream << line;
        eventStream.flush();
    }
    if(corruptionLog.isOpen())
    {
        for(const QString & line: pendingCorruptionLines) corruptionStream << line;
        corruptionStream.flush();
    }

    pendingDataLines.clear();
    pendingEventLines.clear();
    pendingCorruptionLines.clear();
}

void LogHandler::writeOrDefer(QTextStream & stream, QStringList & pending, const QString & line)
{
    if(deferWrites)
    {
        pending.append(line);
    }
    else
    {
        stream << line;
    }
}

void LogHandler::logData(int countdownClockMS, const SensorData & data)
{
    qint64 unixTime = QDateTime::currentMSecsSinceEpoch();
    QString localTime = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString countdownTime = formatCountdown(countdownClockMS);

    QString line;
    QTextStream lineStream(&line);
    lineStream << unixTime << ", " << localTime << ", " << countdownTime << ", " << data.loadCell
               << ", " << data.thermocouple[0] << ", " << data.thermocouple[1]
               << ", " << data.thermocouple[2] << ", " << data.thermocouple[3]
               << ", " << data.pressureTransducer[0] << ", " << data.pressureTransducer[1]
               << ", " << data.pressureTransducer[2] << ", " << data.pressureTransducer[3]
               << ", " << data.pressureTransducer[4] << ", " << data.pressureTransducer[5]
               << "\n";
    lineStream.flush();

    writeOrDefer(dataStream, pendingDataLines, line);
}

void LogHandler::logEvent(int countdownClockMS, EventType eventType, const QString & message)
{
    qint64 unixTime = QDateTime::currentMSecsSinceEpoch();
    QString localTime = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString countdownTime = formatCountdown(countdownClockMS);
    QString messageType;

    switch(eventType){
        case EventType::SignalReceived:
            messageType = "[SIGNAL] ";
            break;
        case EventType::CommandFailed:
            messageType = "[COMMAND FAIL] ";
            break;
        case EventType::CommandSent:
            messageType = "[COMMAND SENT] ";
            break;
        case EventType::SerialError:
            messageType = "[SERIAL ERROR] ";
            break;
        case EventType::CorruptedData:
            messageType = "[CORRUPTED DATA] ";
            break;
        case EventType::Debug:
            messageType = "[DEBUG] ";
            break;
        case EventType::Info:
            messageType = "[INFO] ";
            break;
        case EventType::Warning:
            messageType = "[WARNING] ";
            break;
        case EventType::Error:
            messageType = "[ERROR] ";
            break;
    }

    QString line;
    QTextStream lineStream(&line);
    lineStream << unixTime << ", " << localTime << ", " << countdownTime << ", "
               << messageType << ", " << message << "\n";
    lineStream.flush();

    writeOrDefer(eventStream, pendingEventLines, line);
}

void LogHandler::logCorruptedData(int countdownClockMS, const QByteArray & corruptedData)
{
    qint64 unixTime = QDateTime::currentMSecsSinceEpoch();
    QString localTime = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString countdownTime = formatCountdown(countdownClockMS);

    QString line;
    QTextStream lineStream(&line);
    lineStream << unixTime << ", " << localTime << ", " << countdownTime << ", "
               << (corruptedData).size() << ", " << (corruptedData).toHex() << "\n";
    lineStream.flush();

    writeOrDefer(corruptionStream, pendingCorruptionLines, line);
}

bool LogHandler::openFile(QFile & fileToOpen)
{
    return fileToOpen.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}
