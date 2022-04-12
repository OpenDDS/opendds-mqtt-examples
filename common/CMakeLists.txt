cmake_minimum_required(VERSION 3.18)

project(opendds-mqtt-examples-common CXX)

find_package(OpenDDS REQUIRED)

add_library(${PROJECT_NAME} STATIC)
OPENDDS_TARGET_SOURCES(${PROJECT_NAME}
  "MqttMessage.idl"
  "tasmota.idl"
  OPENDDS_IDL_OPTIONS -Lc++11
)
target_link_libraries(${PROJECT_NAME} PUBLIC OpenDDS::Dcps)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(${PROJECT_NAME} PUBLIC OPENDDS_RAPIDJSON)
target_include_directories(${PROJECT_NAME} PUBLIC "${OPENDDS_RAPIDJSON}/include")
