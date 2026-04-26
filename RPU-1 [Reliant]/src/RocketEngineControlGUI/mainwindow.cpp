#include <QtCharts/QtCharts>
#include <QSerialPort>

#include "./ui_mainwindow.h"
#include "mainwindow.h"
#define DATA_OUTPUT_PATH "../../../../data"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
    logger(DATA_OUTPUT_PATH),
    userAlert(new AlertDialog()),
    commsCenter(new SerialWorker(this)),
    commsThread(new QThread(this)),
    countdown(new QTimer()),
    pingCheck(new QTimer()),
    telemetry(new QTimer())
{
    ui->setupUi(this);


    commsCenter->moveToThread(commsThread);
    this->setupConnections();
    commsThread->start();
    handleSerialPortRefresh();

    // Disable the Abort and Countdown Button until a connection is established
    ui->AbortButton->setDisabled(true);
    ui->StartCountdown->setDisabled(true);
    this->configureCharts();
}

MainWindow::~MainWindow()
{
    if (commsThread) {
        commsThread->quit();
        commsThread->wait();

        delete commsCenter;   // only if you didn't set a parent
        delete commsThread;
    }
    delete userAlert;
    delete countdown;
    delete pingCheck;
    delete ui;
}

void MainWindow::keyPressEvent(QKeyEvent* keyEvent)
{
    if(keyEvent->key() == Qt::Key_Backspace
        && this->ui->AbortButton->isEnabled())
    {
        this->handleShutdown();
    }

    QMainWindow::keyPressEvent(keyEvent);
}

void MainWindow::handleSerialPortRefresh()
{
    ui->AbortButton->setDisabled(true);
    ui->StartCountdown->setText("Start Countdown");
    countdownMs = COUNTDOWN_LENGTH_MS;
    pastAutoHold = false;
    countdown->stop();
    pingCheck->stop();
    ui->CountdowLabel->setText(LogHandler::formatCountdown(countdownMs));
    ui->SerialPortDropdown->clear();
    ui->SerialPortDropdown->addItem("Select a Serial Port");
    ui->SerialPortDropdown->addItems(this->getSerialPorts());
}

void MainWindow::handleStartCountdown()
{
    // Handle the case of when this is a reconnection button
    if(currentState == EngineStates::NO_CONNECTION)
    {
        emit setPings(false);
        emit issueCommand(CONTROL_ACTIVE_COMMAND);
        return;
    }

    // Handle the hold continue
    if(currentState == EngineStates::HOLDING)
    {
        ui->StartCountdown->setDisabled(true);
        ui->AbortButton->setText("Hold Countdown");
        currentState = beforeHoldState;
        return;
    }

    // Allow for reset if we are in shutdown confirmed state
    if(currentState == EngineStates::SHUTDOWN_COMPLETE)
    {
        currentState = EngineStates::CONNECTION_ESTABLISHED;
        countdownMs = COUNTDOWN_LENGTH_MS;
        pastAutoHold = false;
        ui->AbortButton->setDisabled(true);
        ui->StartCountdown->setEnabled(true);
        ui->StartCountdown->setText("Start Countdown");
        ui->CountdowLabel->setText(LogHandler::formatCountdown(countdownMs));
        resetCharts();
        timeSinceLogStart = 0;
        if(!logger.restartLogs())
        {
            userAlert->setAlertTitle("The logger failed to restart the logs");
            userAlert->setAlertDescription("The logger failed to reset the logs, please check file permissions.");
            userAlert->show();
        }
        return;
    }

    // Set the burn duration from the text input
    bool doubleConversionOk;
    QString burnDurationString = ui->BurnDurationInput->text();
    double burnDuration = burnDurationString.toDouble(&doubleConversionOk);

    if(!doubleConversionOk) {
        userAlert->setAlertTitle("Invalid Burn Duration");
        userAlert->setAlertDescription("Unable to parse the burn duration into a number, please check input. ");
        userAlert->show();
        return;
    }

    burnDurationMs = (int) (burnDuration*1000);

    if(burnDuration <= 0) {
        userAlert->setAlertTitle("Invalid Burn Duration");
        userAlert->setAlertDescription("Burn duration cannot be negative. ");
        userAlert->show();
        return;
    }

    // Issue the log start command to start logs
    emit issueCommand(LOG_START_COMMAND);
    emit setPings(true);
}

