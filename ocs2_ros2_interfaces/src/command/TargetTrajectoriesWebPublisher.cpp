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

#include "ocs2_ros2_interfaces/command/TargetTrajectoriesWebPublisher.h"

#include <sstream>
#include <thread>

#include <httplib.h>

#include <ocs2_core/misc/Display.h>
#include <ocs2_ros2_interfaces/command/HtmlFormUtils.h>
#include <ocs2_ros2_interfaces/common/RosMsgConversions.h>
#include <ocs2_ros2_msgs/msg/mpc_observation.hpp>

namespace ocs2 {

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
TargetTrajectoriesWebPublisher::TargetTrajectoriesWebPublisher(rclcpp::Node::SharedPtr node, const std::string& topicPrefix,
                                                                const scalar_array_t& targetCommandLimits,
                                                                std::vector<std::string> fieldNames,
                                                                CommandLineToTargetTrajectories commandLineToTargetTrajectoriesFun)
    : targetCommandLimits_(Eigen::Map<const vector_t>(targetCommandLimits.data(), targetCommandLimits.size())),
      fieldNames_(std::move(fieldNames)),
      commandLineToTargetTrajectoriesFun_(std::move(commandLineToTargetTrajectoriesFun)),
      node_(node) {
  auto observationCallback = [this](const ocs2_ros2_msgs::msg::MpcObservation::SharedPtr msg) {
    std::lock_guard<std::mutex> lock(latestObservationMutex_);
    latestObservation_ = ros_msg_conversions::readObservationMsg(*msg);
  };
  observationSubscriber_ = node->create_subscription<ocs2_ros2_msgs::msg::MpcObservation>(
      topicPrefix + "/mpc_observation", rclcpp::QoS(1).best_effort(), observationCallback);

  targetTrajectoriesPublisherPtr_.reset(new TargetTrajectoriesRosPublisher(node, rclcpp::QoS(1).reliable(), topicPrefix));
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void TargetTrajectoriesWebPublisher::run(int port, const std::string& promptText) {
  const auto renderPage = [this, &promptText](const std::string& status) {
    std::ostringstream html;
    html << "<!doctype html><html><head><meta charset=\"utf-8\"><title>" << htmlEscape(node_->get_name())
         << "</title>\n"
            "<style>body{font-family:sans-serif;max-width:32rem;margin:2rem auto;padding:0 1rem}"
            "label{display:block;margin-top:0.75rem}input{width:100%;font-size:1rem;padding:0.25rem;"
            "box-sizing:border-box}button{margin-top:1rem;font-size:1rem;padding:0.5rem 1.5rem}"
            ".status{margin-top:1rem;color:#155724;background:#d4edda;padding:0.5rem;border-radius:4px}"
            "</style></head><body>\n<h1>"
         << htmlEscape(promptText) << "</h1>\n<form method=\"POST\" action=\"/command\">\n";
    for (size_t i = 0; i < fieldNames_.size(); ++i) {
      html << "<label>" << htmlEscape(fieldNames_[i]) << " (limit \xc2\xb1" << targetCommandLimits_(i) << ")"
           << "<input type=\"number\" step=\"any\" name=\"f" << i << "\" value=\"0\" min=\"" << -targetCommandLimits_(i) << "\" max=\""
           << targetCommandLimits_(i) << "\"></label>\n";
    }
    html << "<button type=\"submit\">Send</button>\n</form>\n";
    if (!status.empty()) {
      html << "<div class=\"status\">" << htmlEscape(status) << "</div>\n";
    }
    html << "</body></html>\n";
    return html.str();
  };

  httplib::Server server;

  server.Get("/", [&](const httplib::Request&, httplib::Response& res) { res.set_content(renderPage(""), "text/html"); });

  server.Post("/command", [&](const httplib::Request& req, httplib::Response& res) {
    vector_t commandLineInput = vector_t::Zero(targetCommandLimits_.size());
    for (Eigen::Index i = 0; i < targetCommandLimits_.size(); ++i) {
      const std::string fieldKey = "f" + std::to_string(i);
      if (req.has_param(fieldKey)) {
        try {
          commandLineInput(i) = std::stod(req.get_param_value(fieldKey));
        } catch (const std::exception&) {
        }
      }
    }
    commandLineInput = commandLineInput.cwiseMin(targetCommandLimits_).cwiseMax(-targetCommandLimits_);

    SystemObservation observation;
    {
      std::lock_guard<std::mutex> lock(latestObservationMutex_);
      observation = latestObservation_;
    }

    const auto targetTrajectories = commandLineToTargetTrajectoriesFun_(commandLineInput, observation);
    targetTrajectoriesPublisherPtr_->publishTargetTrajectories(targetTrajectories);

    const std::string statusMsg = "Sent: [" + toDelimitedString(commandLineInput) + "]";
    RCLCPP_INFO(node_->get_logger(), "%s", statusMsg.c_str());
    res.set_content(renderPage(statusMsg), "text/html");
  });

  // rclcpp::spin() blocks until shutdown then returns, doubling as this server's stop signal.
  std::thread spinThread([this, &server]() {
    rclcpp::spin(node_);
    server.stop();
  });

  RCLCPP_INFO(node_->get_logger(), "Command form ready at http://localhost:%d/", port);
  server.listen("0.0.0.0", port);
  spinThread.join();
}

}  // namespace ocs2
