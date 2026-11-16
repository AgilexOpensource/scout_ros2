# scout_ros2

[English](README.md)

AgileX Scout 系列移动底盘的 ROS 2 功能包。本分支面向 ROS 2 Humble。

| 功能包 | 说明 |
| --- | --- |
| scout_base | 基于 [ugv_sdk](https://github.com/westonrobot/ugv_sdk) 的 ROS 2 驱动节点：下发运动指令，发布里程计和状态 |
| scout_description | 底盘的 URDF 模型 |
| scout_msgs | Scout 消息定义 |

## 支持的硬件

- Scout
- Scout Mini
- Scout Mini Omni

支持 V1 和 V2 两种协议，仅限 CAN 接口。协议版本自动识别，只需选择与底盘型号对应的 launch 文件。

## 安装

把功能包克隆到 colcon 工作区并编译：

```bash
mkdir -p ~/ros2_ws/src && cd ~/ros2_ws/src
git clone https://github.com/westonrobot/ugv_sdk.git
git clone -b humble https://github.com/AgilexOpensource/scout_ros2.git
cd ~/ros2_ws
source /opt/ros/humble/setup.bash
colcon build
source install/setup.bash
```

## 使用

1. 启用 CAN 接口：

   ```bash
   sudo ip link set can0 up type can bitrate 500000
   ```

2. 用与底盘型号对应的 launch 文件启动驱动：

   ```bash
   ros2 launch scout_base scout_base.launch.py
   ```

   | launch 文件 | 底盘 |
   | --- | --- |
   | `scout_base.launch.py` | Scout |
   | `scout_mini_base.launch.py` | Scout Mini |
   | `scout_mini_omni_base.launch.py` | Scout Mini Omni |

3. 控制底盘运动，例如使用键盘：

   ```bash
   ros2 run teleop_twist_keyboard teleop_twist_keyboard
   ```

## 接口

| 话题 | 类型 | 方向 | 说明 |
| --- | --- | --- | --- |
| `/cmd_vel` | `geometry_msgs/msg/Twist` | 订阅 | 运动指令。`linear.y` 仅全向底盘使用 |
| `/light_control` | `scout_msgs/msg/ScoutLightCmd` | 订阅 | 灯光指令 |
| `odom` | `nav_msgs/msg/Odometry` | 发布 | 里程计；话题名由 `odom_topic_name` 设置 |
| `/scout_status` | `scout_msgs/msg/ScoutStatus` | 发布 | 底盘状态和原始速度 |
| `/rc_status` | `scout_msgs/msg/ScoutRCState` | 发布 | 遥控器状态 |

节点同时广播 TF 变换 `odom_frame` → `base_frame`。

## 参数

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `port_name` | `can0` | CAN 接口名 |
| `odom_frame` | `odom` | 里程计坐标系 |
| `base_frame` | `base_link` | 底盘坐标系 |
| `odom_topic_name` | `odom` | 里程计话题名 |
| `is_scout_mini` | `false` | Scout Mini 底盘 |
| `is_omni_wheel` | `false` | Scout Mini Omni 底盘 |
| `simulated_robot` | `false` | 仿真模式：运动和灯光指令不下发到底盘 |
| `control_rate` | `50` | 仿真模式下的循环频率，Hz |
| `odom_linear_velocity_sign` | `1` | 纵向里程计反馈的符号，见下文 |
| `cmd_lateral_velocity_sign` | `1` | 全向底盘横向指令的符号，见下文 |

各 launch 文件已按对应底盘设置 `is_scout_mini` 和 `is_omni_wheel`，并把除两个符号参数以外的参数都开放为启动参数。符号参数需通过参数文件或命令行设置：

```bash
ros2 run scout_base scout_base_node --ros-args \
  -p is_scout_mini:=true -p is_omni_wheel:=true \
  -p odom_linear_velocity_sign:=-1 -p cmd_lateral_velocity_sign:=-1
```

两个符号参数都是只读整数，只在启动时读取，仅接受 `1` 和 `-1`。

### odom_linear_velocity_sign

确认硬件上报的纵向反馈与机器人前进方向相反后，才使用 `-1`。

- 符号作用于积分前的机体纵向速度。里程计位姿、里程计 twist 和 TF 位姿都使用修正后的值。
- 横向和 yaw 反馈、运动指令、`/scout_status` 中的原始速度、CAN 反馈频率以及 50 Hz 的状态和里程计循环都不变。
- 这不是世界 X 轴的镜像：yaw 不为零时前进也会改变世界 Y。
- 指令方向需单独确认，不能仅因里程计反馈反向就反转指令。
- 重启驱动会重置里程计原点。

### cmd_lateral_velocity_sign

用于全向底盘（`is_omni_wheel: true`）。确认 `cmd_vel` 的 `linear.y` 为正时机器人向右移动后，才使用 `-1`。

- 符号在下发前乘到横向速度指令上。
- 纵向和 yaw 指令以及所有反馈都不变。

## 停止节点

收到 SIGINT 或 SIGTERM 后，状态与控制循环在 ROS context 关闭时停止，节点随后释放 ROS 资源。停止节点前，请先停止指令来源并让底盘停稳。进程正常退出不等于硬件急停。

## 许可证

见 [LICENSE](LICENSE)。
