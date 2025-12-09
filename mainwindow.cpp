#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>
#include "globals.h"
#include "database.h"
#include <iostream>
#include <QLCDNumber>
#include <QLabel>
#include "libBrakeCooling/include/libBrakeCooling.h"

#define DEB qDebug()

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    dbConnected = Database::connect();
    if (!dbConnected) {
        std::cout << "Unable to establish database connection. Exiting";
        qApp->quit();
    }

    // set style
    //qApp->setStyle(QStyleFactory::create("Fusion"));
    // increase font size for better reading
    //QFont defaultFont = QApplication::font();
    //defaultFont.setPointSize(defaultFont.pointSize()+2);
    //qApp->setFont(defaultFont);
    // modify palette to dark
    //QPalette darkPalette;
    //darkPalette.setColor(QPalette::Window,QColor(53,53,53));
    //darkPalette.setColor(QPalette::WindowText,Qt::white);
    //darkPalette.setColor(QPalette::Disabled,QPalette::WindowText,QColor(127,127,127));
    //darkPalette.setColor(QPalette::Base,QColor(42,42,42));
    //darkPalette.setColor(QPalette::AlternateBase,QColor(66,66,66));
    //darkPalette.setColor(QPalette::ToolTipBase,Qt::white);
    //darkPalette.setColor(QPalette::ToolTipText,Qt::white);
    //darkPalette.setColor(QPalette::Text,Qt::white);
    //darkPalette.setColor(QPalette::Disabled,QPalette::Text,QColor(127,127,127));
    //darkPalette.setColor(QPalette::Dark,QColor(35,35,35));
    //darkPalette.setColor(QPalette::Shadow,QColor(20,20,20));
    //darkPalette.setColor(QPalette::Button,QColor(53,53,53));
    //darkPalette.setColor(QPalette::ButtonText,Qt::white);
    //darkPalette.setColor(QPalette::Disabled,QPalette::ButtonText,QColor(127,127,127));
    //darkPalette.setColor(QPalette::BrightText,Qt::red);
    //darkPalette.setColor(QPalette::Link,QColor(42,130,218));
    //darkPalette.setColor(QPalette::Highlight,QColor(42,130,218));
    //darkPalette.setColor(QPalette::Disabled,QPalette::Highlight,QColor(80,80,80));
    //darkPalette.setColor(QPalette::HighlightedText,Qt::white);
    //darkPalette.setColor(QPalette::Disabled,QPalette::HighlightedText,QColor(127,127,127));

    //qApp->setPalette(darkPalette);

    //for (const auto &name : Global::BRAKING_EVENT_DISPLAY_NAMES)
    //    ui->brakingEventComboBox->addItem(name);
    //for (const auto &name : Global::REVERSE_THRUST_DISPLAY_NAMES)
    //    ui->revThrustComboBox->addItem(name);
    for (const auto &name : Global::BRAKE_CATEGORY_DISPLAY_NAMES)
        ui->brakeCategoryComboBox->addItem(name);

    ui->validFrame->  setStyleSheet(Global::StyleSheets::VALID);
    ui->cautionFrame->setStyleSheet(Global::StyleSheets::CAUTION);
    ui->warningFrame->setStyleSheet(Global::StyleSheets::WARNING);

    m_model = QStringLiteral("B_737_800WSFP1");
    // get vectors for table values
    vec_speed  = Database::getTableValues(m_model, Global::Parameter::Speed);
    vec_weight = Database::getTableValues(m_model, Global::Parameter::Weight);
    vec_temp   = Database::getTableValues(m_model, Global::Parameter::Temperature);
    vec_alt    = Database::getTableValues(m_model, Global::Parameter::Altitude);

    setBrakeVector();
    QObject::connect(ui->brakeCategoryComboBox, &QComboBox::currentIndexChanged,
                     this, &MainWindow::setBrakeVector);
}
MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_calcPushButton_clicked()
{
    if (!dbConnected) {
        QMessageBox mb;
        mb.setText("No database available.");
        mb.setIcon(QMessageBox::Critical);
        mb.exec();
        return;
    }

    const double reference_braking_energy = referenceBrakingEnergy();
    brakingEvents(reference_braking_energy);
}

double MainWindow::referenceBrakingEnergy()
{
    const auto speed  = BrakeCooling::Params(ui->speedSpinBox->value(), vec_speed);
    const auto weight = BrakeCooling::Params(ui->weightSpinBox->value() / double(1000), vec_weight);
    const auto temp   = BrakeCooling::Params(ui->tempSpinBox->value(), vec_temp);
    const auto alt    = BrakeCooling::Params(ui->altitudeSpinBox->value() / double(1000), vec_alt);

    DEB << Database::getRefBe(m_model, speed.getLowBorder(), weight.getLowBorder(), temp.getLowBorder(), alt.getLowBorder());
    const auto ref_be_values = Database::getReferenceBrakingEnergyValues(m_model, speed, weight, temp, alt);
    const auto ref_be = BrakeCooling::Interpol(speed, weight, temp, alt, ref_be_values);

    double reference_braking_energy = ref_be.getReferenceBrakingEnergy();

    if (ui->TaxiDistanceSpinBox->value() == 0)
        return reference_braking_energy;
    else
        return reference_braking_energy + ui->TaxiDistanceSpinBox->value();
}

