cmake_minimum_required(VERSION 3.18)

project(opendds-mqtt-examples-idl-relay CXX)

find_package(PahoMqttCpp REQUIRED)
find_package(OpenDDS REQUIRED)

add_executable(idl-relay "idl-relay.cpp")
target_link_libraries(idl-relay
  opendds-mqtt-examples-common
  PahoMqttCpp::paho-mqttpp3-static
  OpenDDS::Dcps
  OpenDDS::Rtps_Udp
)

add_executable(tasmota-limit "tasmota-limit.cpp")
target_link_libraries(tasmota-limit
  opendds-mqtt-examples-common
  OpenDDS::Dcps
  OpenDDS::Rtps_Udp
)
