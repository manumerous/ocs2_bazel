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

#include "ocs2_legged_robot_ros/gait/GaitWebPublisher.h"

#include <sstream>
#include <thread>

#include <httplib.h>

#include <ocs2_core/misc/LoadData.h>
#include <ocs2_ros2_interfaces/command/HtmlFormUtils.h>

#include "ocs2_legged_robot_ros/gait/ModeSequenceTemplateRos.h"

namespace ocs2 {
namespace legged_robot {

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
GaitWebPublisher::GaitWebPublisher(rclcpp::Node::SharedPtr nodeHandle, const std::string& gaitFile, const std::string& robotName,
                                   bool verbose)
    : node_(nodeHandle) {
  RCLCPP_INFO(nodeHandle->get_logger(), "%s_mpc_mode_schedule node is setting up ...", robotName.c_str());
  loadData::loadStdVector(gaitFile, "list", gaitList_, verbose);

  modeSequenceTemplatePublisher_ =
      nodeHandle->create_publisher<ocs2_ros2_msgs::msg::ModeSchedule>(robotName + "_mpc_mode_schedule", 1);

  gaitMap_.clear();
  for (const auto& gaitName : gaitList_) {
    gaitMap_.insert({gaitName, loadModeSequenceTemplate(gaitFile, gaitName, verbose)});
  }
  RCLCPP_INFO(nodeHandle->get_logger(), "%s_mpc_mode_schedule command node is ready.", robotName.c_str());
}

/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/
void GaitWebPublisher::run(int port) {
  const auto renderPage = [this](const std::string& status) {
    std::ostringstream html;
    html << "<!doctype html><html><head><meta charset=\"utf-8\"><title>" << htmlEscape(node_->get_name())
         << "</title>\n"
            "<style>body{font-family:sans-serif;max-width:32rem;margin:2rem auto;padding:0 1rem}"
            "select{width:100%;font-size:1rem;padding:0.25rem;margin-top:0.75rem;box-sizing:border-box}"
            "button{margin-top:1rem;font-size:1rem;padding:0.5rem 1.5rem}"
            ".status{margin-top:1rem;color:#155724;background:#d4edda;padding:0.5rem;border-radius:4px}"
            "</style></head><body>\n<h1>Select a gait</h1>\n<form method=\"POST\" action=\"/command\">\n<select name=\"gait\">\n";
    for (const auto& gaitName : gaitList_) {
      html << "<option value=\"" << htmlEscape(gaitName) << "\">" << htmlEscape(gaitName) << "</option>\n";
    }
    html << "</select>\n<button type=\"submit\">Send</button>\n</form>\n";
    if (!status.empty()) {
      html << "<div class=\"status\">" << htmlEscape(status) << "</div>\n";
    }
    html << "</body></html>\n";
    return html.str();
  };

  httplib::Server server;

  server.Get("/", [&](const httplib::Request&, httplib::Response& res) { res.set_content(renderPage(""), "text/html"); });

  server.Post("/command", [&](const httplib::Request& req, httplib::Response& res) {
    const std::string gaitCommand = req.get_param_value("gait");
    const auto it = gaitMap_.find(gaitCommand);
    if (it == gaitMap_.end()) {
      RCLCPP_WARN(node_->get_logger(), "Gait \"%s\" not found.", gaitCommand.c_str());
      res.set_content(renderPage("Gait \"" + gaitCommand + "\" not found."), "text/html");
      return;
    }
    modeSequenceTemplatePublisher_->publish(createModeSequenceTemplateMsg(it->second));
    RCLCPP_INFO(node_->get_logger(), "Published gait: %s", gaitCommand.c_str());
    res.set_content(renderPage("Sent gait: " + gaitCommand), "text/html");
  });

  std::thread spinThread([this, &server]() {
    rclcpp::spin(node_);
    server.stop();
  });

  RCLCPP_INFO(node_->get_logger(), "Gait command form ready at http://localhost:%d/", port);
  server.listen("0.0.0.0", port);
  spinThread.join();
}

}  // namespace legged_robot
}  // namespace ocs2
