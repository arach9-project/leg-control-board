#include "arm_math.h"

class PIController {
  private:
    float Kp;
    float Ki;
    float integrator;
    float out_max;
    float out_min;
    float _output;

  public:
    PIController(float vbus, float Kp_0, float Ki_0);
    void update(float target, float measured, float dt);
    float output();
};
