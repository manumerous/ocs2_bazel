/******************************************************************************
Copyright (c) 2026, Manuel Yves Galliker. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#pragma once

#include <map>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>

#include <ocs2_legged_robot/gait/ModeSequenceTemplate.h>
#include <ocs2_ros2_msgs/msg/mode_schedule.hpp>

namespace ocs2 {
namespace legged_robot {

/** Browser-form drop-in replacement for GaitKeyboardPublisher; run() blocks until rclcpp::ok() is false. */
class GaitWebPublisher {
 public:
  GaitWebPublisher(rclcpp::Node::SharedPtr nodeHandle, const std::string& gaitFile, const std::string& robotName, bool verbose = false);

  void run(int port);

 private:
  std::vector<std::string> gaitList_;
  std::map<std::string, ModeSequenceTemplate> gaitMap_;

  rclcpp::Node::SharedPtr node_;
  rclcpp::Publisher<ocs2_ros2_msgs::msg::ModeSchedule>::SharedPtr modeSequenceTemplatePublisher_;
};

}  // namespace legged_robot
}  // namespace ocs2
