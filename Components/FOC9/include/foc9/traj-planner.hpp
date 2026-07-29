// trapezoidal trajectory planner
// This one is also extremely useful.
// Suppose you command
// Move from
//
// 0°
//
// to
//
// 180°
// Without trajectory planning
// 0°
//
// ↓
//
// 180°
//
// instantaneously
// The controller immediately asks for maximum torque.
// Huge jerk.
// Instead
// velocity
//
//       /\
//      /  \
// ____/    \____
// This is a trapezoidal velocity profile.
// Acceleration
// ↓
// Constant velocity
// ↓
// Deceleration
// Position looks like
//       ________
//
//     /
//
//   /
//
// /
// Nice and smooth.
// You specify
// max velocity
//
// max acceleration
//
// max deceleration
// The planner generates intermediate position setpoints.
// The position controller simply follows them.
