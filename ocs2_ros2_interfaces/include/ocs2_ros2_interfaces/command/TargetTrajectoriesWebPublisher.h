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

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>

#include <ocs2_mpc/SystemObservation.h>
#include <ocs2_ros2_interfaces/command/TargetTrajectoriesRosPublisher.h>

namespace ocs2 {

/** Browser-form drop-in replacement for TargetTrajectoriesKeyboardPublisher; run() blocks until rclcpp::ok() is false. */
class TargetTrajectoriesWebPublisher final {
 public:
  using CommandLineToTargetTrajectories =
      std::function<TargetTrajectories(const vector_t& commadLineTarget, const SystemObservation& observation)>;

  TargetTrajectoriesWebPublisher(rclcpp::Node::SharedPtr node, const std::string& topicPrefix, const scalar_array_t& targetCommandLimits,
                                 std::vector<std::string> fieldNames, CommandLineToTargetTrajectories commandLineToTargetTrajectoriesFun);

  size_t targetCommandSize() const { return targetCommandLimits_.size(); }

  void run(int port, const std::string& promptText = "Enter command");

 private:
  const vector_t targetCommandLimits_;
  const std::vector<std::string> fieldNames_;
  CommandLineToTargetTrajectories commandLineToTargetTrajectoriesFun_;

  std::unique_ptr<TargetTrajectoriesRosPublisher> targetTrajectoriesPublisherPtr_;

  rclcpp::Subscription<ocs2_ros2_msgs::msg::MpcObservation>::SharedPtr observationSubscriber_;
  mutable std::mutex latestObservationMutex_;
  SystemObservation latestObservation_;
  rclcpp::Node::SharedPtr node_;
};

}  // namespace ocs2
