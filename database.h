#ifndef DATABASE_H
#define DATABASE_H

#include <QDir>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include <QDebug>
#include <QStringLiteral>
#include <QMessageBox>
#include "globals.h"
#include "libBrakeCooling/include/libBrakeCooling.h"

class Database {
private:
    const static inline char* DRIVER  = "QSQLITE";
    const static inline char* DB_FILE = "database.db";
    const static inline char* CHECK_QUERY = "SELECT name FROM sqlite_master";
    inline static double executeQuery(QSqlQuery &query);

    static void error(const QString &error_msg, QWidget* parent = nullptr);
public:
    /*!
     * \brief Establish the database connection
     */
    static bool connect(QWidget* parent = nullptr);

    static std::vector<double> getTableValues(const QString &table_name, Global::Parameter parameter);
    static std::array<double, 16> getReferenceBrakingEnergyValues(
            const QString &table_name,
            const BrakeCooling::Params &speed,
            const BrakeCooling::Params &weight,
            const BrakeCooling::Params &temp,
            const BrakeCooling::Params &alt );

    static double getCautionValue(const QString &table_name, Global::BrakeCategory brake_category);
    static double getWarningValue(const QString &table_name, Global::BrakeCategory brake_category);

    /*!
     * \brief retreive reference braking energy for a given speed, weight, temperature and altitude values.
     */
    static double getRefBe(const QString &table_name, int speed, int weight, int temp, int alt);

    /*!
     * \brief retreive adjusted brake energy for a given reference brake energy, braking event and reverse thrust usage
     */
    static double getAdjustedBe(const QString &table_name, int reference_braking_energy, Global::BrakingEvent braking_event, bool rev_t);

    /*!
     * \brief retreive cooling time (in minutes) for a given adjusted brake energy and brake category
     */
    static double getCoolingTime(const QString &table_name, Global::BrakeCategory brake_category, const double &adjusted_be);


};

#endif // DATABASE_H
