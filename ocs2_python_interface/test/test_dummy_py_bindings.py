"""End-to-end smoke test for the nanobind-ported ocs2_python_interface API.

Imports the real nanobind extension module built from
testDummyPyBindingsModule.cc (via CREATE_ROBOT_PYTHON_BINDINGS) and drives
it through the same call sequence a real robot's Python bindings would use,
to verify the ported API actually works from Python, not just that it
compiles.
"""

import os
import sys

import numpy as np
from python.runfiles import runfiles

# The nanobind_extension target lives in the parent Bazel package
# (//ocs2_python_interface), so its .so doesn't land next to this test
# script in runfiles and isn't found by the default sys.path setup.
# Locate it explicitly via the runfiles API instead of relying on cwd.
_r = runfiles.Create()
_module_so = _r.Rlocation("_main/ocs2_python_interface/test_dummy_py_bindings_module.so")
sys.path.insert(0, os.path.dirname(_module_so))

import test_dummy_py_bindings_module as bindings


def test_mpc_interface_smoke():
    mpc = bindings.mpc_interface("", "", "")

    assert mpc.getStateDim() == 2
    assert mpc.getInputDim() == 1

    x0 = np.zeros(2)
    u0 = np.zeros(1)
    mpc.setObservation(0.0, x0, u0)

    t_arr = bindings.scalar_array()
    t_arr.push_back(0.0)
    x_arr = bindings.vector_array()
    x_arr.push_back(np.zeros(2))
    u_arr = bindings.vector_array()
    u_arr.push_back(np.zeros(1))
    target = bindings.TargetTrajectories(t_arr, x_arr, u_arr)
    mpc.setTargetTrajectories(target)
    mpc.reset(target)

    mpc.advanceMpc()

    t = bindings.scalar_array()
    x = bindings.vector_array()
    u = bindings.vector_array()
    mpc.getMpcSolution(t, x, u)
    assert len(t) > 0
    assert len(x) == len(t)

    # getLinearFeedbackGain is intentionally not exercised here: it requires
    # a linear-feedback DDP controller type, which is a solver-settings
    # concern unrelated to what this test verifies (the binding layer).

    dx = mpc.flowMap(0.0, x0, u0)
    assert dx.shape == (2,)

    lin = mpc.flowMapLinearApproximation(0.0, x0, u0)
    assert lin.dfdx.shape == (2, 2)
    assert lin.dfdu.shape == (2, 1)

    cost = mpc.cost(0.0, x0, u0)
    assert isinstance(cost, float)


def test_vector_array_container_protocol():
    v = bindings.vector_array()
    assert len(v) == 0
    v.push_back(np.array([1.0, 2.0]))
    v.push_back(np.array([3.0, 4.0]))
    assert len(v) == 2
    np.testing.assert_array_equal(v[0], [1.0, 2.0])
    v[0] = np.array([5.0, 6.0])
    np.testing.assert_array_equal(v[0], [5.0, 6.0])
    collected = [np.asarray(row).tolist() for row in v]
    assert collected == [[5.0, 6.0], [3.0, 4.0]]
    v.pop_back()
    assert len(v) == 1
    v.clear()
    assert len(v) == 0


if __name__ == "__main__":
    test_mpc_interface_smoke()
    test_vector_array_container_protocol()
    print("OK")
