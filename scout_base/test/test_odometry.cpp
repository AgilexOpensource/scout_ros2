#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

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
  using westonrobot::IsFeedbackFresh;
  using westonrobot::ValidateFeedbackTimeout;
  using westonrobot::ValidateVelocitySigns;
  using westonrobot::VelocitySigns;
  try {
    const auto parsed = ValidateVelocitySigns({1, -1, -1}, "signs");
    Check(parsed.x == 1 && parsed.y == -1 && parsed.wz == -1, "parsed signs");
    for (const std::vector<int64_t> &bad :
         {std::vector<int64_t>{1, 1}, std::vector<int64_t>{1, 1, 1, 1},
          std::vector<int64_t>{0, 1, 1}, std::vector<int64_t>{1, 2, 1},
          std::vector<int64_t>{1, 1, -2}}) {
      bool rejected = false;
      try {
        ValidateVelocitySigns(bad, "signs");
      } catch (const std::invalid_argument &) {
        rejected = true;
      }
      Check(rejected, "invalid signs accepted");
    }
    const auto forward = ComputeOdometryStep(0.02, 0.0, 0.0, 0.0, 1.0);
    Check(Near(forward.dx, 0.02) && Near(forward.linear_speed, 0.02),
          "default forward behavior changed");
    const VelocitySigns flip_x{-1, 1, 1};
    const auto reversed = ComputeOdometryStep(-0.02, 0.0, 0.0, 0.0, 1.0, flip_x);
    Check(Near(reversed.dx, 0.02) && Near(reversed.linear_speed, 0.02),
          "reversed feedback did not become forward");
    const double yaw = std::acos(-1.0) / 2.0;
    const auto rotated = ComputeOdometryStep(-0.02, 0.0, 0.0, yaw, 1.0, flip_x);
    Check(Near(rotated.dx, 0.0) && Near(rotated.dy, 0.02),
          "correction must precede body-to-world rotation");
    const auto lateral = ComputeOdometryStep(0.0, 0.03, 0.0, yaw, 1.0, flip_x);
    Check(Near(lateral.dx, -0.03) && Near(lateral.dy, 0.0) &&
              Near(lateral.lateral_speed, 0.03),
          "x sign must not change lateral feedback");
    const auto flip_y =
        ComputeOdometryStep(0.0, 0.03, 0.0, 0.0, 1.0, VelocitySigns{1, -1, 1});
    Check(Near(flip_y.dy, -0.03) && Near(flip_y.lateral_speed, -0.03),
          "y sign must flip lateral feedback");
    const auto flip_wz = ComputeOdometryStep(0.02, 0.0, 0.03, 0.0, 0.2,
                                             VelocitySigns{1, 1, -1});
    Check(Near(flip_wz.angular_speed, -0.03) && Near(flip_wz.dtheta, -0.006) &&
              Near(flip_wz.dx, 0.004),
          "wz sign must flip yaw feedback only");
    const auto turning =
        ComputeOdometryStep(-0.02, 0.01, 0.03, 0.37, 0.2, flip_x);
    const auto original = ComputeOdometryStep(0.02, 0.01, 0.03, 0.37, 0.2);
    Check(Near(original.dx, turning.dx) && Near(original.dy, turning.dy) &&
              Near(original.dtheta, turning.dtheta),
          "combined body motion");
    const auto stationary =
        ComputeOdometryStep(-0.02, 0.01, 0.03, 0.37, 0.0, VelocitySigns{-1, -1, -1});
    Check(Near(stationary.dx, 0.0) && Near(stationary.dy, 0.0) &&
              Near(stationary.dtheta, 0.0),
          "zero time step");
    using Clock = std::chrono::steady_clock;
    const Clock::time_point now{std::chrono::seconds(100)};
    Check(IsFeedbackFresh(now - std::chrono::milliseconds(20), now, 0.5),
          "recent feedback is fresh");
    Check(!IsFeedbackFresh(now - std::chrono::milliseconds(600), now, 0.5),
          "old feedback is stale");
    Check(!IsFeedbackFresh(Clock::time_point{}, now, 0.5),
          "no feedback yet is stale");
    Check(ValidateFeedbackTimeout(0.5) == 0.5, "valid feedback timeout");
    for (double timeout : {0.0, -1.0, std::nan("")}) {
      bool rejected = false;
      try {
        ValidateFeedbackTimeout(timeout);
      } catch (const std::invalid_argument&) {
        rejected = true;
      }
      Check(rejected, "invalid feedback timeout accepted");
    }
    std::cout << "odometry checks passed\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  return 0;
}
