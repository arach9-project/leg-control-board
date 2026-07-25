#ifndef GENERIC_CS_LIB_H
#define GENERIC_CS_LIB_H

#include "common/constants.hpp"
#include "current_sense/current_sense.hpp"
#include "current_sense/lowside_current_sense.hpp"
#include "foc/hardware_api.hpp"
#include "foc/time_utils.hpp"
#include "foc/utils.hpp"

class GenericCurrentSense : public CurrentSense {
  public:
    /**
      GenericCurrentSense class constructor
    */
    GenericCurrentSense(PhaseCurrent_s (*readCallback)() = nullptr,
                        void (*initCallback)() = nullptr);

    // CurrentSense interface implementing functions
    int init() override;
    PhaseCurrent_s getPhaseCurrents() override;
    int driverAlign(float align_voltage, bool modulation_centered) override;

    PhaseCurrent_s (*readCallback)() =
        nullptr; //!< function pointer to sensor reading
    void (*initCallback)() =
        nullptr; //!< function pointer to sensor initialisation

  private:
    /**
     *  Function finding zero offsets of the ADC
     */
    void calibrateOffsets();
    float
        offset_ia; //!< zero current A voltage value (center of the adc reading)
    float
        offset_ib; //!< zero current B voltage value (center of the adc reading)
    float
        offset_ic; //!< zero current C voltage value (center of the adc reading)
};

#endif
