# dsm115_motor_driver_ros2

Simple ROS 2 driver for Waveshare DDSM115 motor wheels connected on an RS485 bus.

This is a ROS 2 port of the ROS 1 `ros_ddsm115_driver` package. It keeps the same basic interface: each wheel has one target velocity topic and three feedback topics.

## Features

- ROS 2 `rclcpp` C++ node
- RS485 serial communication at 115200 baud
- DDSM115 velocity loop mode
- Per-wheel target velocity in `rad/s`
- Per-wheel feedback for velocity, angle, and current
- Safe shutdown command that sends `0 RPM` to every configured wheel

## Build

Clone or place this package inside the `src` folder of a ROS 2 workspace:

```bash
cd ~/ros2_ws
colcon build --packages-select dsm115_motor_driver_ros2
source install/setup.bash
```

If the serial port needs permission, add your user to the `dialout` group and log in again:

```bash
sudo usermod -a -G dialout $USER
```

## Configure

Edit:

```text
config/dsm115_motor_driver.yaml
```

Example:

```yaml
dsm115_motor_driver:
  ros__parameters:
    port_name: "/dev/ttyUSB0"
    wheel_names:
      - "front_left_wheel"
      - "front_right_wheel"
      - "rear_left_wheel"
      - "rear_right_wheel"
    wheel_ids: [1, 2, 3, 4]
    wheel_directions:
      - "forward"
      - "backward"
      - "forward"
      - "backward"
```

Parameters:

| Name | Type | Description |
| --- | --- | --- |
| `port_name` | string | RS485 serial device, for example `/dev/ttyUSB0`. |
| `wheel_names` | string array | Wheel names used to create ROS topics. Values must be unique. |
| `wheel_ids` | integer array | DDSM115 IDs configured on the motors. Values must be unique and in `1..255`. |
| `wheel_directions` | string array | Use `forward` or `backward`. `backward` reverses commands and velocity/angle feedback. |

The three wheel arrays must have the same length.

## Run

Use the included launch file:

```bash
ros2 launch dsm115_motor_driver_ros2 dsm115_motor_driver.launch.py
```

Use a custom config file:

```bash
ros2 launch dsm115_motor_driver_ros2 dsm115_motor_driver.launch.py \
  config_file:=/path/to/my_dsm115_config.yaml
```

Or run the node directly:

```bash
ros2 run dsm115_motor_driver_ros2 dsm115_motor_driver_node \
  --ros-args --params-file config/dsm115_motor_driver.yaml
```

## Topics

For each configured wheel name, the node creates these topics:

| Topic | Type | Direction | Description |
| --- | --- | --- | --- |
| `/<wheel_name>/target_velocity` | `std_msgs/msg/Float64` | Subscribe | Target wheel angular velocity in `rad/s`. |
| `/<wheel_name>/current_velocity` | `std_msgs/msg/Float64` | Publish | Measured wheel angular velocity in `rad/s`. |
| `/<wheel_name>/angle` | `std_msgs/msg/Float64` | Publish | Wheel angle in `rad`. |
| `/<wheel_name>/current` | `std_msgs/msg/Float64` | Publish | Motor current feedback. |

Example command:

```bash
ros2 topic pub /front_left_wheel/target_velocity std_msgs/msg/Float64 "{data: 3.0}" -r 10
```

Stop a wheel:

```bash
ros2 topic pub /front_left_wheel/target_velocity std_msgs/msg/Float64 "{data: 0.0}" --once
```

## Notes

- This driver only implements velocity control mode.
- DDSM115 wheels keep the last command if control messages stop. The node sends `0 RPM` to all configured wheels during normal shutdown, but it cannot protect against hardware, bus, or power failures.
- Occasional RS485 timeouts can happen. The node ignores a bad feedback packet and waits for the next command.
