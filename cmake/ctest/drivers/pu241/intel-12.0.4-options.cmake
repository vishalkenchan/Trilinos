INCLUDE(${TRILINOS_HOME_DIR}/cmake/ctest/drivers/pu241/mkl-12.0.4-options.cmake)
INCLUDE(${TRILINOS_HOME_DIR}/cmake/ctest/drivers/pu241/boost-1.46.1-options.cmake)
INCLUDE(${TRILINOS_HOME_DIR}/cmake/ctest/drivers/pu241/casl-vri-tpls.cmake)

SET(INTEL_BIN /opt/intel/Compiler/composerxe-2011.4.191/bin/intel64)

SET(CMAKE_SKIP_RPATH ON BOOL "")
#SET(BUILD_SHARED_LIBS ON CACHE BOOL "")
SET(TPL_ENABLE_BinUtils ON CACHE BOOL "")
SET(CMAKE_C_COMPILER ${INTEL_BIN}/icc CACHE FILEPATH "")
SET(CMAKE_CXX_COMPILER ${INTEL_BIN}/icpc CACHE FILEPATH "")
SET(CMAKE_Fortran_COMPILER ${INTEL_BIN}/ifort CACHE FILEPATH "")
