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
| `odom` | `nav_msgs/msg/Odometry` | 发布 | 里程计，仅在 CAN 运动状态新鲜时发布；话题名由 `odom_topic_name` 设置 |
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
| `cmd_velocity_signs` | `[1, 1, 1]` | 运动指令各轴 `[x, y, wz]` 的符号，见下文 |
| `odom_velocity_signs` | `[1, 1, 1]` | 里程计反馈各轴 `[x, y, wz]` 的符号，见下文 |
| `feedback_timeout` | `0.5` | 超过该秒数未收到 CAN 运动状态时停发里程计，见下文 |

各 launch 文件已按对应底盘设置 `is_scout_mini` 和 `is_omni_wheel`，并把除两个符号参数和 `feedback_timeout` 以外的参数都开放为启动参数。这三个参数需通过参数文件或命令行设置：

```bash
ros2 run scout_base scout_base_node --ros-args \
  -p is_scout_mini:=true -p is_omni_wheel:=true \
  -p cmd_velocity_signs:='[1, -1, 1]' -p odom_velocity_signs:='[-1, 1, 1]'
```

### 速度符号

底盘因电机接线或固件导致某个轴反向时使用。两个参数都是只读的三元整数列表 `[x, y, wz]`（对应 `linear.x`、`linear.y`、`angular.z`），启动时读取。每项只能是 `1` 或 `-1`，否则节点启动失败。

- `cmd_velocity_signs` 在下发前乘到 `/cmd_vel` 上。`y` 只对全向底盘生效。
- `odom_velocity_signs` 在积分前乘到机体系速度反馈上。里程计位姿、里程计 twist 和 TF 位姿都使用修正后的值。它不是世界坐标轴的镜像：修正发生在机体系转到世界系之前。
- `/scout_status` 中的原始速度不变。

每个轴的指令和反馈要分别确认：发一个小的正值观察机器人（前进、向左、俯视逆时针为正），再对比里程计 twist，只翻转出错的一侧。重启驱动会重置里程计原点。

### feedback_timeout

CAN 总线没有数据后（线缆拔出、底盘断电），SDK 仍保留最后收到的状态。只有在 `feedback_timeout` 秒内收到过运动状态时才发布里程计及其 TF，这样 kstone 等使用方会看到底盘反馈过期，而不是一个按最后速度不断漂移的位姿。

- 日志显示 `Chassis feedback lost` 和 `Chassis feedback recovered`。
- 无反馈期间不积分：恢复后位姿从停下的位置继续。
- `/scout_status` 和 `/rc_status` 仍按缓存状态发布。
- 只读，正数，启动时读取。

## 停止节点

收到 SIGINT 或 SIGTERM 后，状态与控制循环在 ROS context 关闭时停止，节点随后释放 ROS 资源。停止节点前，请先停止指令来源并让底盘停稳。进程正常退出不等于硬件急停。

## 许可证

见 [LICENSE](LICENSE)。
