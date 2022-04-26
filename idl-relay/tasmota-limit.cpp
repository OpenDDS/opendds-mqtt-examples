#include <opendds.h>
#include <tasmota.h>
#include <ArgParser.h>

#include <dds/DCPS/WaitSet.h>

#include <iostream>
#include <string>
#include <cstdint>
#include <map>

int main(int argc, char* argv[])
{
  try {
    OpenddsWrapper opendds_wrapper(argc, argv);

    ArgParser arg_parser(argc, argv, "tasmota-limit", "[OPENDDS_OPTIONS] WATTAGE_LIMIT");
    const int32_t total_watts_limit = arg_parser.get_next_pos_arg_as_int("WATTAGE_LIMIT");
    arg_parser.done();

    // Setup OpenDDS
    auto wattage_ts = opendds_wrapper.register_typesupport<tasmota::Wattage>();
    auto wattage_topic = wattage_ts.create_topic(tasmota::wattage_topic_name);
    auto reader = wattage_topic.create_datareader();
    DDS::ReadCondition_var read_condition = reader->create_readcondition(
      DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    DDS::WaitSet_var wait_set = new DDS::WaitSet;
    wait_set->attach_condition(read_condition);
    auto power_ts = opendds_wrapper.register_typesupport<tasmota::Power>();
    auto power_topic = power_ts.create_topic(tasmota::power_topic_name);
    auto writer = power_topic.create_datawriter();

    std::map<std::string, tasmota::Wattage> devices;

    while (true) {
      DDS::ConditionSeq conditions;
      const DDS::Duration_t timeout = { 1, 0 };
      wait_set->wait(conditions, timeout);

      tasmota::WattageSeq messages;
      DDS::SampleInfoSeq infos;
      reader->take(messages, infos, DDS::LENGTH_UNLIMITED,
        DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
      bool changed = false;
      for (unsigned i = 0; i != messages.length(); ++i) {
        std::cout << messages[i].display_name() << ": "
          << messages[i].watts() << " watts" << std::endl;
        const std::string& key = messages[i].device_name();
        if (devices.count(key) == 0) {
          changed = true;
        } else if (devices[key].watts() != messages[i].watts()) {
          changed = true;
        }
        devices[key] = messages[i];
      }

      if (changed) {
        int32_t total_watts = 0;
        for (auto it : devices) {
          const std::string& device_name = it.first;
          auto& wattage = it.second;
          total_watts += wattage.watts();
          if (total_watts > total_watts_limit) {
            std::cout << wattage.display_name() << " exceeds limit by "
              << (total_watts - total_watts_limit) << " watts, powering off" << std::endl;
            total_watts -= wattage.watts();
            wattage.watts() = 0;
            check_rc(writer->write({wattage.device_name(), wattage.display_name(), false},
              DDS::HANDLE_NIL), "write");
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
