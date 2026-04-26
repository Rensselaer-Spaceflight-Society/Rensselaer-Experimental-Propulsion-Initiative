#ifndef DATACHART_H
#define DATACHART_H

#include <QWidget>
#include <QtCharts/QtCharts>
#include "mainwindow.h"


enum class ChartType {
    Force,
    Temperature,
    Pressure
};

namespace Ui {
class DataChart;
}

class DataChart : public QWidget
{
    Q_OBJECT

public:
    explicit DataChart(QWidget *parent = nullptr);
    ~DataChart();
    void append(float xValue, float yValue);
    void setChartType(ChartType chartType);
    void setChartTitle(const QString& title);
    void reset();

private:
    Ui::DataChart *ui;
    QChart * chart;
    QAreaSeries * series;
    QLineSeries * lowerSeries;
    QLineSeries * upperSeries;
    QValueAxis * axisX;
    QValueAxis * axisY;
    double maxYValue = 1;
    double minXValue = COUNTDOWN_LENGTH_MS;
    double maxXValue = minXValue;
};

#endif // DATACHART_H
