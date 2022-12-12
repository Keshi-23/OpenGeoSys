AddTest(
    NAME PhaseField_3D_beam_tens_AT1_iso
    PATH PhaseField/beam
    EXECUTABLE ogs
    EXECUTABLE_ARGS AT1_iso_tensile.prj
    WRAPPER mpirun
    WRAPPER_ARGS -np 1
    TESTER vtkdiff
    REQUIREMENTS OGS_USE_MPI
    RUNTIME 18
    DIFF_DATA
        expected_AT1_iso_tension_ts_10_t_1_000000_0.vtu AT1_iso_tension_ts_10_t_1.000000.vtu displacement displacement 1e-5 0
        expected_AT1_iso_tension_ts_10_t_1_000000_0.vtu AT1_iso_tension_ts_10_t_1.000000.vtu phasefield phasefield 1e-6 0
)

AddTest(
    NAME PhaseField_3D_beam_tens_AT2_iso
    PATH PhaseField/beam
    EXECUTABLE ogs
    EXECUTABLE_ARGS AT2_iso_tensile.prj
    WRAPPER mpirun
    WRAPPER_ARGS -np 1
    TESTER vtkdiff
    REQUIREMENTS OGS_USE_MPI
    RUNTIME 260
    DIFF_DATA
        expected_AT2_iso_tension_ts_10_t_1_000000_0.vtu AT2_iso_tension_ts_10_t_1_000000_0.vtu displacement displacement 1e-5 0
        expected_AT2_iso_tension_ts_10_t_1_000000_0.vtu AT2_iso_tension_ts_10_t_1_000000_0.vtu phasefield phasefield 1e-6 0
)

AddTest(
    NAME PhaseField_3D_beam_tens_AT1_vd
    PATH PhaseField/beam
    EXECUTABLE ogs
    EXECUTABLE_ARGS AT1_vd_tensile.prj
    WRAPPER mpirun
    WRAPPER_ARGS -np 1
    TESTER vtkdiff
    REQUIREMENTS OGS_USE_MPI
    RUNTIME 18
    DIFF_DATA
        expected_AT1_vd_tension_ts_10_t_1_000000_0.vtu AT1_vd_tension_ts_10_t_1.000000.vtu displacement displacement 1e-5 0
        expected_AT1_vd_tension_ts_10_t_1_000000_0.vtu AT1_vd_tension_ts_10_t_1.000000.vtu phasefield phasefield 1e-6 0
)

AddTest(
    NAME PhaseField_3D_beam_tens_AT1_vd_2core
    PATH PhaseField/beam
    EXECUTABLE ogs
    EXECUTABLE_ARGS AT1_vd_tensile_2core.prj
    WRAPPER mpirun
    WRAPPER_ARGS -np 2
    TESTER vtkdiff
    REQUIREMENTS OGS_USE_MPI
    RUNTIME 140
    DIFF_DATA
        expected_AT1_vd_tension_2core_ts_10_t_1_000000_0.vtu AT1_vd_tension_2core_ts_10_t_1_000000_0.vtu displacement displacement 1e-5 0
        expected_AT1_vd_tension_2core_ts_10_t_1_000000_1.vtu AT1_vd_tension_2core_ts_10_t_1_000000_1.vtu displacement displacement 1e-5 0
        expected_AT1_vd_tension_2core_ts_10_t_1_000000_0.vtu AT1_vd_tension_2core_ts_10_t_1_000000_0.vtu phasefield phasefield 1e-6 0
        expected_AT1_vd_tension_2core_ts_10_t_1_000000_1.vtu AT1_vd_tension_2core_ts_10_t_1_000000_1.vtu phasefield phasefield 1e-6 0
)

AddTest(
    NAME PhaseField_3D_beam_tens_COHESIVE_linear_es
    PATH PhaseField/beam
    EXECUTABLE ogs
    EXECUTABLE_ARGS bar_COHESIVE_linear.prj
    WRAPPER mpirun
    WRAPPER_ARGS -np 1
    TESTER vtkdiff
    REQUIREMENTS OGS_USE_MPI
    RUNTIME 300
    DIFF_DATA
        expected_bar_COHESIVE_linear_ts_10_t_1_000000_0.vtu bar_COHESIVE_linear_ts_10_t_1_000000_0.vtu displacement displacement 1e-5 0
        expected_bar_COHESIVE_linear_ts_10_t_1_000000_0.vtu bar_COHESIVE_linear_ts_10_t_1_000000_0.vtu phasefield phasefield 1e-6 0
)

