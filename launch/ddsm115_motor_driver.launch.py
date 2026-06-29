from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    default_config_file = PathJoinSubstitution(
        [
            FindPackageShare("dsm115_motor_driver_ros2"),
            "config",
            "dsm115_motor_driver.yaml",
        ]
    )

    config_file = LaunchConfiguration("config_file")

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "config_file",
                default_value=default_config_file,
                description="Path to the DDSM115 driver YAML parameter file.",
            ),
            Node(
                package="ddsm115_motor_driver_ros2",
                executable="ddsm115_motor_driver_node",
                name="ddsm115_motor_driver",
                output="screen",
                parameters=[config_file],
            ),
        ]
    )
