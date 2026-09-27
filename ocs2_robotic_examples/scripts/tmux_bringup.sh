#!/usr/bin/env bash
#
# Single-command bringup for one ocs2_robotic_examples demo: the MPC solver,
# the dummy simulator, rviz2, and the interactive commander node(s) -- each in
# its own tmux pane of one session.
#
# This exists because of two things Bazel's `bringup_*` rules_multirun
# targets (see the ocs2_*_ros BUILD.bazel files) genuinely cannot do:
#   - rviz2 isn't a Bazel target in this workspace (no rviz2/rviz_common/
#     rviz_rendering module in the ROS Central Registry) -- it has to be
#     run from a real, sourced system ROS2 install.
#   - The interactive commander nodes (ballbot_target, legged_robot_target,
#     legged_robot_gait_command) read line-by-line from std::cin.
#     rules_multirun's forward_stdin only replays whole-stdin-until-EOF as a
#     single batch to every process in the group afterwards, not live
#     keystrokes to one process -- unusable for a REPL-style commander.
# tmux panes are real ptys, so the commander gets working interactive input,
# and killing the session cleans up every pane at once.
#
# Requires: tmux, bazel, and (for rviz2) a sourced system ROS2 install whose
# `rviz2` is on PATH (e.g. `source /opt/ros/jazzy/setup.bash` beforehand).
#
# Usage:
#   ocs2_robotic_examples/scripts/tmux_bringup.sh <robot> <solver> [options]
#
#   <robot>   ballbot | legged_robot
#   <solver>  ballbot: ddp | slp | sqp        legged_robot: ddp | sqp
#
# Options:
#   --no-rviz   don't start rviz2
#   --kill      stop the matching session (mpc/dummy/rviz2/commanders) and exit
#
# Examples:
#   ocs2_robotic_examples/scripts/tmux_bringup.sh ballbot sqp
#   ocs2_robotic_examples/scripts/tmux_bringup.sh legged_robot ddp --no-rviz
#   ocs2_robotic_examples/scripts/tmux_bringup.sh ballbot sqp --kill

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
workspace_dir="$(cd "$script_dir/../.." && pwd)"
cd "$workspace_dir"

usage() {
  cat >&2 <<'EOF'
Usage: tmux_bringup.sh <robot> <solver> [--no-rviz] [--kill]

  <robot>   ballbot | legged_robot
  <solver>  ballbot: ddp | slp | sqp      legged_robot: ddp | sqp

Options:
  --no-rviz   don't start rviz2
  --kill      stop the matching tmux session and exit
EOF
  exit 1
}

[[ $# -ge 2 ]] || usage
robot="$1"
solver="$2"
shift 2

with_rviz=1
kill_only=0
for arg in "$@"; do
  case "$arg" in
    --no-rviz) with_rviz=0 ;;
    --kill) kill_only=1 ;;
    *) usage ;;
  esac
done

# Per-robot config: package dir, rviz config, node executables, and the
# shared task_name/reference_name argv every node in this demo takes (this
# matches the "all targets default to the same args" convention already used
# by the bringup_* / run_*_target rules_multirun targets in each package's
# BUILD.bazel).
case "$robot" in
  ballbot)
    pkg="ocs2_robotic_examples/ocs2_ballbot_ros"
    rviz_config="$pkg/rviz/ballbot.rviz"
    args=(mpc)
    case "$solver" in
      ddp|slp|sqp) mpc_target="ballbot_$solver" ;;
      *) echo "error: unknown ballbot solver '$solver' (expected ddp, slp, sqp)" >&2; exit 1 ;;
    esac
    dummy_target="ballbot_dummy_test"
    commander_targets=(ballbot_target)
    ;;
  legged_robot)
    pkg="ocs2_robotic_examples/ocs2_legged_robot_ros"
    rviz_config="$pkg/rviz/legged_robot.rviz"
    args=(mpc command)
    case "$solver" in
      ddp|sqp) mpc_target="legged_robot_${solver}_mpc" ;;
      *) echo "error: unknown legged_robot solver '$solver' (expected ddp, sqp)" >&2; exit 1 ;;
    esac
    dummy_target="legged_robot_dummy"
    commander_targets=(legged_robot_target legged_robot_gait_command)
    ;;
  *)
    usage
    ;;
esac

session="ocs2_${robot}_${solver}"

command -v tmux >/dev/null 2>&1 || {
  echo "error: tmux not found. Install it (e.g. 'sudo apt install tmux')." >&2
  exit 1
}

if [[ "$kill_only" == 1 ]]; then
  if tmux has-session -t "$session" 2>/dev/null; then
    tmux kill-session -t "$session"
    echo "killed tmux session '$session'"
  else
    echo "no running session '$session'"
  fi
  exit 0
fi

if tmux has-session -t "$session" 2>/dev/null; then
  echo "tmux session '$session' is already running -- attaching."
  exec tmux attach -t "$session"
fi

if [[ "$with_rviz" == 1 ]] && ! command -v rviz2 >/dev/null 2>&1; then
  echo "warning: rviz2 not found on PATH." >&2
  echo "  Source a system ROS2 install first, e.g.:" >&2
  echo "    source /opt/ros/jazzy/setup.bash" >&2
  echo "  Continuing without rviz2 (pass --no-rviz to silence this)." >&2
  with_rviz=0
fi

build_targets=("//$pkg:$mpc_target" "//$pkg:$dummy_target")
for c in "${commander_targets[@]}"; do
  build_targets+=("//$pkg:$c")
done

echo "Building: ${build_targets[*]}"
bazel build "${build_targets[@]}"

bin_dir="bazel-bin/$pkg"
argv="${args[*]}"

# One tmux window, one pane per process, tiled. Panes are real ptys (unlike
# rules_multirun's forward_stdin), so the commander pane(s) get working
# line-by-line stdin. remain-on-exit keeps a pane open (showing its error)
# if its process dies instead of the pane just vanishing.
tmux new-session -d -s "$session" -n "$robot" "$bin_dir/$mpc_target $argv"
tmux set-window-option -t "$session" remain-on-exit on

tmux split-window -t "$session" "$bin_dir/$dummy_target $argv"

if [[ "$with_rviz" == 1 ]]; then
  tmux split-window -t "$session" "rviz2 -d '$workspace_dir/$rviz_config'"
fi

for c in "${commander_targets[@]}"; do
  tmux split-window -t "$session" "$bin_dir/$c $argv"
done

tmux select-layout -t "$session" tiled

echo ""
echo "Started tmux session '$session' (mpc: $mpc_target, dummy: $dummy_target$(
  [[ "$with_rviz" == 1 ]] && echo -n ", rviz2"
), commander(s): ${commander_targets[*]})."
echo "Ctrl-b d to detach (keeps running in the background)."
echo "Stop everything with: $0 $robot $solver --kill"
echo ""

exec tmux attach -t "$session"