void MainWindow::brakingEvents(const double &reference_braking_energy)
{
    std::vector vec_reference_be = Database::getTableValues(m_model, Global::Parameter::RefBe);
    const auto brake_category = Global::BrakeCategory(ui->brakeCategoryComboBox->currentIndex());
    const BrakeCooling::Params ref_be_params(reference_braking_energy, vec_reference_be);


    QHash<QLCDNumber*, double> braking_events;
    const QVector<QLCDNumber*> minute_displays = {
        ui->minutes_mm_idle, ui->minutes_abm_idle, ui->minutes_ab3_idle, ui->minutes_ab2_idle, ui->minutes_ab1_idle,
        ui->minutes_mm_revt, ui->minutes_abm_revt, ui->minutes_ab3_revt, ui->minutes_ab2_revt, ui->minutes_ab1_revt
    };
    for (const auto& display : minute_displays)
        display->setStyleSheet(QString());

    int vector_index = 0;
    for (int i = 0 ; i < 2; i++) {
        bool rev_t = i;
        for (int j = 0; j < 5; j++) {
            auto event = Global::BrakingEvent(j);
            const auto adjusted_be = adjustedBrakeEnergy(ref_be_params, event, rev_t);
            if (adjusted_be > Database::getCautionValue(m_model, brake_category)) {
                styleLCDNumber(adjusted_be, brake_category, minute_displays[vector_index]);
                //DEB << "Caution - Adjusted BE: " << adjusted_be << " - Limit value: " << AdjustedBrakingEnergyParameters::getCautionValue(brake_category) << '(' << Global::BRAKE_CATEGORY_DISPLAY_NAMES.value(brake_category) << ')';
            } else {
                const auto adjusted_be_parameters = BrakeCooling::Params(adjusted_be, vec_brakes);
                const auto cooling_time           = coolingTime(adjusted_be_parameters, brake_category);
                DEB << adjusted_be_parameters.getInputParameter();
                braking_events.insert(minute_displays[vector_index], cooling_time);
            }
            vector_index ++;
        }
    }

    QHash<QLCDNumber*, double>::Iterator i;
    for (i = braking_events.begin(); i != braking_events.end(); ++i) {
        //DEB << "Setting: " << i.key()->objectName() << " to value: " << i.value();
        if (i.value() >0)
            i.key()->display(i.value());
        else {
            i.key()->display(QString());
            i.key()->setStyleSheet(Global::StyleSheets::VALID);
        }
    }
}

void MainWindow::styleLCDNumber(const double &adjusted_braking_energy, Global::BrakeCategory brake_category, QLCDNumber *display)
{
    display->display(QString());
    switch (brake_category) {
    case Global::BrakeCategory::Steel:
        if (adjusted_braking_energy > Database::getWarningValue(m_model, Global::BrakeCategory::Steel))
            display->setStyleSheet(Global::StyleSheets::WARNING);
        else
            display->setStyleSheet(Global::StyleSheets::CAUTION);
        break;
    case Global::BrakeCategory::Carbon:
        if (adjusted_braking_energy > Database::getWarningValue(m_model, Global::BrakeCategory::Carbon))
            display->setStyleSheet(Global::StyleSheets::WARNING);
        else
            display->setStyleSheet(Global::StyleSheets::CAUTION);
        break;
    }
}

double MainWindow::adjustedBrakeEnergy(const BrakeCooling::Params &ref_be_parameters, Global::BrakingEvent event, bool rev_t)
{
    // get Values
    const auto &[ref_be_low, ref_be_high, ref_be_param] = ref_be_parameters.getValues();{}
    const double value_low  = Database::getAdjustedBe(m_model, ref_be_low, event, rev_t);
    const double value_high = Database::getAdjustedBe(m_model, ref_be_high, event, rev_t);

    const auto ret = BrakeCooling::linearInterpol(ref_be_param, ref_be_low, value_low, ref_be_high, value_high);

    // Debug
    const char* rev = rev_t ? " - Second Detent" : " - Idle Reverse";
    DEB << "Adjusted Brake Energy for event: " << Global::BRAKING_EVENT_DISPLAY_NAMES.value(event) << rev << ":" << ret;

    return ret;
}

double MainWindow::coolingTime(const BrakeCooling::Params &adj_be, Global::BrakeCategory brake_category)
{
    const auto&[abe_low, abe_high, abe_param] = adj_be.getValues();{}
    DEB << "Cooling Time Parameters received: " << abe_low << '/' << abe_high << '/' << abe_param;
    if (abe_high == 0)
        return -1; // No special procedure required per Brake Cooling Schedule
    const double value_low = Database::getCoolingTime( m_model, brake_category, abe_low);
    const double value_high = Database::getCoolingTime(m_model, brake_category, abe_high);

    const auto ret = BrakeCooling::linearInterpol(abe_param, abe_low, value_low, abe_high, value_high);
    DEB << "Cooling Time: " << ret << " minutes for category " << Global::BRAKE_CATEGORY_DISPLAY_NAMES.value(brake_category);

    return ret;
}

void MainWindow::setBrakeVector()
{
    switch (ui->brakeCategoryComboBox->currentIndex()) {
    case 0:
        vec_brakes = Database::getTableValues(m_model, Global::Parameter::AdjustedSteel);
        break;
    case 1:
        vec_brakes = Database::getTableValues(m_model, Global::Parameter::AdjustedCarbon);
        break;
    }
    DEB << "Brakes reset: " << ui->brakeCategoryComboBox->currentText() << vec_brakes;
}
