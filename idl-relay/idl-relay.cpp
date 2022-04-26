#include <tasmota.h>
#include <opendds.h>
#include <ArgParser.h>
#include <NopDataReaderListener.h>

#include <mqtt/client.h>

#include <iostream>

class PowerDataReaderListener : public NopDataReaderListener {
public:
  mqtt::client& mqtt_client_;
  Configs& configs_;

  PowerDataReaderListener(mqtt::client& mqtt_client, Configs& configs)
    : mqtt_client_(mqtt_client)
    , configs_(configs)
  {
  }

  void on_data_available(DDS::DataReader_ptr reader)
  {
    tasmota::PowerDataReader_var reader_i = tasmota::PowerDataReader::_narrow(reader);
    if (!reader_i) {
      std::abort();
    }

    tasmota::PowerSeq messages;
    DDS::SampleInfoSeq infos;
    reader_i->take(messages, infos, DDS::LENGTH_UNLIMITED,
      DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    for (unsigned i = 0; i != messages.length(); ++i) {
      const std::string to = messages[i].on() ? "on" : "off";
      std::cout << "Turning " << messages[i].display_name() << " " << to << std::endl;
      auto it = configs_.find(messages[i].device_name());
      if (it != configs_.end()) {
        auto mqtt_message = mqtt::make_message(
          get_tasmota_topic(it->second, "cmnd", "Power"), to, 2, true);
        mqtt_client_.publish(mqtt_message);
      } else {
        std::cout << "Missing " << messages[i].device_name() << std::endl;
      }
    }
  }
};

int main(int argc, char* argv[])
{
  try {
    OpenddsWrapper opendds_wrapper(argc, argv);

    ArgParser arg_parser(argc, argv, "generic-relay", "[OPENDDS_OPTIONS] MQTT_BROKER");
    const std::string broker = arg_parser.get_next_pos_arg("MQTT_BROKER");
    arg_parser.done();

    // Setup MQTT
    mqtt::client mqtt_client(broker, "idl-relay");
    std::cout << "Connecting to the broker " << broker << "..." << std::flush;
    mqtt::connect_response rsp = mqtt_client.connect();
    std::cout << "OK\n" << std::endl;

    // Setup OpenDDS
    auto wattage_ts = opendds_wrapper.register_typesupport<tasmota::Wattage>();
    auto wattage_topic = wattage_ts.create_topic(tasmota::wattage_topic_name);
    auto writer = wattage_topic.create_datawriter();
    auto power_ts = opendds_wrapper.register_typesupport<tasmota::Power>();
    auto power_topic = power_ts.create_topic(tasmota::power_topic_name);
    Configs configs;
    DDS::DataReaderListener_var listener(new PowerDataReaderListener(mqtt_client, configs));
    auto reader = power_topic.create_datareader(listener);

    // Start off by discovering Tasmota devices
    Configs sensor_topic_to_configs;
    mqtt_client.subscribe("tasmota/discovery/+/config", 2);
    while (true) {
      auto msg = mqtt_client.consume_message();
      if (msg) {
        const std::string topic = msg->get_topic();
        const std::string message = msg->to_string();
        std::cout << "Received on " << topic << ": " << message << std::endl;

        if (is_tasmota_config(topic)) { // Discovered Device
          const tasmota::Config config = get_tasmota_config(message);
          configs[config.t()] = config;

          // Subscribe to sensor status topic
          const std::string sensor_topic = get_tasmota_topic(config, "stat", "STATUS10");
          std::cout << "Subscribing to " << sensor_topic << " for " << config.fn() << std::endl;
          mqtt_client.subscribe(sensor_topic, 2);
          sensor_topic_to_configs[sensor_topic] = config;

          // Write a status command to force a sensor status update
          const std::string status_topic = get_tasmota_topic(config, "cmnd", "Status");
          std::cout << "Getting an update on " << sensor_topic
            << " by writing to " << status_topic << std::endl;
          mqtt_client.publish(mqtt::make_message(status_topic, "10", 2, true));
        } else {
          // If sensor status, then relay to DDS
          auto it = sensor_topic_to_configs.find(topic);
          if (it != sensor_topic_to_configs.end()) {
            const auto& config = it->second;
            std::cout << "Updating DDS topic on " << config.fn() << std::endl;
            check_rc(writer->write(get_tasmota_wattage(
              config, message), DDS::HANDLE_NIL), "write");
          }
        }
      }
    }
  } catch (int value) {
    return value;
  } catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
