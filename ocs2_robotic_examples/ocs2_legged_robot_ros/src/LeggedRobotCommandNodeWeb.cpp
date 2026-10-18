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

// Combines LeggedRobotPoseCommandNodeWeb and LeggedRobotGaitCommandNodeWeb's forms onto a single
// httplib::Server/page instead of two separate ports -- one browser tab for both pose and gait commands.

#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include <httplib.h>

#include "ament_index_cpp/get_package_share_directory.hpp"
#include "rclcpp/rclcpp.hpp"

#include <ocs2_core/Types.h>
#include <ocs2_core/misc/LoadData.h>
#include <ocs2_ros2_interfaces/command/HtmlFormUtils.h>
#include <ocs2_ros2_interfaces/command/TargetTrajectoriesWebPublisher.h>

#include "ocs2_legged_robot_ros/gait/GaitWebPublisher.h"

using namespace ocs2;
using namespace legged_robot;

namespace {
scalar_t targetDisplacementVelocity;
scalar_t targetRotationVelocity;
scalar_t comHeight;
vector_t defaultJointState(12);
}  // namespace

scalar_t estimateTimeToTarget(const vector_t& desiredBaseDisplacement) {
  const scalar_t& dx = desiredBaseDisplacement(0);
  const scalar_t& dy = desiredBaseDisplacement(1);
  const scalar_t& dyaw = desiredBaseDisplacement(3);
  const scalar_t rotationTime = std::abs(dyaw) / targetRotationVelocity;
  const scalar_t displacement = std::sqrt(dx * dx + dy * dy);
  const scalar_t displacementTime = displacement / targetDisplacementVelocity;
  return std::max(rotationTime, displacementTime);
}

TargetTrajectories commandLineToTargetTrajectories(const vector_t& commadLineTarget, const SystemObservation& observation) {
  const vector_t currentPose = observation.state.segment<6>(6);
  const vector_t targetPose = [&]() {
    vector_t target(6);
    target(0) = currentPose(0) + commadLineTarget(0);
    target(1) = currentPose(1) + commadLineTarget(1);
    target(2) = comHeight + commadLineTarget(2);
    target(3) = currentPose(3) + commadLineTarget(3) * M_PI / 180.0;
    target(4) = currentPose(4);
    target(5) = currentPose(5);
    return target;
  }();

  const scalar_t targetReachingTime = observation.time + estimateTimeToTarget(targetPose - currentPose);

  const scalar_array_t timeTrajectory{observation.time, targetReachingTime};

  vector_array_t stateTrajectory(2, vector_t::Zero(observation.state.size()));
  stateTrajectory[0] << vector_t::Zero(6), currentPose, defaultJointState;
  stateTrajectory[1] << vector_t::Zero(6), targetPose, defaultJointState;

  const vector_array_t inputTrajectory(2, vector_t::Zero(observation.input.size()));

  return {timeTrajectory, stateTrajectory, inputTrajectory};
}

int main(int argc, char* argv[]) {
  const std::string robotName = "legged_robot";

  std::vector<std::string> programArgs = rclcpp::remove_ros_arguments(argc, argv);
  if (programArgs.size() <= 1) {
    throw std::runtime_error("No task file specified. Aborting.");
  }
  std::string taskFileFolderName = std::string(programArgs[1]);
  std::string referenceFileFolderName = std::string(programArgs[2]);

  rclcpp::init(argc, argv);
  rclcpp::Node::SharedPtr nodeHandle = rclcpp::Node::make_shared(robotName + "_command");

  const std::string referenceFile =
      ament_index_cpp::get_package_share_directory("ocs2_legged_robot") + "/config/" + referenceFileFolderName + "/reference.info";

  loadData::loadCppDataType(referenceFile, "comHeight", comHeight);
  loadData::loadEigenMatrix(referenceFile, "defaultJointState", defaultJointState);
  loadData::loadCppDataType(referenceFile, "targetRotationVelocity", targetRotationVelocity);
  loadData::loadCppDataType(referenceFile, "targetDisplacementVelocity", targetDisplacementVelocity);

  const scalar_array_t relativeBaseLimit{10.0, 10.0, 0.2, 360.0};  // [deltaX, deltaY, deltaZ, deltaYaw]
  TargetTrajectoriesWebPublisher targetPoseCommand(nodeHandle, robotName, relativeBaseLimit,
                                                   {"deltaX", "deltaY", "deltaZ", "deltaYaw (deg)"}, &commandLineToTargetTrajectories);

  const std::string gaitCommandFile =
      ament_index_cpp::get_package_share_directory("ocs2_legged_robot") + "/config/" + referenceFileFolderName + "/gait.info";
  GaitWebPublisher gaitCommand(nodeHandle, gaitCommandFile, robotName, true);

  constexpr char kPosePath[] = "/pose-command";
  constexpr char kGaitPath[] = "/gait-command";

  std::mutex statusMutex;
  std::string poseStatus;
  std::string gaitStatus;

  const auto renderPage = [&]() {
    std::lock_guard<std::mutex> lock(statusMutex);
    std::ostringstream html;
    html << "<!doctype html><html><head><meta charset=\"utf-8\"><title>" << htmlEscape(nodeHandle->get_name())
         << "</title>\n"
            "<style>body{font-family:sans-serif;max-width:32rem;margin:2rem auto;padding:0 1rem}"
            "label{display:block;margin-top:0.75rem}input,select{width:100%;font-size:1rem;padding:0.25rem;"
            "margin-top:0.75rem;box-sizing:border-box}button{margin-top:1rem;font-size:1rem;padding:0.5rem 1.5rem}"
            ".status{margin-top:1rem;color:#155724;background:#d4edda;padding:0.5rem;border-radius:4px}"
            "hr{margin:2rem 0}</style></head><body>\n"
         << targetPoseCommand.renderFormHtml("Enter XYZ and Yaw (deg) displacements for the TORSO", kPosePath, poseStatus) << "<hr>\n"
         << gaitCommand.renderFormHtml(kGaitPath, gaitStatus) << "</body></html>\n";
    return html.str();
  };

  httplib::Server server;

  server.Get("/", [&](const httplib::Request&, httplib::Response& res) { res.set_content(renderPage(), "text/html"); });

  server.Post(kPosePath, [&](const httplib::Request& req, httplib::Response& res) {
    const std::string status = targetPoseCommand.handleCommand(req);
    {
      std::lock_guard<std::mutex> lock(statusMutex);
      poseStatus = status;
    }
    res.set_content(renderPage(), "text/html");
  });

  server.Post(kGaitPath, [&](const httplib::Request& req, httplib::Response& res) {
    const std::string status = gaitCommand.handleCommand(req);
    {
      std::lock_guard<std::mutex> lock(statusMutex);
      gaitStatus = status;
    }
    res.set_content(renderPage(), "text/html");
  });

  // rclcpp::spin() blocks until shutdown then returns, doubling as this server's stop signal.
  std::thread spinThread([&]() {
    rclcpp::spin(nodeHandle);
    server.stop();
  });

  constexpr int port = 8081;
  RCLCPP_INFO(nodeHandle->get_logger(), "Command form ready at http://localhost:%d/", port);
  server.listen("0.0.0.0", port);
  spinThread.join();

  return 0;
}
