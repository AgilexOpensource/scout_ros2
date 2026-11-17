# scout_ros2

[中文](README.zh-CN.md)

ROS 2 packages for the AgileX Scout mobile bases. This branch targets ROS 2 Humble.

| Package | Description |
| --- | --- |
| scout_base | ROS 2 driver node built on [ugv_sdk](https://github.com/westonrobot/ugv_sdk): sends motion commands and publishes odometry and status |
| scout_description | URDF model of the mobile base |
| scout_msgs | Scout message definitions |

## Supported hardware

- Scout
- Scout Mini
- Scout Mini Omni

Both the V1 and V2 protocols are supported, over CAN only. The protocol version is detected automatically; choose the launch file that matches your base.

## Installation

Clone the packages into a colcon workspace and build:

```bash
mkdir -p ~/ros2_ws/src && cd ~/ros2_ws/src
git clone https://github.com/westonrobot/ugv_sdk.git
git clone -b humble https://github.com/AgilexOpensource/scout_ros2.git
cd ~/ros2_ws
source /opt/ros/humble/setup.bash
colcon build
source install/setup.bash
```

## Usage

1. Bring up the CAN interface:

   ```bash
   sudo ip link set can0 up type can bitrate 500000
   ```

2. Start the driver with the launch file for your base:

   ```bash
   ros2 launch scout_base scout_base.launch.py
   ```

   | Launch file | Base |
   | --- | --- |
   | `scout_base.launch.py` | Scout |
   | `scout_mini_base.launch.py` | Scout Mini |
   | `scout_mini_omni_base.launch.py` | Scout Mini Omni |

3. Drive the base, for example with the keyboard:

   ```bash
   ros2 run teleop_twist_keyboard teleop_twist_keyboard
   ```

## Interfaces

| Topic | Type | Direction | Description |
| --- | --- | --- | --- |
| `/cmd_vel` | `geometry_msgs/msg/Twist` | Subscribed | Motion command. `linear.y` is used only by omni bases |
| `/light_control` | `scout_msgs/msg/ScoutLightCmd` | Subscribed | Light command |
| `odom` | `nav_msgs/msg/Odometry` | Published | Odometry, only while the CAN motion state is fresh; the name is set by `odom_topic_name` |
| `/scout_status` | `scout_msgs/msg/ScoutStatus` | Published | Base status and raw velocities |
| `/rc_status` | `scout_msgs/msg/ScoutRCState` | Published | Remote controller state |

The node also broadcasts the TF transform `odom_frame` → `base_frame`.

## Parameters

| Parameter | Default | Description |
| --- | --- | --- |
| `port_name` | `can0` | CAN interface name |
| `odom_frame` | `odom` | Odometry frame id |
| `base_frame` | `base_link` | Base frame id |
| `odom_topic_name` | `odom` | Odometry topic name |
| `is_scout_mini` | `false` | Scout Mini base |
| `is_omni_wheel` | `false` | Scout Mini Omni base |
| `simulated_robot` | `false` | Simulation mode: motion and light commands are not sent to the base |
| `control_rate` | `50` | Loop rate in simulation mode, Hz |
| `odom_linear_velocity_sign` | `1` | Sign of the longitudinal odometry feedback, see below |
| `cmd_lateral_velocity_sign` | `1` | Sign of the lateral command on omni bases, see below |
| `feedback_timeout` | `0.5` | Seconds without CAN motion state before odometry stops, see below |

The launch files set `is_scout_mini` and `is_omni_wheel` for their base and expose every parameter except the two sign parameters and `feedback_timeout` as launch arguments. Set these in a parameter file or on the command line:

```bash
ros2 run scout_base scout_base_node --ros-args \
  -p is_scout_mini:=true -p is_omni_wheel:=true \
  -p odom_linear_velocity_sign:=-1 -p cmd_lateral_velocity_sign:=-1
```

Both sign parameters are read-only integers that are read at startup. Only `1` and `-1` are accepted.

### odom_linear_velocity_sign

Use `-1` only after verifying that the hardware reports longitudinal feedback opposite to the robot's forward direction.

- The sign is applied to the body-frame longitudinal velocity before odometry integration. The odometry pose, the odometry twist and the TF pose all use the corrected value.
- Lateral and yaw feedback, motion commands, the raw velocities in `/scout_status`, the CAN feedback rates and the 50 Hz state and odometry loop are unchanged.
- It is not a reflection of the world X axis: forward travel at a nonzero yaw also changes world Y.
- Verify the command direction separately. Do not invert commands only because the odometry feedback is reversed.
- Restarting the driver resets the odometry origin.

### cmd_lateral_velocity_sign

Applies to omni bases (`is_omni_wheel: true`). Use `-1` only after verifying that a positive `cmd_vel` `linear.y` moves the robot to its right.

- The sign multiplies the commanded lateral velocity before it is sent to the base.
- Longitudinal and yaw commands and all feedback are unchanged.

### feedback_timeout

The SDK keeps the last received state after the CAN bus goes silent (cable unplugged, base powered off). Odometry and its TF are published only while the motion state was received within `feedback_timeout` seconds, so consumers such as kstone see the base as stale instead of a pose that keeps drifting with the last velocity.

- The log shows `Chassis feedback lost` and `Chassis feedback recovered`.
- The time without feedback is not integrated: after recovery the pose continues from where it stopped.
- `/scout_status` and `/rc_status` are still published from the cached state.
- Read-only, positive number, read at startup.

## Stopping the node

On SIGINT or SIGTERM the state and control loop stops when the ROS context closes, and the node releases its ROS resources. Stop the command sources and bring the base to rest before stopping the node. A clean process exit is not a hardware emergency stop.

## License

See [LICENSE](LICENSE).
