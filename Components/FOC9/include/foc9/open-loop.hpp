//
// class OpenLoop {
//   private:
//     float theta_el = 0.0f;
//     float target_speed_hz = 5.0f;
//     float v_amp = 0.15f;
//     float arr = 0.0f;
//
//     uint16_t ccr_a, ccr_b, ccr_c;
//
//   public:
//     OpenLoop(float theta_el, float target_speed_hz, float v_amp, float arr);
//
//     void step(float dt);
//
//     const uint16_t getCCRa();
//
//     const uint16_t getCCRb();
//
//     const uint16_t getCCRc();
// };
//
// OpenLoop::OpenLoop(float theta_el, float target_speed_hz, float v_amp,
//                    float arr) {
//
//     this->theta_el = theta_el;
//     this->target_speed_hz = target_speed_hz;
//     this->arr = arr;
// }
//
// void OpenLoop::step(float dt) {
//     theta_el += TWO_PI * target_speed_hz * dt;
//     if (theta_el >= TWO_PI)
//         theta_el -= TWO_PI;
//
//     // Normalized Phase calc
//     float v_a = sinf(theta_el);
//     float v_b = sinf(theta_el - TWO_PI / 3.0f);
//     float v_c = sinf(theta_el + TWO_PI / 3.0f);
//
//     // 3. Scale by Vq (target amplitude)
//
//     // 4. Map [-1.0, +1.0] -> [0, ARR]
//     uint16_t half_arr = arr / 2.0f;
//     ccr_a = (1.0f + v_amp * v_a) * half_arr;
//     ccr_b = (v_b + 1) * half_arr;
//     ccr_c = (v_c + 1) * half_arr;
// }
//
// const uint16_t OpenLoop::getCCRa() {
//     return ccr_a;
// }
//
// const uint16_t OpenLoop::getCCRb() {
//     return ccr_b;
// }
//
// const uint16_t OpenLoop::getCCRc() {
//     return ccr_c;
// }
