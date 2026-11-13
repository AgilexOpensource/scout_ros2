#ifndef SCOUT_ODOMETRY_HPP
#define SCOUT_ODOMETRY_HPP

#include <cmath>
#include <stdexcept>

namespace westonrobot {

inline int ValidateOdometryLinearVelocitySign(int sign) {
  if (sign != 1 && sign != -1) {
    throw std::invalid_argument("odom_linear_velocity_sign must be +1 or -1");
  }
  return sign;
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
                                       double dt, int linear_velocity_sign = 1) {
  linear_speed *= ValidateOdometryLinearVelocitySign(linear_velocity_sign);
  return {linear_speed, lateral_speed, angular_speed,
          (linear_speed * std::cos(yaw) - lateral_speed * std::sin(yaw)) * dt,
          (linear_speed * std::sin(yaw) + lateral_speed * std::cos(yaw)) * dt,
          angular_speed * dt};
}

}  // namespace westonrobot

#endif
