################################################################################
# created by Ingo Heppner
# Ingo.Heppner@gcsc.uni-frankfurt.de
#
# Toolchain file for JuQueen (type IBM Blue Gene/Q) FZ Juelich.
#
# This toolchain file is included in a CMake run by (check with 'cmake --trace')
# 'Modules/CMakeDetermineSystem.cmake', and (later again) by the - generated -
# file '<ug4_build_dir>/CMakeFiles/CMakeSystem.cmake'.
# (Subdirectory 'Modules/' is part of your CMake installation, on JuQueens login
# nodes: '/usr/local/cmake/share/cmake-2.8/Modules/').
#
# Appropriate platform files for the "CMake system name" and compilers chosen
# here are called later (by 'Modules/CMakeSystemSpecificInformation.cmake'):
#
# I.e., for "BlueGeneQ-static" as "CMake system name" the platform file
# 'Modules/Platform/BlueGeneQ-static.cmake' is included first, which includes
# 'Modules/Platform/BlueGeneQ-base.cmake'. Afterwards CMake calls
# 'Modules/Platform/BlueGeneQ-static-GNU-C.cmake' and
# 'Modules/Platform/BlueGeneQ-static-GNU-CXX.cmake'
# if GNU compilers are chosen (by 'Modules/CMakeCInformation.cmake' and
# 'Modules/CMakeCXXInformation.cmake' respectively).
#
################################################################################
#
# Attention:
# For this to work a CMake version with platform files for Blue Gene/Q are required.
# The currently (October 2012) installed version on JuQueen does not (the FZJ
# support is asked to support one).
#
# In the meantime the Juqueen platform-files supplied with ug are used (located
# in cmake/modules/Platform). Those stem from
# 'http://www.cmake.org/Bug/bug_relationship_graph.php?bug_id=13512&graph=dependency'
################################################################################

# Important: Setting the "cmake system name" will lead to automatic inclusion of
# the corresponding platform files:
set(CMAKE_SYSTEM_NAME BlueGeneQ-static)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# This option tells cmake/ug_includes.cmake to add the -dynamic option to the compiler flags.
SET(enableDynamicOption OFF)

# This option tells cmake/ug_includes.cmake to add the -fno-strict-aliasing option to the compiler flags.
SET(enableNoStrictAliasingOption ON)

# The serial GNU compilers
set(CMAKE_C_COMPILER       bgxlc)
set(CMAKE_CXX_COMPILER     bgxlc++)
set(CMAKE_Fortran_COMPILER bgxlf90)

# The MPI wrappers for the GNU compilers
set(MPI_C_COMPILER       mpixlc)
set(MPI_CXX_COMPILER     mpixlcxx)
set(MPI_Fortran_COMPILER mpixlf90)

message(STATUS "TMP INFO: Value of '\${CMAKE_C_COMPILER}'       is: ${CMAKE_C_COMPILER}")       # TMP
message(STATUS "TMP INFO: Value of '\${CMAKE_CXX_COMPILER}'     is: ${CMAKE_CXX_COMPILER}")     # TMP
message(STATUS "TMP INFO: Value of '\${CMAKE_Fortran_COMPILER}' is: ${CMAKE_Fortran_COMPILER}") # TMP

message(STATUS "TMP INFO: Value of '\${MPI_C_COMPILER}'         is: ${MPI_C_COMPILER}")         # TMP
message(STATUS "TMP INFO: Value of '\${MPI_CXX_COMPILER}'       is: ${MPI_CXX_COMPILER}")       # TMP
message(STATUS "TMP INFO: Value of '\${MPI_Fortran_COMPILER}'   is: ${MPI_Fortran_COMPILER}")   # TMP

# For debugging purposes
#include(CMakePrintSystemInformation)