void MainWindow::handleShutdown()
{

    // If we are pre-ignition then the shutdown button acts as a countdown hold
    if(currentState < EngineStates::HOLDING && currentState != EngineStates::NO_CONNECTION)
    {
        beforeHoldState = currentState; // Know which state we should return to after the hold is cleared
        currentState = EngineStates::HOLDING;
        ui->AbortButton->setText("Shutdown Engine");
        ui->StartCountdown->setText("Continue Countdown");
        ui->StartCountdown->setEnabled(true);
        return;
    }

    // If we are in a hold or ignition state and the shutdown button is clicked then we should
    // transition into a shutdown state
    if(currentState != EngineStates::NO_CONNECTION)
        currentState = EngineStates::PENDING_SHUTDOWN;

    emit issueCommand(SHUTDOWN_COMMAND);
    ui->EngineStatus->setText("Pending Shutdown");
    ui->AbortButton->setText("Pending Shutdown");

}

void MainWindow::handleSerialPortSelection(int index)
{
    // Adjust the index by -1 to account for the "Select a Serial Port Option"
    if(index-1 < 0) return;
    emit serialPortChanged(availableSerialPorts[index-1]);
}

void MainWindow::handleCommandAttempt(const QString & command)
{
    // Ignore pings since they are sent so frequently
    if (command == PING_COMMAND) return;
    // Log the attempt since the serial worker should handle repetition
    logger.logEvent(countdownMs, EventType::CommandSent, command + " Sent to Motor");
}

void MainWindow::handleCommandFailed(const QString & command)
{
    // Log the failure in comms
    logger.logEvent(
        countdownMs,
        EventType::CommandFailed,
        command + " was sent "
            + QString::number(MAX_COMMAND_RETRIES)
            + " times without acknowledgement."
        );

    // Alert the user
    userAlert->setAlertTitle("Communications Lost");
    userAlert->setAlertDescription(
        "The command: "
        + command
        + " has failed to be sent and acknowledged by the test stand."
        + " Please check the communications hardware and try again. "
        + " Engine shutdown has been started. "
        );
    userAlert->show();

    currentState = EngineStates::NO_CONNECTION;

    // If the Shutdown command has failed then we must assume that the engine's firmware
    // has kicked in and shut the engine down.
    if (command == SHUTDOWN_COMMAND) return;
    // Try to shutdown assuming the connection from the rocket is the one that is severed
    handleShutdown();
    pingCheck->stop();
    countdown->stop();
    telemetry->stop();
    emit setPings(false);
    ui->AbortButton->setDisabled(true);
    ui->StartCountdown->setText("Attempt Reconnection");
}

