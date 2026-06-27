#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "dsm115_motor_driver_ros2/ddsm115_communicator.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"

namespace
{
constexpr char kDefaultSerialPort[] = "/dev/ttyUSB0";

double velocity_to_rpm(double velocity)
{
  return velocity / (M_PI * 2.0) * 60.0;
}

double rpm_to_velocity(double rpm)
{
  return rpm / 60.0 * (M_PI * 2.0);
}

template<typename T>
bool values_are_distinct(const std::vector<T> & values)
{
  const std::unordered_set<T> unique_values(values.begin(), values.end());
  return unique_values.size() == values.size();
}
}  // namespace

class Dsm115MotorDriverNode : public rclcpp::Node
{
public:
  Dsm115MotorDriverNode()
  : Node("dsm115_motor_driver")
  {
    const auto port_name = declare_parameter<std::string>("port_name", kDefaultSerialPort);
    wheel_names_ =
      declare_parameter<std::vector<std::string>>("wheel_names", std::vector<std::string>{});
    const auto wheel_ids =
      declare_parameter<std::vector<int64_t>>("wheel_ids", std::vector<int64_t>{});
    const auto wheel_directions =
      declare_parameter<std::vector<std::string>>(
      "wheel_directions", std::vector<std::string>{});

    validate_parameters(wheel_names_, wheel_ids, wheel_directions);

    wheel_ids_.reserve(wheel_ids.size());
    wheel_directions_.reserve(wheel_directions.size());

    for (size_t i = 0; i < wheel_ids.size(); ++i) {
      wheel_ids_.push_back(static_cast<int>(wheel_ids[i]));
      wheel_directions_.push_back(wheel_directions[i] == "forward" ? 1 : -1);
    }

    communicator_ = std::make_unique<ddsm115::Communicator>(port_name);
    if (communicator_->state() != ddsm115::State::normal) {
      throw std::runtime_error("Failed to initialize DDSM115 communication");
    }

    configure_wheels();
    RCLCPP_INFO(get_logger(), "DDSM115 motor driver is ready");
  }

  ~Dsm115MotorDriverNode() override
  {
    stop_all_wheels();
  }

private:
  void validate_parameters(
    const std::vector<std::string> & wheel_names,
    const std::vector<int64_t> & wheel_ids,
    const std::vector<std::string> & wheel_directions) const
  {
    if (wheel_names.empty()) {
      throw std::runtime_error("wheel_names parameter must contain at least one wheel");
    }

    if (wheel_names.size() != wheel_ids.size() || wheel_ids.size() != wheel_directions.size()) {
      throw std::runtime_error("wheel_names, wheel_ids, and wheel_directions must have the same length");
    }

    if (!values_are_distinct(wheel_names)) {
      throw std::runtime_error("wheel_names values must be unique");
    }

    if (!values_are_distinct(wheel_ids)) {
      throw std::runtime_error("wheel_ids values must be unique");
    }

    for (size_t i = 0; i < wheel_names.size(); ++i) {
      if (wheel_names[i].empty()) {
        throw std::runtime_error("wheel_names cannot contain empty strings");
      }
      if (wheel_ids[i] <= 0 || wheel_ids[i] > 255) {
        throw std::runtime_error("wheel_ids must be in the range 1..255");
      }
      if (wheel_directions[i] != "forward" && wheel_directions[i] != "backward") {
        throw std::runtime_error("wheel_directions values must be 'forward' or 'backward'");
      }
    }
  }

  void configure_wheels()
  {
    velocity_subscriptions_.reserve(wheel_names_.size());
    velocity_publishers_.reserve(wheel_names_.size());
    angle_publishers_.reserve(wheel_names_.size());
    current_publishers_.reserve(wheel_names_.size());

    for (size_t i = 0; i < wheel_names_.size(); ++i) {
      const auto & wheel_name = wheel_names_[i];
      RCLCPP_INFO(
        get_logger(), "Adding wheel %s with id %d", wheel_name.c_str(), wheel_ids_[i]);

      velocity_publishers_.push_back(
        create_publisher<std_msgs::msg::Float64>("/" + wheel_name + "/current_velocity", 10));
      angle_publishers_.push_back(
        create_publisher<std_msgs::msg::Float64>("/" + wheel_name + "/angle", 10));
      current_publishers_.push_back(
        create_publisher<std_msgs::msg::Float64>("/" + wheel_name + "/current", 10));

      velocity_subscriptions_.push_back(
        create_subscription<std_msgs::msg::Float64>(
          "/" + wheel_name + "/target_velocity",
          10,
          [this, i](std_msgs::msg::Float64::SharedPtr msg) {
            handle_target_velocity(i, msg->data);
          }));

      if (!communicator_->set_wheel_mode(wheel_ids_[i], ddsm115::Mode::velocity_loop)) {
        throw std::runtime_error("Failed to set velocity mode for wheel " + wheel_name);
      }
    }
  }

  void handle_target_velocity(size_t wheel_index, double target_velocity)
  {
    const auto response = communicator_->set_wheel_rpm(
      wheel_ids_[wheel_index],
      velocity_to_rpm(target_velocity) * wheel_directions_[wheel_index]);

    if (response.result != ddsm115::State::normal) {
      RCLCPP_DEBUG(
        get_logger(), "No valid feedback from wheel %s", wheel_names_[wheel_index].c_str());
      return;
    }

    std_msgs::msg::Float64 velocity_msg;
    std_msgs::msg::Float64 angle_msg;
    std_msgs::msg::Float64 current_msg;

    velocity_msg.data = rpm_to_velocity(response.velocity) * wheel_directions_[wheel_index];
    angle_msg.data =
      std::round(
      response.position * (2.0 * M_PI / 360.0) * wheel_directions_[wheel_index] * 100.0) /
      100.0 * -1.0;
    current_msg.data = response.current;

    velocity_publishers_[wheel_index]->publish(velocity_msg);
    angle_publishers_[wheel_index]->publish(angle_msg);
    current_publishers_[wheel_index]->publish(current_msg);
  }

  void stop_all_wheels()
  {
    if (!communicator_) {
      return;
    }

    RCLCPP_INFO(get_logger(), "Stopping DDSM115 wheels");
    for (const auto wheel_id : wheel_ids_) {
      communicator_->set_wheel_rpm(wheel_id, 0.0);
    }
    communicator_->disconnect();
  }

  std::vector<std::string> wheel_names_;
  std::vector<int> wheel_ids_;
  std::vector<int> wheel_directions_;
  std::unique_ptr<ddsm115::Communicator> communicator_;

  std::vector<rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr> velocity_subscriptions_;
  std::vector<rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr> velocity_publishers_;
  std::vector<rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr> angle_publishers_;
  std::vector<rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr> current_publishers_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  try {
    auto node = std::make_shared<Dsm115MotorDriverNode>();
    rclcpp::spin(node);
    node.reset();
  } catch (const std::exception & error) {
    RCLCPP_FATAL(rclcpp::get_logger("dsm115_motor_driver"), "%s", error.what());
    rclcpp::shutdown();
    return 1;
  }

  rclcpp::shutdown();
  return 0;
}