AddTest(
    NAME PhaseField_3D_beam_tens_COHESIVE_exponential_es
    PATH PhaseField/beam
    EXECUTABLE ogs
    EXECUTABLE_ARGS bar_COHESIVE_exponential.prj
    WRAPPER mpirun
    WRAPPER_ARGS -np 1
    TESTER vtkdiff
    REQUIREMENTS OGS_USE_MPI
    RUNTIME 300
    DIFF_DATA
        expected_bar_COHESIVE_exponential_ts_10_t_1_000000_0.vtu bar_COHESIVE_exponential_ts_10_t_1_000000_0.vtu displacement displacement 1e-5 0
        expected_bar_COHESIVE_exponential_ts_10_t_1_000000_0.vtu bar_COHESIVE_exponential_ts_10_t_1_000000_0.vtu phasefield phasefield 1e-6 0
)

AddTest(
    NAME PhaseField_2D_surfing_AT1_vd
    PATH PhaseField/surfing
    EXECUTABLE ogs
    EXECUTABLE_ARGS surfing.prj
    WRAPPER mpirun
    WRAPPER_ARGS -np 1
    TESTER vtkdiff
    REQUIREMENTS OGS_USE_MPI AND OGS_USE_PYTHON
    RUNTIME 18
    DIFF_DATA
        expected_surfing_ts_20_t_1_000000_0.vtu surfing_ts_20_t_1.000000.vtu displacement displacement 1e-5 0
        expected_surfing_ts_20_t_1_000000_0.vtu surfing_ts_20_t_1.000000.vtu phasefield phasefield 1e-6 0
)

AddTest(
    NAME PhaseField_2D_K_regime_HF
    PATH PhaseField/k_regime_HF
    EXECUTABLE ogs
    EXECUTABLE_ARGS 2D_bm_0p01.prj
    WRAPPER mpirun
    WRAPPER_ARGS -np 1
    TESTER vtkdiff
    REQUIREMENTS OGS_USE_MPI
    RUNTIME 18
    DIFF_DATA
        expected_2D_PropagatingCrack_AT1_h0p01_ts_2_t_0.020000.vtu 2D_PropagatingCrack_AT1_h0p01_ts_2_t_0.020000.vtu displacement displacement 1e-5 0
        expected_2D_PropagatingCrack_AT1_h0p01_ts_2_t_0.020000.vtu 2D_PropagatingCrack_AT1_h0p01_ts_2_t_0.020000.vtu phasefield phasefield 1e-6 0
)

AddTest(
    NAME PhaseField_3D_beam_tens_AT2_vd_ortho
    PATH PhaseField/beam/voldev-ortho
    EXECUTABLE ogs
    EXECUTABLE_ARGS AT2_vd_tensile_VZ.prj
    WRAPPER mpirun
    WRAPPER_ARGS -np 1
    TESTER vtkdiff
    REQUIREMENTS OGS_USE_MPI
    RUNTIME 120
    DIFF_DATA
    expected_AT2_vd_tensile_ts_10_t_1.000000.vtu AT2_vd_tensile_ts_10_t_1.000000.vtu displacement displacement 1e-5 0
    expected_AT2_vd_tensile_ts_10_t_1.000000.vtu AT2_vd_tensile_ts_10_t_1.000000.vtu phasefield phasefield 1e-6 0
)

if(OGS_USE_PETSC)
    NotebookTest(NOTEBOOKFILE PhaseField/surfing_jupyter_notebook/surfing_pyvista.ipynb RUNTIME 25)
    NotebookTest(NOTEBOOKFILE PhaseField/beam_jupyter_notebook/beam.ipynb RUNTIME 500 PROPERTIES PROCESSORS 3)
endif()
