#ifndef GLOBALS_H
#define GLOBALS_H
#pragma once
#include <QtCore>
#define DEB qDebug()
#define DEB_double qDebug() << qSetRealNumberPrecision(16)

namespace Global {

enum class Parameter {Speed, Weight, Temperature, Altitude, RefBe, AdjustedSteel, AdjustedCarbon};

/*!
 * \brief enumerates braking events
 */
enum class BrakingEvent {MAX_MAN = 0, AB_MAX = 1, AB_3 = 2, AB_2 = 3, AB_1 = 4};

const static inline QMap<BrakingEvent, const char*> BRAKING_EVENT_DISPLAY_NAMES = {
    {BrakingEvent::MAX_MAN, "MAX MANUAL"},
    {BrakingEvent::AB_MAX,  "AUTOBRAKE MAX"},
    {BrakingEvent::AB_3,    "AUTOBRAKE 3"},
    {BrakingEvent::AB_2,    "AUTOBRAKE 2"},
    {BrakingEvent::AB_1,    "AUTOBRAKE 1"},
};

const static inline QMap<bool, const char*> REVERSE_THRUST_DISPLAY_NAMES = {
    {false, "IDLE"},
    {true,  "SECOND DETENT"},
};

enum class BrakeCategory {Steel = 0, Carbon = 1};

const static inline QMap<BrakeCategory, const char*> BRAKE_CATEGORY_DISPLAY_NAMES {
    {BrakeCategory::Steel,  "C (Steel Brakes)"},
    {BrakeCategory::Carbon, "N (Carbon Brakes)"},
};

namespace StyleSheets {

    const static inline char* VALID =   "background-image: url(:/valid.png)";
    const static inline char* CAUTION = "background-image: url(:/caution.png)";
    const static inline char* WARNING = "background-image: url(:/warning.png)";

} // namespace StyleSheets

} // namespace Global

#endif // GLOBALS_H