void MainWindow::hanldleSignalReceived(const QString & signal)
{
    okBitsRecieved += BYTES_IN_COMMAND * 10;


    timeSinceLastPing = 0; // All command responses indicate that two way comms are still active
    if(signal == PING_COMMAND) {
        pingsReceived++;
        return;
    } // If the signal is a ping then we have already done all that we need to

    // We also shouldn't log pings since they happen so frequently.
    logger.logEvent(countdownMs, EventType::SignalReceived, signal + " was received. ");

    // qDebug() << "Current State: " << (int) currentState << ", Signal: " << signal;

    if(signal == INVALID_COMMAND)
    {
        logger.logEvent(countdownMs, EventType::Warning, "The engine indicated that it received an invalid command.");
        invalidCommandReceived++;
        return;
    }


    if(signal == CONNECTION_LOST)
    {
        logger.logEvent(countdownMs, EventType::Error, "The engine has indicated that it has lost connection. Automatic Shutdown Occurred.");
        currentState = EngineStates::NO_CONNECTION;
        userAlert->setAlertTitle("Connection Lost");
        userAlert->setAlertDescription("The engine has indicated it has lost connection. Automatic Shutdown has started. Please check comms hardware.");
        userAlert->show();
        countdown->stop();
        pingCheck->stop();
        telemetry->stop();
        emit setPings(false);
        ui->AbortButton->setEnabled(false);
        ui->StartCountdown->setText("Attempt Reconnection");
        ui->StartCountdown->setEnabled(true);
        ui->EngineStatus->setText("No Connection");
        return;
    }

    // We ignore no connection because if we ever are in the no connection state after
    // startup then the engine should automatically perform shutdown, and we want to
    // stay in the no connection state
    if(signal == SHUTDOWN_COMMAND && currentState != EngineStates::NO_CONNECTION)
    {
        logger.logEvent(countdownMs, EventType::Info, "Engine Shutdown Started");
        currentState = EngineStates::SHUTDOWN_STARTED;
        ui->AbortButton->setText("Shutdown Started");
        ui->StartCountdown->setDisabled(true);
        ui->EngineStatus->setText("Shutdown Started");
        return;
    }

    if(currentState == EngineStates::HOLDING)
    {
        if(signal == INERT_FLUSH_FINISHED)
        {
            beforeHoldState = EngineStates::NITROGEN_FLUSH_DONE;
            ui->EngineStatus->setText("Inert Flush Finished");
            return;
        }
    }

    if(currentState == EngineStates::NO_CONNECTION)
    {
        if(signal == CONTROL_ACTIVE_COMMAND)
        {
            currentState = EngineStates::CONNECTION_ESTABLISHED;
            emit setPings(true);
            pingCheck->start(EVENT_POLL_DURATION_MS);
            ui->StartCountdown->setEnabled(true);
            ui->StartCountdown->setText("Start Countdown");
            ui->AbortButton->setDisabled(true);
            ui->EngineStatus->setText("Connection Established");
            countdownMs = COUNTDOWN_LENGTH_MS;
            pastAutoHold = false;
            ui->StartCountdown->setEnabled(true);
            ui->CountdowLabel->setText(LogHandler::formatCountdown(countdownMs));
            resetCharts();
            telemetry->start(1000);
            timeSinceLogStart = 0;
            if(!logger.restartLogs())
            {
                userAlert->setAlertTitle("The logger failed to restart the logs");
                userAlert->setAlertDescription("The logger failed to reset the logs, please check file permissions.");
                userAlert->show();
            }
            return;
        }
    }

    if(currentState == EngineStates::CONNECTION_ESTABLISHED)
    {
        if(signal == LOG_START_COMMAND)
        {
            currentState = EngineStates::COUNTDOWN_STARTED;
            countdown->start(EVENT_POLL_DURATION_MS);
            ui->StartCountdown->setDisabled(true);
            ui->AbortButton->setEnabled(true);
            ui->AbortButton->setText("Hold Countdown");
            ui->EngineStatus->setText("Logging Started");
            return;
        }
    }

    if(currentState == EngineStates::COUNTDOWN_STARTED)
    {
        if(signal == PRESURIZE_FUEL_COMMAND)
        {
            currentState = EngineStates::PRESSUREIZED_FUEL;
            ui->EngineStatus->setText("Fuel Pressurized");
            return;
        }
    }

    if(currentState == EngineStates::PRESSUREIZED_FUEL)
    {
        if(signal == INERT_GAS_FLUSH_COMMAND)
        {
            currentState = EngineStates::NITROGEN_FLUSH_STARTED;
            ui->EngineStatus->setText("Inert Flush Started");
            return;
        }
    }

    if(currentState == EngineStates::NITROGEN_FLUSH_STARTED)
    {
        if(signal == INERT_FLUSH_FINISHED)
        {
            currentState = EngineStates::NITROGEN_FLUSH_DONE;
            ui->EngineStatus->setText("Inert Flush Finished");
            return;
        }
    }



    if(currentState == EngineStates::NITROGEN_FLUSH_DONE)
    {
        if(signal == IGNITION_COMMAND)
        {
            currentState = EngineStates::IGNITION;
            ui->EngineStatus->setText("Engine Ignition");
            ui->AbortButton->setText("Shutdown Engine");
            return;
        }
    }

    if(currentState == EngineStates::SHUTDOWN_STARTED || currentState == EngineStates::PENDING_SHUTDOWN)
    {
        if(signal == SHUTDOWN_CONFIRMED)
        {
            logger.logEvent(countdownMs, EventType::Info, "Engine Shutdown Completed");
            currentState = EngineStates::SHUTDOWN_COMPLETE;
            ui->AbortButton->setText("Shutdown Complete");
            ui->AbortButton->setDisabled(true);
            ui->EngineStatus->setText("Shutdown Complete");
            ui->StartCountdown->setText("Reset Test");
            countdown->stop();
            ui->StartCountdown->setEnabled(true);
            return;
        }
    }
}

