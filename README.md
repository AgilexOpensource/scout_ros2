# ROS2 Packages for Scout Mobile Robot

## Packages

This repository contains minimal packages to control the scout robot using ROS. 

* scout_base: a ROS wrapper around [ugv_sdk](https://github.com/westonrobot/ugv_sdk) to monitor and control the scout robot
* scout_description: URDF model for the mobile base
* scout_msgs: scout related message definitions

## Stopping the Node

On SIGINT or SIGTERM, the state/control loop stops when the ROS context closes, and the node releases its ROS resources without recreating an executor after shutdown. Stop teleoperation command sources and bring the base to rest before stopping the node; a clean process exit is not a hardware emergency stop.

## Longitudinal Odometry Feedback

`odom_linear_velocity_sign` is a startup-only, read-only integer parameter. Its default is `+1`; only `+1` and `-1` are accepted. Use `-1` only after verifying that the hardware longitudinal feedback is reversed relative to the robot's forward direction. It adjusts the body-frame longitudinal velocity before odometry integration and the published odometry twist; the TF pose uses the same corrected integration. Lateral and yaw feedback are unchanged. This is not a world-X reflection: forward travel at nonzero yaw also affects world Y.

Motion commands, raw `scout_status` velocities, CAN feedback rates and the 50 Hz state/odometry loop are unchanged. Verify command direction independently; do not invert commands merely because odometry feedback is reversed. The verified Lookie Trio Revo2 profile sets this parameter to `-1`; other profiles retain the default. Restarting the driver resets the odometry origin.

## Lateral Command Direction

`cmd_lateral_velocity_sign` is a startup-only, read-only integer parameter for omni bases (`is_omni_wheel: true`). Its default is `+1`; only `+1` and `-1` are accepted. Use `-1` only after verifying that a positive `cmd_vel` `linear.y` moves the robot to its right. It multiplies the commanded lateral velocity before it is sent to the base; longitudinal and yaw commands and all feedback are unchanged. The verified Lookie Trio Revo2 profile sets this parameter to `-1` (positive `linear.y` moved the base right while the lateral feedback correctly reported the motion as negative).

## 纵向里程计反馈

`odom_linear_velocity_sign` 是启动时设置的只读整数参数，默认 `+1`，仅接受 `+1` 或 `-1`。确认硬件纵向反馈与机器人前向相反后，才使用 `-1`。修正作用于进入积分前的机体纵向速度及里程计 twist，TF 位姿使用同一积分结果；横向和 yaw 反馈不变。这不是世界 X 镜像，非零 yaw 下的前进也会改变世界 Y。

运动命令、`scout_status` 原始速度、CAN 反馈频率及 50 Hz 状态/里程计循环均不变。命令方向必须独立确认，不能因为里程计反向就同时反转命令。已验证的 Lookie Trio Revo2 配置设为 `-1`，其他配置保持默认。驱动重启会重置里程计原点。

## 横向运动命令方向

`cmd_lateral_velocity_sign` 是启动时设置的只读整数参数，仅用于全向底盘（`is_omni_wheel: true`），默认 `+1`，仅接受 `+1` 或 `-1`。确认 `cmd_vel` 的 `linear.y` 为正时机器人向右移动后，才使用 `-1`。它在下发前乘到横向速度命令上；纵向、yaw 命令以及所有反馈都不变。已验证的 Lookie Trio Revo2 配置设为 `-1`（`linear.y` 为正时底盘向右移动，而横向反馈正确地报告为负）。

## Supported Hardware

* Scout
* Scout Mini
* Scout Mini Omni

**Note:** Both V1 and V2 protocols are supported by this package (only with CAN interface). You don't have to specify the protocol version when launching the base node but you need to use the right launch file for your specific Scout model.  

## Basic usage of the ROS packages

1. Clone the packages into your colcon workspace and compile

    (the following instructions assume your colcon workspace is at: ~/ros2_ws/src)

    ```
    $ mkdir -p ~/ros2_ws/src
    $ cd ~/ros2_ws/src
    $ git clone https://github.com/westonrobot/ugv_sdk.git
    $ git clone https://github.com/AgilexOpensource/scout_ros2.git
    $ cd ..
    $ colcon build
    ```

2. Launch ROS nodes

* Enable CAN module

    ```
    sudo ip link set can0 up type can bitrate 500000
    ```

* Start the base node with the launch file for your Scout model

    ```
    $ ros2 launch scout_base scout_base.launch.py
    ```

    * `scout_base.launch.py`: Scout
    * `scout_mini_base.launch.py`: Scout Mini
    * `scout_mini_omni_base.launch.py`: Scout Mini Omni

* Start the keyboard tele-op node

    ```
    $ ros2 run teleop_twist_keyboard teleop_twist_keyboard
    ```