/** Copyright 2024 Osprey Robotics - UNF
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 */

#include <chrono>
#include <functional>

#include "teleop_manual_joy.hpp"

using namespace std::chrono_literals;
using std::placeholders::_1;

namespace teleop_manual_joy
{
    TeleopManualJoy::TeleopManualJoy(const rclcpp::NodeOptions &options)
        : Node("teleop_manual_joy"), count_(0)
    {
        // publish to controller specific topics, may combine into unified
        publisher_cmd_vel_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);
        publisher_gpio_ = this->create_publisher<std_msgs::msg::String>("gpio_controller/commands", 10);
        publisher_pos_ = this->create_publisher<std_msgs::msg::Float64MultiArray>("position_controllers/commands", 10);
        publisher_vel_ = this->create_publisher<std_msgs::msg::Float64MultiArray>("velocity_controllers/commands", 10);

        // subscribe to /joy topic for joystick messages
        subscription_ = this->create_subscription<sensor_msgs::msg::Joy>(
            "joy", rclcpp::QoS(10), std::bind(&TeleopManualJoy::joyCallback, this, _1));
    }

    TeleopManualJoy::~TeleopManualJoy()
    {
    }

    void TeleopManualJoy::joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
    {
        // Initializes with zeros by default.
        auto cmd_vel_msg = std::make_unique<geometry_msgs::msg::Twist>();

        // should move hard coded value to configuration file
        cmd_vel_msg->linear.x = joy_msg->axes[TeleopManualJoy::axes::LEFT_JOY_Y] * 7.612;
        cmd_vel_msg->angular.z = joy_msg->axes[TeleopManualJoy::axes::LEFT_JOY_X] * 7.612;

        publisher_cmd_vel_->publish(std::move(cmd_vel_msg));

        // Initializes with zeros by default.
        auto pos_msg = std::make_unique<std_msgs::msg::Float64MultiArray>();
        pos_msg->data.resize(3);

        // should move hard coded value to configuration file
        if (joy_msg->buttons[TeleopManualJoy::buttons::A])
        {
            // dump bucket forward
            pos_msg->data[0] = joy_msg->buttons[TeleopManualJoy::buttons::A] * -30.0;
        }
        else if (joy_msg->buttons[TeleopManualJoy::buttons::B])
        {
            // dump bucket backward
            pos_msg->data[0] = joy_msg->buttons[TeleopManualJoy::buttons::B] * 30.0;
        }
        else
        {
            // dump bucket stop
            pos_msg->data[0] = 0;
        }

        // should move hard coded value to configuration file
        if (joy_msg->buttons[TeleopManualJoy::buttons::X])
        {
            // linear actuator forward
            pos_msg->data[1] = joy_msg->buttons[TeleopManualJoy::buttons::X];
        }
        else if (joy_msg->buttons[TeleopManualJoy::buttons::Y])
        {
            // linear actuator backward
            pos_msg->data[1] = -joy_msg->buttons[TeleopManualJoy::buttons::Y];
        }
        else
        {
            // linear actuator stop
            pos_msg->data[1] = 0;
        }

        // should move hard coded value to configuration file
        if (joy_msg->buttons[TeleopManualJoy::buttons::LEFT_BUMPER])
        {
            // bucket ladder up
            pos_msg->data[2] = joy_msg->buttons[TeleopManualJoy::buttons::LEFT_BUMPER] * 30.0;
        }
        else if (joy_msg->buttons[TeleopManualJoy::buttons::RIGHT_BUMPER])
        {
            // bucket ladder down
            pos_msg->data[2] = joy_msg->buttons[TeleopManualJoy::buttons::RIGHT_BUMPER] * -30.0;
        }
        else
        {
            // bucket ladder stop
            pos_msg->data[2] = 0;
        }

        publisher_pos_->publish(std::move(pos_msg));

        // Initializes with zeros by default.
        auto vel_msg = std::make_unique<std_msgs::msg::Float64MultiArray>();
        vel_msg->data.resize(1);

        // should move hard coded value to configuration file
        if (joy_msg->axes[TeleopManualJoy::axes::LEFT_TRIGGER] < 0)
        {
            // bucket ladder buckets forward
            vel_msg->data[0] = joy_msg->axes[TeleopManualJoy::axes::LEFT_TRIGGER] * -45.0;
        }
        else 
        if (joy_msg->axes[TeleopManualJoy::axes::RIGHT_TRIGGER] < 0)
        {
            // bucket ladder buckets reverse
            vel_msg->data[0] = joy_msg->axes[TeleopManualJoy::axes::RIGHT_TRIGGER] * 45.0;
        }
        else
        {
            // bucket ladder buckets stop
            vel_msg->data[0] = 0;
        }

        publisher_vel_->publish(std::move(vel_msg));

    }
}
