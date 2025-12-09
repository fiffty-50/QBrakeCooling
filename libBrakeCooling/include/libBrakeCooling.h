#pragma once
#include <vector>
#include <algorithm>
#include <array>

namespace BrakeCooling {

/*!
 * \brief enumerates braking events Max Manual and Autobrake Max, 3, 2, 1
 */
enum class BrakingEvent {MaxMan = 0, AbMax = 1, Ab3 = 2, Ab2 = 3, Ab1 = 4};

enum class BrakeCategory {Steel = 0, Carbon = 1};

/*!
 * \brief performs linear interpolation
 */
inline double linearInterpol(const double parameter_in,
                             const double &parameter_low, const double &value_low,
                             const double &parameter_high, const double &value_high)
{
    return value_low + (((parameter_in - parameter_low) * (value_high - value_low)) / (parameter_high - parameter_low));
}

/*!
 * \brief convenience struct to use with structured bindings
 */
struct Values
{
    Values(const double &low, const double &high, const double &param)
    : low_border(low), high_border(high), input_parameter(param) {;}
    double low_border;
    double high_border;
    double input_parameter;
};

/*!
 * \brief Base class for the parameters affecting the calculation (speed, weight, temperature, altitude)
 * \details A Parameter is received as an exact doubleing point value. This value is then compared against
 * a set of values for which data points exist in the table. The high and low values corresponding
 * to the next closest value in the allowable input range can be retreived with getValues()
 */
class Params
{
public:
    Params() = delete;
    Params(const double &parameter_in, const std::vector<double> &table_values);

    double getLowBorder()  const {return m_low_border;}
    double getHighBorder() const {return m_high_border;}
    double getInputParameter() const {return m_input_parameter;}

    Values getValues() const {return Values(m_low_border, m_high_border, m_input_parameter);}
private:
    double m_input_parameter;
    double m_low_border;
    double m_high_border;
};

/*!
 * \brief Interpolates a reference braking energy value from the raw input parameters
   \details todo, input array, drill down
 */
class Interpol 
{
public:
    Interpol() = delete;
    Interpol(const Params &speed, 
             const Params &weight,
             const Params &temp, 
             const Params &alt,
             const std::array<double, 16> &raw_ref_be);
    double getReferenceBrakingEnergy() const { return m_interpolation;}
private:
    const std::array<double, 8> correctSpeed(const std::array<double, 16> &raw_braking_energy,
                                             const double &speed_param, 
                                             const double &speed_low, 
                                             const double &speed_high) const;
    const std::array<double, 4> correctWeight(const std::array<double, 8> &speed_corrected_be,
                                             const double &weight_param,
                                             const double &weight_low,
                                             const double &weight_high) const;
    const std::array<double, 2> correctTemperature(const std::array<double, 4> &weight_corrected_be,
                                                  const double &temp_param,
                                                  const double &temp_low,
                                                  const double &temp_high) const;
    double m_interpolation = 0;
};

} // namespace BrakeCooling
