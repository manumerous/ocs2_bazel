# OCS2 Bazel Toolbox
An updated version of the Optimal Control for Switched Systems (OCS2) library ported to build with bazel and up to date dependencies including ROS2 and Pinocchio 4.

 Furthermore it includes improved interactivity (Gain setting, enabling/disabling costs & constraints) from my prior ROS2 port of OCS2.

## Summary
OCS2 is a C++ toolbox tailored for Optimal Control for Switched Systems (OCS2). The toolbox provides an efficient implementation of the following algorith

* SLQ: Continuous-time domin DDP
* iLQR: Discrete-time domain DDP
* SQP: Multiple-shooting algorithm based on HPIPM
* PISOC: Path integral stochatic optimal control

![legged-robot](https://leggedrobotics.github.io/ocs2/_static/gif/legged_robot.gif)

OCS2 handles general path constraints through Augmented Lagrangian or relaxed barrier methods. To facilitate the application of OCS2 in robotic tasks, it provides the user with additional tools to set up the system dynamics (such as kinematic or dynamic models) and cost/constraints (such as self-collision avoidance and end-effector tracking) from a URDF model. The library also provides an automatic differentiation tool to calculate derivatives of the system dynamics, constraints, and cost. To facilitate its deployment on robotic platforms, the OCS2 provides tools for ROS interfaces. The toolbox’s efficient and numerically stable implementations in conjunction with its user-friendly interface have paved the way for employing it on numerous robotic applications with limited onboard computation power.

For more information refer to the project's [Documentation Page](https://leggedrobotics.github.io/ocs2/)

## Bazel build (active ROS2 tree)

The actively-developed ROS2 packages (`ocs2_core` through the robotic
examples that don't depend on Pinocchio -- see the migration notes for the
current exceptions) build with Bazel 9.3 via bzlmod, using the ROS Central
Registry for ROS2 "Lyrical Luth" packages and the Bazel Central Registry
for generic C++ dependencies:

```
bazel build //...
bazel test //...
```

See [ocs2_robotic_examples/README.md](ocs2_robotic_examples/README.md) for
how to run the robot examples (double integrator, ballbot, legged robot).

