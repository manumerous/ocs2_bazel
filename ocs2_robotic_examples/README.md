# Running the robotic examples

Each robot example lives under its own `<name>` / `<name>_ros` package pair
(core dynamics/cost/constraints, and ROS2 nodes: MPC solver, dummy
simulator, interactive commanders) and follows the same pattern:

* **`bringup_*`** -- a `rules_multirun` target that starts the MPC solver
  and the dummy simulator together as a single `bazel run`, streaming both
  logs to one terminal; one Ctrl-C stops both.
* **`run_*_target`** / **`run_*_gait_command`** -- interactive commanders
  (pose, gait, ...) that block on `std::cin`
  (`TargetTrajectoriesKeyboardPublisher` / `GaitKeyboardPublisher`).
  `rules_multirun`'s parallel mode can't give a single command its own
  terminal or route keystrokes to just one process -- that's exactly what
  a separate `gnome-terminal --` window did in the old ROS1 launch files --
  so these are run on demand, each in its own terminal, alongside a
  `bringup_*` target.

All of this builds and runs with plain Bazel; no ROS install,
`AMENT_PREFIX_PATH`, or `source install/setup.bash` is needed. `rviz2`
visualization is not wired in (it isn't in the ROS Central Registry's
Bazel scope) -- run it separately from a system ROS2 install if you want
it.

## Double integrator (`ocs2_double_integrator[_ros]`)

```
# bringup: mpc solver + dummy simulator
bazel run //ocs2_robotic_examples/ocs2_double_integrator_ros:bringup_double_integrator

# interactive pose commander -- run alongside, in its own terminal
bazel run //ocs2_robotic_examples/ocs2_double_integrator_ros:run_double_integrator_target
```

DDP is its only solver variant.

## Ballbot (`ocs2_ballbot[_ros]`)

```
# bringup: mpc solver + dummy simulator (ddp or slp), or the combined mpc+mrt loop
bazel run //ocs2_robotic_examples/ocs2_ballbot_ros:bringup_ballbot_ddp
bazel run //ocs2_robotic_examples/ocs2_ballbot_ros:bringup_ballbot_slp
bazel run //ocs2_robotic_examples/ocs2_ballbot_ros:bringup_ballbot_mpc_mrt   # single combined process

# interactive pose commander -- run alongside, in its own terminal
bazel run //ocs2_robotic_examples/ocs2_ballbot_ros:run_ballbot_target
```

DDP, SLP, and the combined MPC+MRT loop are all buildable. `ballbot_sqp` is
not: it still includes ROS1-only headers (`<ros/init.h>`,
`<ocs2_ros_interfaces/...>`) -- pre-existing dead code, not something the
Bazel/ROS2 migration skipped.

## Legged robot / ANYmal (`ocs2_legged_robot[_ros]`)

```
# bringup: mpc solver + dummy simulator
bazel run //ocs2_robotic_examples/ocs2_legged_robot_ros:bringup_anymal

# interactive pose/gait commanders -- run alongside, each in its own terminal
bazel run //ocs2_robotic_examples/ocs2_legged_robot_ros:run_anymal_target
bazel run //ocs2_robotic_examples/ocs2_legged_robot_ros:run_anymal_gait_command
```

DDP is the only buildable solver variant: `legged_robot_sqp_mpc` and
`legged_robot_ipm_mpc` both reference the out-of-scope legacy
`ocs2_ros_interfaces` package (never ported to ROS2) -- pre-existing dead
code, not something the Bazel/ROS2 migration skipped.
