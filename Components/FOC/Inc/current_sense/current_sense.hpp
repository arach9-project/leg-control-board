#ifndef CURRENT_SENSE_HPP
#define CURRENT_SENSE_HPP

// #include "StepperDriver.h"
// #include "bldc_driver.hpp"
// #include "foc/bldc_driver.hpp"
#include "foc/bldc_driver.hpp"
#include "foc/foc_driver.hpp"
#include "foc/time_utils.hpp"
#include "foc/utils.hpp"

/**
 *  Current sensing abstract class defintion
 * Each current sensing implementation needs to extend this interface
 */
class CurrentSense {
  public:
    /**
     *  Function intialising the CurrentSense class
     *   - All the necessary intialisations of adc and sync should be
     * implemented here
     *
     * @returns -  0 - for failure &  1 - for success
     */
    virtual int init() = 0;

    /**
     * Linking the current sense with the motor driver
     * Only necessary if synchronisation in between the two is required
     */
    void linkDriver(FOCDriver *_driver);

    // variables
    bool skip_align = false; //!< variable signaling that the phase current
                             //!< direction should be verified during initFOC()

    FOCDriver *driver = nullptr; //!< driver link
    bool initialized =
        false; // true if current sense was successfully initialized
    void *params =
        0; //!< pointer to hardware specific parameters of current sensing
    DriverType driver_type =
        DriverType::UnknownDriver; //!< driver type (BLDC or Stepper)

    // ADC measurement gain for each phase
    // support for different gains for different phases of more commonly -
    // inverted phase currents this should be automated later
    float gain_a; //!< phase A gain
    float gain_b; //!< phase B gain
    float gain_c; //!< phase C gain

    float
        offset_ia; //!< zero current A voltage value (center of the adc reading)
    float
        offset_ib; //!< zero current B voltage value (center of the adc reading)
    float
        offset_ic; //!< zero current C voltage value (center of the adc reading)

    // hardware variables
    int pinA; //!< pin A analog pin for current measurement
    int pinB; //!< pin B analog pin for current measurement
    int pinC; //!< pin C analog pin for current measurement

    /**
     * Function intended to verify if:
     *   - phase current are oriented properly
     *   - if their order is the same as driver phases
     *
     * This function corrects the alignment errors if possible ans if no such
     thing is needed it can be left empty (return 1)
     * @returns -
            0 - failure
            1 - success and nothing changed
            2 - success but pins reconfigured
            3 - success but gains inverted
            4 - success but pins reconfigured and gains inverted
     *
     * IMPORTANT: Default implementation provided in the CurrentSense class, but
     can be overriden in the child classes
     */
    // Function aligning the current sense with motor driver
    // if all pins are connected well none of this is really necessary! - can be
    // avoided returns flag 0 - fail 1 - success and nothing changed 2 - success
    // but pins reconfigured 3 - success but gains inverted 4 - success but pins
    // reconfigured and gains inverted IMPORTANT, this function can be overriden
    // in the child class
    virtual int driverAlign(float voltage, bool modulation_centered = false);

    /**
     *  Function rading the phase currents a, b and c
     *   This function will be used with the foc control throught the function
     *   getFOCCurrents(electrical_angle)
     *   - it returns current c equal to 0 if only two phase measurements
     * available
     *
     *  @return PhaseCurrent_s current values
     */
    virtual PhaseCurrent_s getPhaseCurrents() = 0;
    /**
     * Function reading the magnitude of the current set to the motor
     *  It returns the absolute or signed magnitude if possible
     *  It can receive the motor electrical angle to help with calculation
     *  This function is used with the current control  (not foc)
     *
     * @param angle_el - electrical angle of the motor (optional)
     */
    // get current magnitude
    //   - absolute  - if no electrical_angle provided
    //   - signed    - if angle provided
    float getDCCurrent(float motor_electrical_angle);

    /**
     * Function used for FOC control, it reads the DQ currents of the motor
     *   It uses the function getPhaseCurrents internally
     *
     * @param angle_el - motor electrical angle
     */

    // function used with the foc algorithm
    //   calculating DQ currents from phase currents
    //   - function calculating park and clarke transform of the phase currents
    //   - using getPhaseCurrents and getABCurrents internally
    DQCurrent_s getFOCCurrents(float angle_el);

    /**
     * Function used for Clarke transform in FOC control
     *   It reads the phase currents of the motor
     *   It returns the alpha and beta currents
     *
     * @param current - phase current
     */
    // function used with the foc algorithm
    //   calculating Alpha Beta currents from phase currents
    //   - function calculating Clarke transform of the phase currents
    ABCurrent_s getABCurrents(PhaseCurrent_s current);

    /**
     * Function used for Park transform in FOC control
     *   It reads the Alpha Beta currents and electrical angle of the motor
     *   It returns the D and Q currents
     *
     * @param current - phase current
     */
    // function used with the foc algorithm
    //   calculating D and Q currents from Alpha Beta currents and electrical
    //   angle
    //   - function calculating Clarke transform of the phase currents
    DQCurrent_s getDQCurrents(ABCurrent_s current, float angle_el);

    /**
     * enable the current sense. default implementation does nothing, but you
     * can override it to do something useful.
     */
    void enable() {
        // nothing is done here, but you can override this function
    };

    /**
     * disable the current sense. default implementation does nothing, but you
     * can override it to do something useful.
     */
    void disable() {
        // nothing is done here, but you can override this function
    };

    /**
     * Function used to align the current sense with the BLDC motor driver
     */
    // Function aligning the current sense with motor driver
    // if all pins are connected well none of this is really necessary! - can be
    // avoided returns flag 0 - fail 1 - success and nothing changed 2 - success
    // but pins reconfigured 3 - success but gains inverted 4 - success but pins
    // reconfigured and gains inverted
    int alignBLDCDriver(float voltage, BLDCDriver *bldc_driver,
                        bool modulation_centered);

    /**
     * Function used to align the current sense with the Stepper motor driver
     */
    // Function aligning the current sense with motor driver
    // if all pins are connected well none of this is really necessary! - can be
    // avoided returns flag 0 - fail 1 - success and nothing changed 2 - success
    // but pins reconfigured 3 - success but gains inverted 4 - success but pins
    // reconfigured and gains inverted
    // TODO: Uncomment this and get this to work
    // int alignStepperDriver(float voltage, StepperDriver *stepper_driver,
    //                        bool modulation_centered);

    /**
     * Function used to align the current sense with the Hybrid motor driver
     */
    int alignHybridDriver(float voltage, BLDCDriver *bldc_driver,
                          bool modulation_centered);

    /**
     * Function used to read the average current values over N samples
     */
    // Helper function to read and average phase currents
    PhaseCurrent_s readAverageCurrents(int N = 100);
};

#endif // !CURRENT_SENSE_HPP
