# Examples of Bridging OpenDDS and MQTT to Control Tasmota Smart Devices

These are examples of bridging data between DDS and MQTT using
[OpenDDS](https://opendds.org) and [Eclipse Paho
C++](https://github.com/eclipse/paho.mqtt.cpp) to control [Tasmota smart
devices](https://tasmota.github.io/docs/). This is discussed in this [Object
Computing Inc Middleware News Brief Article](https://objectcomputing.com/resources/publications/mnb/2022/06/01/bridging-opendds-and-mqtt-messaging).

## Requirements

Building the examples requires OpenDDS and CMake. Running the examples requires
a MQTT broker to be setup and at least one Tasmota device connected to the
broker that supports power on/off and power usage reporting functionality.

## Building

Assuming OpenDDS is available in the environment, then building is the same as
any normal CMake project:

```
cmake -S . -B build
cmake --build build
```

Paho C and C++ libraries are included as git submodules and will be built and
used automatically. To skip this and use another set of Paho libraries, pass
`-DBUILD_PAHO=FALSE` to the CMake configure command.

## Running

Both examples consist of a MQTT relay that has to be connected to a MQTT broker
and a controller OpenDDS-only application that communicates with the MQTT relay
using DDS.

### `generic-relay` Example

`generic-relay` and `tasmota-toggle` use a direct mapping between MQTT and DDS
messages. This consists of two DDS topics to send and receive from MQTT. The
topics share the same topic type that consist of the MQTT topic name and the
message contents as a string. See
[`common/MqttMessage.idl`](common/MqttMessage.idl) for the `MqttMessage` type.

[`generic-relay`](generic-relay/generic-relay.cpp) receives MQTT messages and
writes them to the DDS topic and vice versa. It has no knowledge of how Tasmota
works, the contents are only interpreted by `tasmota-toggle`.
[`tasmota-toggle`](generic-relay/tasmota-toggle.cpp) gets the on/off status of
each Tasmota device it discovers, then sends a message to toggle it. This could
be done without getting the on/off status, as Tasmota has a way to toggle the
power directly, but this is done for showing an example of a back and forth
data exchange.

```
./build/generic-relay/generic-relay 127.0.0.1 &
./build/generic-relay/tasmota-toggle
```

### `idl-relay` Example

`idl-relay` and `tasmota-power` use a more complex mapping between MQTT
messages and DDS messages. It also uses two topics, but the type are specific
to how the topics will be used. See [`common/tasmota.idl`](common/tasmota.idl)
for the `Power` and `Wattage` types.

[`idl-relay`](idl-relay/idl-relay.cpp) is similar to the `tasmota-toggle`
application in that it is trying to discover Tasmota devices, but it's doing
this using MQTT directly and then getting and writing the wattage usage the
devices to a DDS topic. It will send power on/off commands to the device if
another DDS topic call for that. [`tasmota-power`](idl-relay/tasmota-power.cpp)
takes a wattage limit and reads the DDS topic for wattage usage and for every
device that pushes the total wattage over the argument, it shuts off that
device using the other DDS topic.

```
./build/idl-relay/idl-relay 127.0.0.1 &
./build/idl-relay/tasmota-power 100
```
