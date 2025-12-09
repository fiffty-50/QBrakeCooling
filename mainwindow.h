#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLCDNumber>
#include "globals.h"
#include "libBrakeCooling/include/libBrakeCooling.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_calcPushButton_clicked();
    void setBrakeVector();

private:
    Ui::MainWindow *ui;
    bool dbConnected;
    void tempCorrect(double x, double ref_be);
    double referenceBrakingEnergy();
    void brakingEvents(const double &reference_braking_energy);
    void styleLCDNumber(const double &adjusted_braking_energy, Global::BrakeCategory brake_category, QLCDNumber *display);

    std::vector<double> vec_speed;
    std::vector<double> vec_weight;
    std::vector<double> vec_temp;
    std::vector<double> vec_alt;
    std::vector<double> vec_brakes;
    int weight_step = 500;

    QString m_model;
    double adjustedBrakeEnergy(const BrakeCooling::Params &ref_be_parameters, Global::BrakingEvent event, bool rev_t);
    double coolingTime(const BrakeCooling::Params &adj_be, Global::BrakeCategory brake_category);
};
#endif // MAINWINDOW_H
