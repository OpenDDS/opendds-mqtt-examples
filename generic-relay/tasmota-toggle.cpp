#include <opendds.h>
#include <tasmota.h>
#include <ArgParser.h>
#include <MqttMessageTypeSupportImpl.h>

#include <dds/DCPS/WaitSet.h>

#include <iostream>

int main(int argc, char* argv[])
{
  try {
    OpenddsWrapper opendds_wrapper(argc, argv);

    ArgParser arg_parser(argc, argv, "tasmota-toggle", "[OPENDDS_OPTIONS]");
    arg_parser.done();

    // Setup OpenDDS
    auto mqtt_message_ts = opendds_wrapper.register_typesupport<MqttMessage>();
    auto to_mqtt_topic = mqtt_message_ts.create_topic(to_mqtt_topic_name);
    auto from_mqtt_topic = mqtt_message_ts.create_topic(from_mqtt_topic_name);
    auto writer = to_mqtt_topic.create_datawriter();
    auto reader = from_mqtt_topic.create_datareader();
    DDS::ReadCondition_var read_condition = reader->create_readcondition(
      DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    DDS::WaitSet_var wait_set = new DDS::WaitSet;
    wait_set->attach_condition(read_condition);

    Configs stat_power_topic_to_configs;
    std::set<std::string> changed;

    while (true) {
      DDS::ConditionSeq conditions;
      const DDS::Duration_t timeout = { 1, 0 };
      wait_set->wait(conditions, timeout);

      MqttMessageSeq messages;
      DDS::SampleInfoSeq infos;
      reader->take(messages, infos, DDS::LENGTH_UNLIMITED,
        DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
      for (unsigned i = 0; i != messages.length(); ++i) {
        const std::string& topic = messages[i].topic();
        const std::string& message = messages[i].message();
        std::cout << "Received on " << topic << ": " << message << std::endl;
        if (is_tasmota_config(topic)) {
          const tasmota::Config config = get_tasmota_config(message);
          stat_power_topic_to_configs[get_tasmota_topic(config, "stat", "POWER")] = config;
          check_rc(writer->write(
            {get_tasmota_topic(config, "cmnd", "Power"), ""}, DDS::HANDLE_NIL), "write");
        } else {
          auto it = stat_power_topic_to_configs.find(topic);
          if (it != stat_power_topic_to_configs.end()) {
            const auto& config = it->second;
            const std::string& device_name = config.t();
            if (!changed.count(device_name)) {
              tasmota::Power power = get_tasmota_power(config, message);
              const std::string to = toggle_tasmota_power(power);
              std::cout << "Toggling " << power.display_name() << " from "
                << power.on() << " to " << to << std::endl;
              check_rc(writer->write(
                {get_tasmota_topic(config, "cmnd", "Power"), to}, DDS::HANDLE_NIL), "write");
              changed.insert(device_name);
            }
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
