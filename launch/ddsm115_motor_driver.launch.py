from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    default_robot_description_file = PathJoinSubstitution(
        [
            FindPackageShare("ddsm115_motor_driver_ros2"),
            "description",
            "urdf",
            "ddsm115_base.urdf",
        ]
    )
    default_controllers_file = PathJoinSubstitution(
        [
            FindPackageShare("ddsm115_motor_driver_ros2"),
            "config",
            "ros2_control.yaml",
        ]
    )
    default_twist_mux_file = PathJoinSubstitution(
        [
            FindPackageShare("ddsm115_motor_driver_ros2"),
            "config",
            "twist_mux.yaml",
        ]
    )

    robot_description_file = LaunchConfiguration("robot_description_file")
    controllers_file = LaunchConfiguration("controllers_file")
    twist_mux_file = LaunchConfiguration("twist_mux_file")

    robot_description = {
        "robot_description": ParameterValue(
            Command(["cat ", robot_description_file]),
            value_type=str,
        )
    }

    robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        output="screen",
        parameters=[robot_description],
    )

    ros2_control_node = Node(
        package="controller_manager",
        executable="ros2_control_node",
        output="screen",
        parameters=[robot_description, controllers_file],
    )

    controller_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=[
            "joint_state_broadcaster",
            "diff_drive_controller",
            "--controller-manager",
            "/controller_manager",
            "--activate-as-group",
        ],
        output="screen",
    )

    twist_mux = Node(
        package="twist_mux",
        executable="twist_mux",
        name="twist_mux",
        output="screen",
        parameters=[twist_mux_file],
        remappings=[("cmd_vel_out", "/diff_drive_controller/cmd_vel_unstamped")],
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "robot_description_file",
                default_value=default_robot_description_file,
                description="Path to the robot URDF containing the ros2_control hardware block.",
            ),
            DeclareLaunchArgument(
                "controllers_file",
                default_value=default_controllers_file,
                description="Path to ros2_control controller configuration.",
            ),
            DeclareLaunchArgument(
                "twist_mux_file",
                default_value=default_twist_mux_file,
                description="Path to twist_mux configuration.",
            ),
            robot_state_publisher,
            ros2_control_node,
            controller_spawner,
            twist_mux,
        ]
    )
