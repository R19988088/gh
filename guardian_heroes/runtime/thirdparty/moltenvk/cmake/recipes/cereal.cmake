# cereal (https://github.com/USCiLab/cereal)
# License: BSD-3-Clause
if(TARGET cereal::cereal)
    return()
endif()

message(STATUS "External: creating target 'cereal::cereal'")

# Vendored at MoltenVK's pinned revision a56bad8bbb770ee266e930c95d37fff2a5be7fea.
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0126 NEW)
set(BUILD_DOC OFF)
set(BUILD_SANDBOX OFF)
set(SKIP_PERFORMANCE_COMPARISON ON)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../../cereal"
    "${CMAKE_CURRENT_BINARY_DIR}/vendor/cereal" EXCLUDE_FROM_ALL SYSTEM)
