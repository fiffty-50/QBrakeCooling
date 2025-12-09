#include "libBrakeCooling.h"

namespace BrakeCooling {

Params::Params(const double &parameter_in, const std::vector<double> &table_values)
    : m_input_parameter(parameter_in)
{
    if( std::find(table_values.begin(), table_values.end(), parameter_in) != table_values.end()) {
        // input_parameter is contained in vector
        m_low_border  = parameter_in;
        m_high_border = parameter_in;
    } else {
        // starting at the second element, find the first element that is >= parameter_in
        for (int i = 1; table_values.size(); ++i)
            if (table_values.at(i) >= parameter_in) {
                m_low_border = table_values.at(i - 1);
                m_high_border = table_values.at(i);
                break;
            }
    }
}

Interpol::Interpol(const Params &speed, 
             const Params &weight,
             const Params &temp, 
             const Params &alt,
             const std::array<double, 16> &raw_ref_be)
{
    // 1) interpolate with speed parameter
    std::array<double, 8> speed_corrected_braking_energy;
    const auto &[ speed_low, speed_high, speed_param ]  = speed.getValues();
    if (speed_param == speed_low) // no interpolation needed, carry forward every second data point
        for (int i = 0; i < 8; i++)
            speed_corrected_braking_energy[i] = raw_ref_be[2*i];
    else
        speed_corrected_braking_energy = correctSpeed(raw_ref_be, speed_param, speed_low, speed_high);

    // 2) interpolate with weight parameter
    std::array<double, 4> weight_corrected_braking_energy;
    const auto &[ weight_low, weight_high, weight_param ] = weight.getValues();
    if (weight_param == weight_low)
        for (int i = 0; i < 4; i++)
            weight_corrected_braking_energy[i] = speed_corrected_braking_energy[2*i];
    else
        weight_corrected_braking_energy = correctWeight(speed_corrected_braking_energy, weight_param, weight_low, weight_high);

    // 3) interpolate with temperature parameter
    std::array<double, 2> temperature_corrected_braking_energy;
    const auto &[ temp_low, temp_high, temp_param] = temp.getValues();
    if (temp_param == temp_low)
        for (int i = 0; i < 2; i++)
            temperature_corrected_braking_energy[i] = weight_corrected_braking_energy[i * 2];
    else
        temperature_corrected_braking_energy = correctTemperature(weight_corrected_braking_energy, temp_param, temp_low, temp_high);

    // 4) interpolate with altitude parameter
    const auto &[ alt_low, alt_high, alt_param ] = alt.getValues();
    if (alt_param == alt_low)
        m_interpolation = temperature_corrected_braking_energy[0];
    else
        m_interpolation = linearInterpol(alt_param,
                                          alt_low, temperature_corrected_braking_energy[0],
                                          alt_high, temperature_corrected_braking_energy[1]);
}

const std::array<double, 8> Interpol::correctSpeed(const std::array<double, 16> &raw_braking_energy,
                                                  const double &speed_param, const double &speed_low, const double &speed_high) const
{
    std::array<double, 8> speed_corrected_braking_energy;

    for (int i = 0; i < 8; i ++) {
        const double &value_low = raw_braking_energy[i];
        const double &value_high = raw_braking_energy[i + 1];
        speed_corrected_braking_energy[i] = linearInterpol(speed_param,
                                                            speed_low, value_low,
                                                            speed_high, value_high);
    }

    return speed_corrected_braking_energy;
}

const std::array<double, 4> Interpol::correctWeight(const std::array<double, 8> &speed_corrected_be,
                                                   const double &weight_param, const double &weight_low, const double &weight_high) const
{
    std::array<double, 4> weight_corrected_brake_energy;

    for (int i = 0; i < 4; i++) {
        const double &value_low = speed_corrected_be[i];
        const double &value_high = speed_corrected_be[i+1];
        weight_corrected_brake_energy[i] = linearInterpol(weight_param,
                                                           weight_low, value_low,
                                                           weight_high, value_high);
    }

    return weight_corrected_brake_energy;
}

const std::array<double, 2> Interpol::correctTemperature(const std::array<double, 4> &weight_corrected_be,
                                                        const double &temp_param, const double &temp_low, const double &temp_high) const
{
    std::array<double, 2> temperature_corrected_brake_energy;

    for (int i = 0; i < 2; i++) {
        const double &value_low = weight_corrected_be[i];
        const double &value_high = weight_corrected_be[i+1];
        temperature_corrected_brake_energy[i] = linearInterpol(temp_param,
                                                                temp_low, value_low,
                                                                temp_high, value_high);
    }

    return temperature_corrected_brake_energy;
}

} // namespace BrakeCooling