/** Copyright 2024 Osprey Robotics - UNF
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 */

#include "gpio_controller.hpp"

namespace gpio_controller
{
    controller_interface::CallbackReturn GPIOController::on_init()
    {
        try
        {
            auto_declare<std::vector<std::string>>("inputs", std::vector<std::string>());
            auto_declare<std::vector<std::string>>("outputs", std::vector<std::string>());
        }
        catch (const std::exception &e)
        {
            fprintf(stderr, "Exception thrown during init stage with message: %s \n", e.what());
            return controller_interface::CallbackReturn::ERROR;
        }
        return controller_interface::CallbackReturn::SUCCESS;
    }

    controller_interface::InterfaceConfiguration GPIOController::command_interface_configuration() const
    {
        controller_interface::InterfaceConfiguration config;
        config.type = controller_interface::interface_configuration_type::INDIVIDUAL;
        config.names = outputs_;

        return config;
    }

    controller_interface::InterfaceConfiguration GPIOController::state_interface_configuration() const
    {
        controller_interface::InterfaceConfiguration config;
        config.type = controller_interface::interface_configuration_type::INDIVIDUAL;
        config.names = inputs_;

        return config;
    }

    controller_interface::return_type GPIOController::update(
        const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
    {
        // send inputs
        for (size_t i = 0; i < state_interfaces_.size(); i++)
        {
            RCLCPP_DEBUG(get_node()->get_logger(),
                         "%s: (%f)",
                         state_interfaces_[i].get_name().c_str(),
                         state_interfaces_[i].get_value());
            gpio_msg_.values.at(i) = static_cast<float>(state_interfaces_.at(i).get_value());
        }
        gpio_publisher_->publish(gpio_msg_);

        // set outputs
        if (!output_cmd_ptr_)
        {
            // no command received yet
            return controller_interface::return_type::OK;
        }
        if (output_cmd_ptr_->data.size() != command_interfaces_.size())
        {
            RCLCPP_ERROR_THROTTLE(get_node()->get_logger(),
                                  *(get_node()->get_clock()), 1000,
                                  "command size (%zu) does not match number of interfaces (%zu)",
                                  output_cmd_ptr_->data.size(),
                                  command_interfaces_.size());
            return controller_interface::return_type::ERROR;
        }

        for (size_t i = 0; i < command_interfaces_.size(); i++)
        {
            command_interfaces_[i].set_value(output_cmd_ptr_->data[i]);
            RCLCPP_DEBUG(get_node()->get_logger(),
                         "%s: (%f)",
                         command_interfaces_[i].get_name().c_str(),
                         command_interfaces_[i].get_value());
        }

        return controller_interface::return_type::OK;
    }

    controller_interface::CallbackReturn GPIOController::on_configure(
        const rclcpp_lifecycle::State & /*previous_state*/)
    {
        try
        {
            inputs_ = get_node()->get_parameter("inputs").as_string_array();
            outputs_ = get_node()->get_parameter("outputs").as_string_array();

            initMsgs();

            // register publisher
            gpio_publisher_ = get_node()->create_publisher<control_msgs::msg::InterfaceValue>(
                "~/inputs", rclcpp::SystemDefaultsQoS()
            );

            // register subscriber
            subscription_command_ = get_node()->create_subscription<CmdType>(
                "~/commands", rclcpp::SystemDefaultsQoS(),
                [this](const CmdType::SharedPtr msg)
                { output_cmd_ptr_ = msg; }
            );
        }
        catch (...)
        {
            return LifecycleNodeInterface::CallbackReturn::ERROR;
        }
        return LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    void GPIOController::initMsgs()
    {
        gpio_msg_.interface_names = inputs_;
        gpio_msg_.values.resize(inputs_.size());
    }

    controller_interface::CallbackReturn GPIOController::on_activate(
        const rclcpp_lifecycle::State & /*previous_state*/)
    {
        return LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

    controller_interface::CallbackReturn GPIOController::on_deactivate(
        const rclcpp_lifecycle::State & /*previous_state*/)
    {
        try
        {
            // reset publisher
            gpio_publisher_.reset();
        }
        catch (...)
        {
            return LifecycleNodeInterface::CallbackReturn::ERROR;
        }
        return LifecycleNodeInterface::CallbackReturn::SUCCESS;
    }

} 

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(gpio_controller::GPIOController,
                       controller_interface::ControllerInterface)
