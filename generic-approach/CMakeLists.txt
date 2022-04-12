cmake_minimum_required(VERSION 3.18)

project(opendds-mqtt-examples-generic-approach CXX)

find_package(PahoMqttCpp REQUIRED)
find_package(OpenDDS REQUIRED)

add_executable(generic-relay "generic-relay.cpp")
target_link_libraries(generic-relay
  opendds-mqtt-examples-common
  PahoMqttCpp::paho-mqttpp3-static
  OpenDDS::Dcps
  OpenDDS::Rtps_Udp
)

add_executable(tasmota-toggle "tasmota-toggle.cpp")
target_link_libraries(tasmota-toggle
  opendds-mqtt-examples-common
  OpenDDS::Dcps
  OpenDDS::Rtps_Udp
)
