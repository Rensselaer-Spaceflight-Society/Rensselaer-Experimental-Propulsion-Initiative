#include "serialworker.h"
#include "mainwindow.h"

SerialWorker::SerialWorker(MainWindow * window, QObject *parent)
    : mainWindow(window),
    serialPort(new QSerialPort(this)),
    dataBuffer(new QByteArray()),
    commandTimer(new QTimer(this))
{
    // TODO: Configure Serial Port to match the test stand config
    this->serialPort->setBaudRate(76800);
    this->serialPort->setParity(QSerialPort::Parity::NoParity);
    this->serialPort->setFlowControl(QSerialPort::FlowControl::NoFlowControl);
    this->serialPort->setDataBits(QSerialPort::DataBits::Data8);
    this->serialPort->setStopBits(QSerialPort::StopBits::OneStop);

    this->commandToSend.clear();

    connect(commandTimer, &QTimer::timeout, this, &SerialWorker::handleTimeout);
    connect(serialPort, &QSerialPort::readyRead, this, &SerialWorker::handleReadReady);
    connect(serialPort, &QSerialPort::errorOccurred, this, &SerialWorker::handleSerialError);

    commandTimer->start(COMMAND_SEND_TIMEOUT_DURRATION_MS);
}

SerialWorker::~SerialWorker()
{
    delete serialPort;
    delete dataBuffer;
    commandTimer->stop();
    delete commandTimer;
}

void SerialWorker::checksum12(void *checksum, const void *data, int n) {
    uint8_t* checksumPtr = (uint8_t *) checksum;
    const uint8_t* dataPtr = (uint8_t *) data;
    memset(checksumPtr, 0, 12);
    for (int i = 0; i < n; ++i) {
        checksumPtr[i % 12] ^= dataPtr[i];
        checksumPtr[(i + 1) % 12] ^= (dataPtr[i] >> 4) | (dataPtr[i] << 4);  // Simple mixing
    }
}

void SerialWorker::processSensorData(QByteArray && data)
{
    // 1. Compute the checksum of the incomming data
    QByteArray incommingDataChecksum(CHECKSUM_SIZE, 0);
    checksum12(incommingDataChecksum.data(), data.data(), sizeof(SensorData)-CHECKSUM_SIZE);

    // 2. Compare the computed checksum and the existing one
    const char * receivedChecksumPointer = dataBuffer->data() + sizeof(SensorData)-CHECKSUM_SIZE;
    if(!memcmp(incommingDataChecksum.data(), receivedChecksumPointer, CHECKSUM_SIZE))
    {
        // If the checksums match that means data wasn't modified in transit so we send to be graphed
        SensorData data = {0};
        memcpy(&data, dataBuffer->data(), sizeof(SensorData));
        emit dataAvailable(data);
    }
    else
    {
        // If the checksums don't match then we emit as corrupted data
        QByteArray corruptedDataBuffer = *dataBuffer;
        emit corruptedData(corruptedDataBuffer);
    }
}

void SerialWorker::onPortNameChange(const QSerialPortInfo &port)
{
    if(serialPort->isOpen())
        serialPort->close();

    serialPort -> setPort(port);

    if(serialPort->open(QIODevice::ReadWrite))
    {
        emit portOpenSuccess();
    }
    else
    {
        emit portOpenFailed();
    }
}

void SerialWorker::setStartPings(bool value)
{
    startPings = value;
}

void SerialWorker::issueCommand(const QString &command)
{
    if (command != commandToSend){
        commandRetries = 0;
    }

    commandToSend = command;
}

void SerialWorker::handleReadReady()
{
    qDebug() << "Read Ready: " << serialPort->bytesAvailable();

    dataBuffer->append(serialPort->readAll());

    while(dataBuffer->size() >= BYTES_IN_COMMAND) {
        // First we look for a data header to parse a data packet
        if (memcmp(dataBuffer->data(), EXPECTED_DATA_HEADER, BYTES_IN_COMMAND) == 0) {
            // Can we parse a full data packet?
            if (dataBuffer->size() >= sizeof(SensorData)) {
                processSensorData(dataBuffer->first(sizeof(SensorData)));
                dataBuffer->remove(0, sizeof(SensorData));
            } else {
                // Wait for full SensorData to arrive
                break;
            }
        }
        else {
            // Then we check for a command response from the engine
            bool matchedSignal = false;
            for (const auto & signal: EngineSignals) {
                if (memcmp(dataBuffer->data(), signal.toStdString().data(), BYTES_IN_COMMAND) == 0) {
                    // Emit that the signal was received, and then remove it from the buffer
                    emit signalReceived(signal);
                    dataBuffer->remove(0, BYTES_IN_COMMAND);
                    matchedSignal = true;
                    commandToSend.clear();
                    commandRetries = 0;
                    break;
                }
            }

            if (!matchedSignal) {
                // Unknown header, drop one byte and retry
                QByteArray corrupted = dataBuffer->left(BYTES_IN_COMMAND);
                emit corruptedData(corrupted);
                dataBuffer->remove(0, 1); // shift by one byte to resync
            }
        }
    }

}

void SerialWorker::handleSerialError(QSerialPort::SerialPortError error)
{
    if(error == QSerialPort::NoError) return;

    emit serialErrorOccurred(error, serialPort->errorString());
}

void SerialWorker::handleTimeout()
{
    // If the serial port is not open then don't try to write
    if(!(serialPort->isOpen())) return;

    // If we have a command to send then send that command
    if(!(commandToSend.isEmpty()))
    {
        // But if we have exceeded the max command attempts then say this command failed
        if(commandRetries >= MAX_COMMAND_RETRIES)
        {
            commandRetries = 0;
            emit commandFailed(commandToSend);
            commandToSend.clear();
            return;
        }

        qDebug() << commandToSend.toUtf8();

        serialPort->write(commandToSend.toUtf8());
        serialPort->flush();
        commandRetries++;
        emit commandAttempt(commandToSend);
    }

    // Don't start sending pings until startPings is set to be true
    if(!startPings) return;

    serialPort->write(PING_COMMAND);
    serialPort->flush();
    emit commandAttempt(PING_COMMAND);
}

