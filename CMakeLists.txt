cmake_minimum_required(VERSION 3.18)

project(opendds-mqtt-examples CXX)

option(BUILD_PAHO "Build Paho in the local submodules" TRUE)

if(BUILD_PAHO)
  set(paho_dest_dir "${CMAKE_CURRENT_BINARY_DIR}/paho-mqtt")
  set(paho_install_prefix "${paho_dest_dir}/install")

  function(build_paho which)
    set(build "${paho_dest_dir}/${which}build")
    execute_process(
      COMMAND "${CMAKE_COMMAND}"
        "-DPAHO_BUILD_STATIC=ON"
        "-DPAHO_WITH_SSL=OFF"
        "-DPAHO_ENABLE_TESTING=OFF"
        "-DPAHO_BUILD_DOCUMENTATION=OFF"
        "-DPAHO_BUILD_SAMPLES=OFF"
        "-DCMAKE_PREFIX_PATH=${paho_install_prefix}"
        "-DCMAKE_INSTALL_PREFIX=${paho_install_prefix}"
        "-S" "${CMAKE_CURRENT_SOURCE_DIR}/paho.mqtt.${which}"
        "-B" "${build}"
      COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(COMMAND "${CMAKE_COMMAND}" "--build" "${build}" COMMAND_ERROR_IS_FATAL ANY)
    execute_process(COMMAND "${CMAKE_COMMAND}" "--install" "${build}" COMMAND_ERROR_IS_FATAL ANY)
  endfunction()

  build_paho("c")
  build_paho("cpp")

  list(APPEND CMAKE_PREFIX_PATH "${paho_install_prefix}")
endif()

add_subdirectory(common)
add_subdirectory(generic-relay)
add_subdirectory(idl-relay)
