# Running the robotic examples

Each robot example lives under its own `<name>` / `<name>_ros` package pair
(core dynamics/cost/constraints, and ROS2 nodes: MPC solver, dummy
simulator, interactive commanders) and follows the same pattern:

* **`bringup_*`** -- a `rules_multirun` target that starts the MPC solver,
  the dummy simulator, and the browser-form pose/gait commander(s) (see
  below) together as a single `bazel run`, streaming every log to one
  terminal; one Ctrl-C stops all of them.
* **`*_target_web`** / **`*_gait_command_web`** -- the pose/gait commanders
  bringup_* actually starts. Each serves a tiny HTML form (one number input
  per command field, or a gait dropdown; no JavaScript) on its own
  `localhost` port instead of reading from `std::cin`
  (`TargetTrajectoriesWebPublisher` / `GaitWebPublisher`, browser-based
  drop-in replacements for `TargetTrajectoriesKeyboardPublisher` /
  `GaitKeyboardPublisher`). Open the printed URL in a browser, fill in the
  form, hit Send.
* **`run_*_target`** / **`run_*_gait_command`** -- the original
  `std::cin`-reading commanders, kept for terminal-only use. `rules_multirun`'s
  parallel mode can't give a single command its own terminal or route
  keystrokes to just one process -- that's exactly what a separate
  `gnome-terminal --` window did in the old ROS1 launch files, and exactly
  what the `*_web` commanders above sidestep by not needing a terminal at
  all -- so if you want these instead, run them on demand, each in its own
  terminal, alongside a `bringup_*` target.

All of this builds and runs with plain Bazel; no ROS install,
`AMENT_PREFIX_PATH`, or `source install/setup.bash` is needed. `rviz2`
visualization is not wired in (it isn't in the ROS Central Registry's
Bazel scope) -- run it separately from a system ROS2 install if you want
it.

## Double integrator (`ocs2_double_integrator[_ros]`)

```
# bringup: mpc solver + dummy simulator + pose commander form (http://localhost:8080/)
bazel run //ocs2_robotic_examples/ocs2_double_integrator_ros:bringup_double_integrator

# std::cin pose commander instead -- run alongside, in its own terminal
bazel run //ocs2_robotic_examples/ocs2_double_integrator_ros:run_double_integrator_target
```

DDP is its only solver variant.

## Ballbot (`ocs2_ballbot[_ros]`)

```
# bringup: mpc solver + dummy simulator + pose commander form (http://localhost:8080/)
# (ddp, slp, or sqp), or the combined mpc+mrt loop
bazel run //ocs2_robotic_examples/ocs2_ballbot_ros:bringup_ballbot_ddp
bazel run //ocs2_robotic_examples/ocs2_ballbot_ros:bringup_ballbot_slp
bazel run //ocs2_robotic_examples/ocs2_ballbot_ros:bringup_ballbot_sqp
bazel run //ocs2_robotic_examples/ocs2_ballbot_ros:bringup_ballbot_mpc_mrt   # single combined process, no commander form

# std::cin pose commander instead -- run alongside, in its own terminal
bazel run //ocs2_robotic_examples/ocs2_ballbot_ros:run_ballbot_target
```

DDP, SLP, SQP, and the combined MPC+MRT loop are all buildable.

## Legged robot / ANYmal (`ocs2_legged_robot[_ros]`)

```
# bringup: mpc solver + dummy simulator + pose commander form (http://localhost:8081/)
# + gait commander form (http://localhost:8082/) (ddp or sqp)
bazel run //ocs2_robotic_examples/ocs2_legged_robot_ros:bringup_anymal
bazel run //ocs2_robotic_examples/ocs2_legged_robot_ros:bringup_anymal_sqp

# std::cin pose/gait commanders instead -- run alongside, each in its own terminal
bazel run //ocs2_robotic_examples/ocs2_legged_robot_ros:run_anymal_target
bazel run //ocs2_robotic_examples/ocs2_legged_robot_ros:run_anymal_gait_command
```

DDP and SQP are buildable. `legged_robot_ipm_mpc` is not: it still
references the out-of-scope legacy `ocs2_ros_interfaces` package (never
ported to ROS2) -- pre-existing dead code, not something the Bazel/ROS2
migration skipped.
