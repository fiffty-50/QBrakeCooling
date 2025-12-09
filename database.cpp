#include "database.h"

double Database::executeQuery(QSqlQuery &query)
{
    if (!query.exec()) {
        error("Unable to execute query.<br>" + query.lastQuery());
        return 0;
    }

    if (!query.next()) {
        QString error_msg = "Query result empty.<br>" + query.lastQuery() + "<br>";
        for (const auto &item : query.boundValues())
            error_msg.append(item.toString() + "<br>");
        error(error_msg);
        return 0;
    }

    //DEB << "DB Return: " << query.value(0).toDouble();
    return query.value(0).toDouble();
}

void Database::error(const QString& error_msg, QWidget *parent)
{
    QMessageBox mb(parent);
    mb.setText("<b>Database Error</b><br><br>" + error_msg);
    mb.setIcon(QMessageBox::Warning);
    mb.exec();
}

bool Database::connect(QWidget *parent)
{
    if (!QSqlDatabase::isDriverAvailable(DRIVER)) {
        error("No SQLITE Driver availabe.", parent);
        return false;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(DRIVER);
    db.setDatabaseName(DB_FILE);

    if (!db.open()) {
        error(QString("Unable to establish database connection. The following error has ocurred:<br><br>%1")
               .arg(db.lastError().databaseText()), parent);
        return false;
    }

    QSqlQuery q;
    q.prepare(CHECK_QUERY);
    q.exec();
    if (!q.next()) {
        error("No database detected.", parent);
        return false;
    }

    DEB << "Database connection established.";
    return true;
}

std::vector<double> Database::getTableValues(const QString &table_name, Global::Parameter parameter)
{
    QString par;
    switch (parameter) {
    case Global::Parameter::Speed:
        par = QLatin1String("speed");
        break;
    case Global::Parameter::Weight:
        par = QLatin1String("weight");
        break;
    case Global::Parameter::Temperature:
        par = QLatin1String("temp");
        break;
    case Global::Parameter::Altitude:
        par = QLatin1String("alt");
        break;
    case Global::Parameter::RefBe:
        par = QLatin1String("referenceBrakeEnergy");
        break;
    case Global::Parameter::AdjustedSteel:
        par = QLatin1String("adjustedBrakeEnergySteel");
        break;
    case Global::Parameter::AdjustedCarbon:
        par = QLatin1String("adjustedBrakeEnergyCarbon");
        break;
    }

    auto q = QString("SELECT COUNT(%1) FROM B_737_800WSFP1_KEYS WHERE %2 NOT NULL").arg(par, par);

    QSqlQuery query;
    query.prepare(q);
    query.exec();

    int result_size;
    if (!query.next())
        return {};
    else
        result_size = query.value(0).toInt();

    if (result_size <= 0)
        return {};

    DEB << "Q1 " << query.lastQuery() << "size:" << result_size;


    q = QString("SELECT %1 FROM %2_KEYS WHERE %3 NOT NULL").arg(par, table_name, par);
    query.prepare(q);

    query.exec();
    if(!query.next())
        return {};

    std::vector<double> ret(result_size);
    for (auto &i : ret) {
        i = query.value(0).toDouble();
        query.next();
    }
    return ret;
}

std::array<double, 16> Database::getReferenceBrakingEnergyValues(
        const QString &table_name,
        const BrakeCooling::Params &speed,
        const BrakeCooling::Params &weight,
        const BrakeCooling::Params &temp,
        const BrakeCooling::Params &alt )
{
    // get input values
    const auto &[ speed_low,  speed_high,  speed_param ]  = speed.getValues();{}
    const auto &[ weight_low, weight_high, weight_param ] = weight.getValues();{}
    const auto &[ temp_low,   temp_high,   temp_param]    = temp.getValues();{}
    const auto &[ alt_low,    alt_high,    alt_param ]    = alt.getValues();

    // Get Reference BE from database for all combinations. Use n-Least significant bit of index to determine if input is high(1) or low(0)
    std::array<double, 16> raw_braking_energy;
    for (int i = 0; i < 16; i++) {
        int speed_temp, weight_temp, temp_temp, alt_temp;
        i & 1        ? speed_temp  = speed_high  : speed_temp  = speed_low;     // LSB set, speed high
        i & (1 << 1) ? weight_temp = weight_high : weight_temp = weight_low;    // 2nd LSB set, weight high
        i & (1 << 2) ? temp_temp   = temp_high   : temp_temp   = temp_low;      // 3rd LSB set, temperature high
        i & (1 << 3) ? alt_temp    = alt_high    : alt_temp    = alt_low;       // 4th LSB set, altitude high

        DEB << "Retreiving data for: " << speed_temp << weight_temp << temp_temp << alt_temp << " index : " << i;
        DEB << "Result: " << getRefBe(table_name, speed_temp, weight_temp, temp_temp, alt_temp);
        raw_braking_energy[i] = getRefBe(table_name, speed_temp, weight_temp, temp_temp, alt_temp);
    }
    return raw_braking_energy;
}

double Database::getCautionValue(const QString &table_name, Global::BrakeCategory brake_category)
{
    QString value;
    switch (brake_category) {
    case Global::BrakeCategory::Steel:
        value = "adjustedBrakeEnergySteel";
        break;
    case Global::BrakeCategory::Carbon:
        value = "adjustedBrakeEnergyCarbon";
        break;
    }
    auto q = QString("SELECT MAX(%1) FROM %2_KEYS "
                     "WHERE %3 < "
                     "(SELECT MAX(%4) FROM %5_KEYS ) ")
            .arg(value, table_name, value, value, table_name);
    QSqlQuery query;
    query.prepare(q);
    query.exec();

    DEB << query.lastQuery();

    if (!query.next())
        return -1;
    else
        return query.value(0).toDouble();
}

double Database::getWarningValue(const QString &table_name, Global::BrakeCategory brake_category)
{
    QString value;
    switch (brake_category) {
    case Global::BrakeCategory::Steel:
        value = "adjustedBrakeEnergySteel";
        break;
    case Global::BrakeCategory::Carbon:
        value = "adjustedBrakeEnergyCarbon";
        break;
    }
    auto q = QString("SELECT MAX(%1) FROM %2_KEYS").arg(value, table_name);
    QSqlQuery query;
    query.prepare(q);
    query.exec();

    DEB << query.lastQuery();

    if (!query.next())
        return -1;
    else
        return query.value(0).toDouble();
}

double Database::getRefBe(const QString &table_name, int speed, int weight, int temp, int alt)
{
    const QString q = QString("SELECT referenceBE FROM %1_RAW_BE WHERE speed = ? AND weight = ? AND temperature = ? AND altitude = ? ").arg(table_name);
    QSqlQuery query;
    query.prepare(q);
    query.addBindValue(speed);
    query.addBindValue(weight);
    query.addBindValue(temp);
    query.addBindValue(alt);

    return executeQuery(query);
}

double Database::getAdjustedBe(const QString &table_name, int reference_braking_energy, Global::BrakingEvent braking_event, bool rev_t)
{
    auto q = QString("SELECT adjustedBE FROM %1_ADJ_BE WHERE refBE = ? AND event = ? AND revT = ?").arg(table_name);
    QSqlQuery query;
    query.prepare(q);
    query.addBindValue(reference_braking_energy);
    query.addBindValue(static_cast<int>(braking_event));
    query.addBindValue(rev_t);

    return executeQuery(query);
}

double Database::getCoolingTime(const QString &table_name, Global::BrakeCategory brake_category, const double &adjusted_be)
{
    auto q = QString("SELECT coolingTime FROM %1_COOLING_TIME WHERE brakeCategory = ? AND adjustedBE = ?").arg(table_name);
    QSqlQuery query;
    query.prepare(q);
    query.addBindValue(static_cast<int>(brake_category));
    query.addBindValue(adjusted_be);
    return executeQuery(query);
}
