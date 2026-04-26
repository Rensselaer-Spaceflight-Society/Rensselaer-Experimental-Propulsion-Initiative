#include "datachart.h"
#include "ui_datachart.h"

#define CHART_RANGE_SECS 10

DataChart::DataChart(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DataChart)
    , minXValue((double) COUNTDOWN_LENGTH_MS / 1000)
    , maxXValue((double) COUNTDOWN_LENGTH_MS / 1000)
    , maxYValue(1.0)
{
    ui->setupUi(this);

    // Create chart elements
    this->chart = new QChart();
    this->series = new QAreaSeries();
    this->lowerSeries = new QLineSeries();
    this->upperSeries = new QLineSeries();
    this->axisX = new QValueAxis();
    this->axisY = new QValueAxis();

    // Configure axes
    axisX->setLabelFormat("%.2f");
    axisX->setTickCount(5);
    axisX->setRange(minXValue, maxXValue);
    axisX->setTitleBrush(QColor(0xffefefef));
    axisX->setLabelsColor(QColor(0xffefefef));
    axisX->setTitleText("Time (s)");

    axisY->setLabelFormat("%.2f");
    axisY->setTickCount(5);
    axisY->setRange(0, maxYValue);
    axisY->setTitleBrush(QColor(0xffefefef));
    axisY->setLabelsColor(QColor(0xffefefef));

    // Configure series
    series->setLowerSeries(lowerSeries);
    series->setUpperSeries(upperSeries);
    series->setName("Data");

    // Add series and axes to chart (order matters)
    chart->addSeries(series);
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    // Attach series to axes AFTER adding both to chart
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    // Chart appearance
    chart->setBackgroundBrush(QColor(0,0,0,0));
    chart->setTitleBrush(QColor(0xffefefef));
    chart->legend()->setVisible(false);
    chart->setMargins(QMargins(0,4,0,0));

    ui->ChartView->setChart(chart);
}

void DataChart::setChartTitle(const QString& title)
{
    chart->setTitle(title);
}

void DataChart::setChartType(ChartType chartType)
{
    if(chartType == ChartType::Force)
    {
        series->setColor(QColor(164, 215, 166, 64));
        series->setBorderColor(QColor(114, 147, 115));
        axisY->setTitleText("Force (N)");
    }
    else if(chartType == ChartType::Pressure)
    {
        series->setColor(QColor(101, 179, 242, 64));
        series->setBorderColor(QColor(73, 132, 178));
        axisY->setTitleText("Pressure (kPa)");
    }
    else
    {
        series->setColor(QColor(197, 73, 35, 64));
        series->setBorderColor(QColor(137, 46, 19));
        axisY->setTitleText("Temp (C)");
    }
}

void DataChart::append(float xValue, float yValue)
{
    // Update range values
    maxXValue = std::fmax(maxXValue, xValue);
    minXValue = std::fmin(minXValue, xValue);
    maxYValue = std::fmax(maxYValue, yValue);

    // Add data points
    lowerSeries->append(xValue, 0);
    upperSeries->append(xValue, yValue);

    // Update axes ranges - consider batch updating for performance
    axisX->setRange(maxXValue - CHART_RANGE_SECS, maxXValue);
    axisY->setRange(0, maxYValue);
}

void DataChart::reset()
{
    minXValue = (double) COUNTDOWN_LENGTH_MS / 1000;
    maxXValue = minXValue;
    maxYValue = 1.0;

    lowerSeries->clear();
    upperSeries->clear();

    axisX->setRange(minXValue, maxXValue);
    axisY->setRange(0, maxYValue);
}

DataChart::~DataChart()
{
    // No need to delete chart elements - QChart takes ownership
    delete ui;
}