void MainWindow::handleCountdownUpdate()
{
    timeSinceLogStart += (float) EVENT_POLL_DURATION_MS / 1000;

    // If we are holding then we shouldn't be updating the countdown;
    if(currentState == EngineStates::HOLDING) {
        return;
    }

    countdownMs += EVENT_POLL_DURATION_MS;


    QString countdownTimerText = LogHandler::formatCountdown(countdownMs);
    ui->CountdowLabel->setText(countdownTimerText);

    // Handle Fuel Pressurization
    if(currentState == EngineStates::COUNTDOWN_STARTED && countdownMs > PRESURIZE_FUEL_POINT_MS)
    {
        emit issueCommand(PRESURIZE_FUEL_COMMAND);
        return;
    }

    if(currentState == EngineStates::PRESSUREIZED_FUEL && countdownMs > AUTO_HOLD_POINT_MS && !pastAutoHold)
    {
        beforeHoldState = currentState;
        currentState = EngineStates::HOLDING;
        ui->AbortButton->setText("Shutdown Engine");
        ui->StartCountdown->setText("Continue Countdown");
        ui->StartCountdown->setEnabled(true);
        pastAutoHold = true;
        return;
    }


    // Handle the Inert Flush When we reach that point
    if(currentState == EngineStates::PRESSUREIZED_FUEL && countdownMs > INERT_FLUSH_POINT_MS)
    {
        emit issueCommand(INERT_GAS_FLUSH_COMMAND);
        return;
    }

    // Handle Ignition after Inert Flush
    if(currentState == EngineStates::NITROGEN_FLUSH_DONE && countdownMs > IGNITION_START_POINT_MS)
    {
        emit issueCommand(IGNITION_COMMAND);
        return;
    }

    // Handle Shutdown After We Have Burned for the Full Duration
    if((currentState == EngineStates::IGNITION && countdownMs > burnDurationMs) || currentState == EngineStates::PENDING_SHUTDOWN)
    {
        handleShutdown();
        return;
    }
}

void MainWindow::handlePingCheck()
{
    timeSinceLastPing+=EVENT_POLL_DURATION_MS;


    if(timeSinceLastPing < MAX_PING_NON_RESPONSE_DELAY_MS || currentState < EngineStates::CONNECTION_ESTABLISHED) return;

    currentState = EngineStates::NO_CONNECTION;
    handleShutdown();

    ui->EngineStatus->setText("Connection Lost");

    userAlert->setAlertTitle("Connection Lost");
    userAlert->setAlertDescription("Engine has failed to respond to pings, please check hardware.");
    userAlert->show();

    pingCheck->stop();
    countdown->stop();
    emit setPings(false);

    ui->StartCountdown->setText("Attempt Reconnection");
    ui->AbortButton->setText("Hold Countdown");
    ui->AbortButton->setDisabled(true);
    ui->StartCountdown->setEnabled(true);
}

