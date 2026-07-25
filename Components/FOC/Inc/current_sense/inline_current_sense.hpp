#ifndef INLINE_CS_LIB_HPP
#define INLINE_CS_LIB_HPP

#include "common/constants.hpp"
#include "current_sense/current_sense.hpp"
#include "foc/bldc_driver.hpp"
#include "foc/hardware_api.hpp"
#include "foc/lowpass_filter.hpp"
#include "foc/time_utils.hpp"
#include "foc/utils.hpp"

class InlineCurrentSense : public CurrentSense {
  public:
    /**
      InlineCurrentSense class constructor
      @param shunt_resistor shunt resistor value
      @param gain current-sense op-amp gain
      @param phA A phase adc pin
      @param phB B phase adc pin
      @param phC C phase adc pin (optional)
    */
    InlineCurrentSense(float shunt_resistor, float gain, int pinA, int pinB,
                       int pinC = NOT_SET);
    /**
      InlineCurrentSense class constructor
      @param mVpA mV per Amp ratio
      @param phA A phase adc pin
      @param phB B phase adc pin
      @param phC C phase adc pin (optional)
    */
    InlineCurrentSense(float mVpA, int pinA, int pinB, int pinC = NOT_SET);

    // CurrentSense interface implementing functions
    int init() override;
    PhaseCurrent_s getPhaseCurrents() override;

  private:
    // gain variables
    float shunt_resistor;      //!< Shunt resistor value
    float amp_gain;            //!< amp gain value
    float volts_to_amps_ratio; //!< Volts to amps ratio

    /**
     *  Function finding zero offsets of the ADC
     */
    void calibrateOffsets();
};

#endif
