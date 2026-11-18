#ifndef SCOUT_ODOMETRY_HPP
#define SCOUT_ODOMETRY_HPP

#include <chrono>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace westonrobot {

// Per-axis signs for (linear.x, linear.y, angular.z) in the body frame.
struct VelocitySigns {
  int x = 1;
  int y = 1;
  int wz = 1;
};

inline VelocitySigns ValidateVelocitySigns(const std::vector<int64_t> &values,
                                           const std::string &name) {
  if (values.size() != 3) {
    throw std::invalid_argument(name + " must have 3 entries [x, y, wz]");
  }
  for (int64_t v : values) {
    if (v != 1 && v != -1) {
      throw std::invalid_argument(name + " entries must be +1 or -1");
    }
  }
  return {static_cast<int>(values[0]), static_cast<int>(values[1]),
          static_cast<int>(values[2])};
}

inline double ValidateFeedbackTimeout(double timeout_s) {
  if (!std::isfinite(timeout_s) || timeout_s <= 0.0) {
    throw std::invalid_argument("feedback_timeout must be a positive number");
  }
  return timeout_s;
}

// The SDK keeps the last state after CAN goes silent, so its receive stamp
// tells whether the motion state is still being updated.
template <typename TimePoint>
inline bool IsFeedbackFresh(TimePoint stamp, TimePoint now, double timeout_s) {
  return std::chrono::duration<double>(now - stamp).count() <= timeout_s;
}

struct OdometryStep {
  double linear_speed;
  double lateral_speed;
  double angular_speed;
  double dx;
  double dy;
  double dtheta;
};

inline OdometryStep ComputeOdometryStep(double linear_speed,
                                       double lateral_speed,
                                       double angular_speed, double yaw,
                                       double dt,
                                       const VelocitySigns &signs = {}) {
  linear_speed *= signs.x;
  lateral_speed *= signs.y;
  angular_speed *= signs.wz;
  return {linear_speed, lateral_speed, angular_speed,
          (linear_speed * std::cos(yaw) - lateral_speed * std::sin(yaw)) * dt,
          (linear_speed * std::sin(yaw) + lateral_speed * std::cos(yaw)) * dt,
          angular_speed * dt};
}

}  // namespace westonrobot

#endif
