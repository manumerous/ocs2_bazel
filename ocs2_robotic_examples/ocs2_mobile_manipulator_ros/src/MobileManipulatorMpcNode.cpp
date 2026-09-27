/******************************************************************************
Copyright (c) 2020, Farbod Farshidian. All rights reserved.

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

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <rclcpp/rclcpp.hpp>

#include <ocs2_ddp/GaussNewtonDDP_MPC.h>
#include <ocs2_ros2_interfaces/mpc/MPC_ROS_Interface.h>
#include <ocs2_ros2_interfaces/synchronized_module/RosReferenceManager.h>

#include <ocs2_mobile_manipulator/MobileManipulatorInterface.h>

using namespace ocs2;
using namespace mobile_manipulator;

int main(int argc, char** argv) {
  const std::string robotName = "mobile_manipulator";

  // task file (relative to ocs2_mobile_manipulator/config/) and urdf file
  // (relative to ocs2_robotic_assets/resources/mobile_manipulator/)
  std::vector<std::string> programArgs = rclcpp::remove_ros_arguments(argc, argv);
  if (programArgs.size() < 3) {
    throw std::runtime_error("Usage: mobile_manipulator_mpc <task file, e.g. franka/task.info> <urdf file, e.g. franka/urdf/panda.urdf>");
  }
  const std::string taskFileName = programArgs[1];
  const std::string urdfFileName = programArgs[2];

  // Initialize ros node
  rclcpp::init(argc, argv);
  rclcpp::Node::SharedPtr nodeHandle = rclcpp::Node::make_shared(robotName + "_mpc");

  // Get node parameters
  const std::string taskFile = ament_index_cpp::get_package_share_directory("ocs2_mobile_manipulator") + "/config/" + taskFileName;
  const std::string urdfFile =
      ament_index_cpp::get_package_share_directory("ocs2_robotic_assets") + "/resources/mobile_manipulator/" + urdfFileName;
  // CppAD codegen output, keyed by task file so e.g. kinova/task_j2n6 and kinova/task_j2n7 don't collide
  const std::string libFolder = "/tmp/ocs2/mobile_manipulator/" + taskFileName.substr(0, taskFileName.rfind(".info"));
  std::cerr << "Loading task file: " << taskFile << std::endl;
  std::cerr << "Loading library folder: " << libFolder << std::endl;
  std::cerr << "Loading urdf file: " << urdfFile << std::endl;
  // Robot interface
  MobileManipulatorInterface interface(taskFile, libFolder, urdfFile);

  // ROS ReferenceManager
  auto rosReferenceManagerPtr = std::make_shared<ocs2::RosReferenceManager>(robotName, interface.getReferenceManagerPtr());
  rosReferenceManagerPtr->subscribe(nodeHandle, rclcpp::QoS(1));

  // MPC
  ocs2::GaussNewtonDDP_MPC mpc(interface.mpcSettings(), interface.ddpSettings(), interface.getRollout(),
                               interface.getOptimalControlProblem(), interface.getInitializer());
  mpc.getSolverPtr()->setReferenceManager(rosReferenceManagerPtr);

  // Launch MPC ROS node
  MPC_ROS_Interface mpcNode(mpc, robotName);
  mpcNode.launchNodes(nodeHandle, rclcpp::QoS(1));

  // Successful exit
  return 0;
}
