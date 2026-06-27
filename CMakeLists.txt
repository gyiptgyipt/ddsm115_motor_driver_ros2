cmake_minimum_required(VERSION 3.8)
project(dsm115_motor_driver_ros2)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(dsm115_motor_driver_node
  src/dsm115_motor_driver_node.cpp
  src/ddsm115_communicator.cpp
)

target_compile_features(dsm115_motor_driver_node PUBLIC cxx_std_17)
target_include_directories(dsm115_motor_driver_node PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
ament_target_dependencies(dsm115_motor_driver_node
  rclcpp
  std_msgs
)

install(TARGETS dsm115_motor_driver_node
  DESTINATION lib/${PROJECT_NAME}
)

install(PROGRAMS scripts/ddsm115_set_id.py
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY config launch
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
