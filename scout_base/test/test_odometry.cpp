#include <cmath>
#include <iostream>
#include <stdexcept>

#include "scout_base/scout_odometry.hpp"

namespace {
void Check(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

bool Near(double actual, double expected) {
  return std::abs(actual - expected) < 1e-12;
}
}  // namespace

int main() {
  using westonrobot::ComputeOdometryStep;
  using westonrobot::ValidateCommandLateralVelocitySign;
  using westonrobot::ValidateOdometryLinearVelocitySign;
  try {
    Check(ValidateOdometryLinearVelocitySign(1) == 1, "default sign");
    Check(ValidateOdometryLinearVelocitySign(-1) == -1, "reversed sign");
    for (int sign : {0, 2, -2}) {
      bool rejected = false;
      try {
        ValidateOdometryLinearVelocitySign(sign);
      } catch (const std::invalid_argument&) {
        rejected = true;
      }
      Check(rejected, "invalid sign accepted");
    }
    Check(ValidateCommandLateralVelocitySign(1) == 1, "default lateral sign");
    Check(ValidateCommandLateralVelocitySign(-1) == -1, "reversed lateral sign");
    for (int sign : {0, 2, -2}) {
      bool rejected = false;
      try {
        ValidateCommandLateralVelocitySign(sign);
      } catch (const std::invalid_argument&) {
        rejected = true;
      }
      Check(rejected, "invalid lateral sign accepted");
    }
    const auto forward = ComputeOdometryStep(0.02, 0.0, 0.0, 0.0, 1.0);
    Check(Near(forward.dx, 0.02) && Near(forward.linear_speed, 0.02),
          "default forward behavior changed");
    const auto reversed = ComputeOdometryStep(-0.02, 0.0, 0.0, 0.0, 1.0, -1);
    Check(Near(reversed.dx, 0.02) && Near(reversed.linear_speed, 0.02),
          "reversed feedback did not become forward");
    const auto backward = ComputeOdometryStep(0.02, 0.0, 0.0, 0.0, 1.0, -1);
    Check(Near(backward.dx, -0.02), "backward feedback sign");
    const double yaw = std::acos(-1.0) / 2.0;
    const auto rotated = ComputeOdometryStep(-0.02, 0.0, 0.0, yaw, 1.0, -1);
    Check(Near(rotated.dx, 0.0) && Near(rotated.dy, 0.02),
          "correction must precede body-to-world rotation");
    const auto lateral = ComputeOdometryStep(0.0, 0.03, 0.0, yaw, 1.0, -1);
    Check(Near(lateral.dx, -0.03) && Near(lateral.dy, 0.0) &&
              Near(lateral.lateral_speed, 0.03),
          "lateral feedback changed");
    const auto turning = ComputeOdometryStep(-0.02, 0.01, 0.03, 0.37, 0.2, -1);
    Check(Near(turning.dx, (0.02 * std::cos(0.37) - 0.01 * std::sin(0.37)) * 0.2) &&
              Near(turning.dy, (0.02 * std::sin(0.37) + 0.01 * std::cos(0.37)) * 0.2),
          "combined body motion");
    Check(Near(turning.dtheta, 0.006) && Near(turning.angular_speed, 0.03),
          "yaw feedback changed");
    const auto original = ComputeOdometryStep(0.02, 0.01, 0.03, 0.37, 0.2);
    Check(Near(original.dx, turning.dx) && Near(original.dy, turning.dy) &&
              Near(original.dtheta, turning.dtheta),
          "default compatibility");
    const auto stationary = ComputeOdometryStep(-0.02, 0.01, 0.03, 0.37, 0.0, -1);
    Check(Near(stationary.dx, 0.0) && Near(stationary.dy, 0.0) &&
              Near(stationary.dtheta, 0.0),
          "zero time step");
    std::cout << "19 odometry checks passed\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  return 0;
}