void MainWindow::handleDataAvailable(const SensorData & data)
{
    // Data also can act as a ping, indicating an intact connection
    timeSinceLastPing = 0;
    // Log the data in the data log
    logger.logData(countdownMs, data);

    totalDataPacketsReceived++;
    packetsReceived++;

    // 10 bits for the byte, start, and stop bit
    okBitsRecieved += sizeof(SensorData) * 10;

    if (totalDataPacketsReceived % 25 != 0) return;
    // Update the graphs and the data table
    this->updateUIWithSensorData(data);
}

void MainWindow::handleCorruptedData(const QByteArray & data)
{
    qDebug() << "Corrupted Data: " << data;
    // Log the corrupted data event
    logger.logEvent(
        countdownMs,
        EventType::CorruptedData,
        QString::number(data.size())+ " Bytes of Corrupted Data recieved and logged. "
        );

    if (data.size() == 8){
        // We send 8 bytes as corrupted as context, but only so far have we shown that
        // 10 bits (1 byte + start bit + stop bit) are corrupted.
        corruptedDataBitsRecieved += 10;
    }else{
        // Otherwise we have a data packet that failed the checksum
        corruptedDataBitsRecieved += sizeof(SensorData) * 10;
    }

    // Log the corrupted data
    logger.logCorruptedData(countdownMs, data);
}

void MainWindow::handlePortOpenFailed()
{
    // Log the event
    logger.logEvent(
        countdownMs,
        EventType::SerialError,
        "Serial Port Failed to Open."
        );

    // Update the screen
    this->ui->EngineStatus->setText("Serial Port Failed to Open");
}

void MainWindow::handleSerialError(QSerialPort::SerialPortError error, const QString & errorStr)
{
    logger.logEvent(countdownMs, EventType::SerialError, QString::number(error) + ": " + errorStr);

    if(error == 0) return;

    userAlert->setAlertTitle("Serial Error Occurred Code: " + QString::number(error));
    userAlert->setAlertDescription("A serial error occurred: " + errorStr + " Please check that the serial hardware and that there is not any other programs using the serial port.");
    userAlert->show();
}

void MainWindow::handlePortOpenSuccess()
{
    logger.logEvent(
        countdownMs,
        EventType::Info,
        "Serial Port opened Successfully"
        );

    // Update the screen

    this->ui->EngineStatus->setText("Serial Port Opened");

    // Try to connect to the engine
    emit issueCommand(CONTROL_ACTIVE_COMMAND);
}

void MainWindow::handleTelemetryUpdate()
{
    float dataRateKbps = (float) (this->okBitsRecieved + this->corruptedDataBitsRecieved) / 1000.0;
    ui->DataRateValue->setText(QString("In Data Rate (Kbps): %1").arg(dataRateKbps, 0, 'f', 1, '0'));
    ui->InvalidCommandRateValue->setText(QString("Invalid Command Rate (Hz): %1").arg(invalidCommandReceived));
    ui->CorruptedDataRateValue->setText(QString("Corrupted Data Rate (bps): %1").arg(corruptedDataBitsRecieved));
    ui->PacketRateValue->setText(QString("Packet Rate (Hz): %1").arg(packetsReceived));
    ui->PingRateValue->setText(QString("Ping Rate (Hz): %1").arg(pingsReceived));

    okBitsRecieved = 0;
    corruptedDataBitsRecieved = 0;
    invalidCommandReceived = 0;
    packetsReceived = 0;
    pingsReceived = 0;
}

QStringList MainWindow::getSerialPorts()
{

    QStringList portDropdownOptions;
    this->availableSerialPorts = QSerialPortInfo::availablePorts();
    for(auto const & port: this->availableSerialPorts)
    {
        if(port.hasVendorIdentifier())
        {
            portDropdownOptions.append(port.portName() + ": " + port.manufacturer());
        }
    }
    return portDropdownOptions;
}

void MainWindow::updateUIWithSensorData(const SensorData & data)
{
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

    // TODO: Add Coloring to the Labels for Values out of spec

    // Load Cell
    this->ui->LoadCellValue->setText(QString("%1 N").arg(data.loadCell, 5, 'f', 2, QChar('0')));
    this->ui->LoadCellChart->append(timeSinceLogStart, data.loadCell);

    // Kerosene Inlet
    this->ui->FuelInletTempValue->setText(QString("%1 C").arg(data.thermocouple[0], 5, 'f', 2, QChar('0')));
    this->ui->FuelInletChart->append(timeSinceLogStart, data.thermocouple[0]);

    // Oxidizer Inlet
    this->ui->OxidizerInletTempValue->setText(QString("%1 C").arg(data.thermocouple[1], 5, 'f', 2, QChar('0')));
    this->ui->OxidizerInletChart->append(timeSinceLogStart, data.thermocouple[1]);

    // Engine Throat
    this->ui->EngineThroatTempValue->setText(QString("%1 C").arg(data.thermocouple[2], 5, 'f', 2, QChar('0')));
    this->ui->EngineThroatChart->append(timeSinceLogStart, data.thermocouple[2]);

    // Nozzle Near Exit
    this->ui->NozzleExitTempValue->setText(QString("%1 C").arg(data.thermocouple[3], 5, 'f', 2, QChar('0')));
    this->ui->NozzleExitChart->append(timeSinceLogStart, data.thermocouple[3]);

    // Fuel Feed Line Pressure
    this->ui->FuelFeedPressureValue->setText(QString("%1 PSI").arg(data.pressureTransducer[1], 5, 'f', 2, QChar('0')));
    this->ui->FuelFeedPressureChart->append(timeSinceLogStart, data.pressureTransducer[1]);

    // Kerosene Tank Pressure
    this->ui->KeroseneTankPressureValue->setText(QString("%1 PSI").arg(data.pressureTransducer[2], 5, 'f', 2, QChar('0')));
    this->ui->FuelTankPressureChart->append(timeSinceLogStart, data.pressureTransducer[2]);

    // Kerosene Line Pressure
    this->ui->FuelLinePressureValue->setText(QString("%1 PSI").arg(data.pressureTransducer[3], 5, 'f', 2, QChar('0')));
    this->ui->FuelLinePressureChart->append(timeSinceLogStart, data.pressureTransducer[3]);

    // Oxidizer Tank Pressure
    this->ui->OxidizerTankPressureValue->setText(QString("%1 PSI").arg(data.pressureTransducer[4], 5, 'f', 2, QChar('0')));
    this->ui->OxidizerTankPressureChart->append(timeSinceLogStart, data.pressureTransducer[4]);

    // Oxidizer Line pressure
    this->ui->OxidizerLinePressureValue->setText(QString("%1 PSI").arg(data.pressureTransducer[5], 5, 'f', 2, QChar('0')));
    this->ui->OxidizerLinePressureChart->append(timeSinceLogStart, data.pressureTransducer[5]);
}

void MainWindow::configureCharts()
{
    ui->LoadCellChart->setChartTitle("Load Cell");
    ui->LoadCellChart->setChartType(ChartType::Force);

    ui->FuelInletChart->setChartTitle("Fuel Inlet Temp");
    ui->FuelInletChart->setChartType(ChartType::Temperature);
    ui->OxidizerInletChart->setChartTitle("Oxidizer Inlet Temp");
    ui->OxidizerInletChart->setChartType(ChartType::Temperature);

    ui->EngineThroatChart->setChartTitle("Throat Temperature");
    ui->EngineThroatChart->setChartType(ChartType::Temperature);
    ui->NozzleExitChart->setChartTitle("Nozzle Exit Temperature");
    ui->NozzleExitChart->setChartType(ChartType::Temperature);

    ui->OxidizerTankPressureChart->setChartTitle("Oxidizer Tank Pressure");
    ui->OxidizerTankPressureChart->setChartType(ChartType::Pressure);
    ui->OxidizerLinePressureChart->setChartTitle("Oxidizer Line Pressure");
    ui->OxidizerLinePressureChart->setChartType(ChartType::Pressure);
    ui->FuelTankPressureChart->setChartTitle("Fuel Tank Pressure");
    ui->FuelTankPressureChart->setChartType(ChartType::Pressure);
    ui->FuelLinePressureChart->setChartTitle("Fuel Line Pressure");
    ui->FuelLinePressureChart->setChartType(ChartType::Pressure);
    ui->FuelFeedPressureChart->setChartTitle("Kerosene Feed Line Pressure");
    ui->FuelFeedPressureChart->setChartType(ChartType::Pressure);
}

void MainWindow::resetCharts()
{
    ui->LoadCellChart->reset();
    ui->FuelInletChart->reset();
    ui->OxidizerInletChart->reset();
    ui->EngineThroatChart->reset();
    ui->NozzleExitChart->reset();
    ui->OxidizerTankPressureChart->reset();
    ui->OxidizerLinePressureChart->reset();
    ui->FuelTankPressureChart->reset();
    ui->FuelLinePressureChart->reset();
    ui->FuelFeedPressureChart->reset();
}

void MainWindow::setupConnections()
{
    // Connections from MainWindow to SerialWorker
    connect(this, &MainWindow::issueCommand, commsCenter, &SerialWorker::issueCommand, Qt::QueuedConnection);
    connect(this, &MainWindow::serialPortChanged, commsCenter, &SerialWorker::onPortNameChange, Qt::QueuedConnection);
    connect(this, &MainWindow::setPings, commsCenter, &SerialWorker::setStartPings, Qt::QueuedConnection);

    // Connecting Timers
    connect(countdown, &QTimer::timeout, this, &MainWindow::handleCountdownUpdate);
    connect(pingCheck, &QTimer::timeout, this, &MainWindow::handlePingCheck);
    connect(telemetry, &QTimer::timeout, this, &MainWindow::handleTelemetryUpdate);

    // Connecting UI
    connect(ui->StartCountdown, &QPushButton::clicked, this, &MainWindow::handleStartCountdown);
    connect(ui->AbortButton, &QPushButton::clicked, this, &MainWindow::handleShutdown);
    connect(ui->RefreshSerialPorts, &QPushButton::clicked, this, &MainWindow::handleSerialPortRefresh);
    connect(ui->SerialPortDropdown, &QComboBox::currentIndexChanged, this, &MainWindow::handleSerialPortSelection);

    // Connecting SerialWorker to MainWindow
    connect(commsCenter, &SerialWorker::serialErrorOccurred, this, &MainWindow::handleSerialError, Qt::QueuedConnection);
    connect(commsCenter, &SerialWorker::commandAttempt, this, &MainWindow::handleCommandAttempt, Qt::QueuedConnection);
    connect(commsCenter, &SerialWorker::commandFailed, this, &MainWindow::handleCommandFailed, Qt::QueuedConnection);
    connect(commsCenter, &SerialWorker::portOpenFailed, this, &MainWindow::handlePortOpenFailed, Qt::QueuedConnection);
    connect(commsCenter, &SerialWorker::portOpenSuccess, this, &MainWindow::handlePortOpenSuccess, Qt::QueuedConnection);
    connect(commsCenter, &SerialWorker::corruptedData, this, &MainWindow::handleCorruptedData, Qt::QueuedConnection);
    connect(commsCenter, &SerialWorker::signalReceived, this, &MainWindow::hanldleSignalReceived, Qt::QueuedConnection);
    connect(commsCenter, &SerialWorker::dataAvailable, this, &MainWindow::handleDataAvailable, Qt::QueuedConnection);
}
