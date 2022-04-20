#include <opendds.h>
#include <NopDataReaderListener.h>

#include <MqttMessageTypeSupportImpl.h>

#include <mqtt/client.h>

#include <iostream>

class ToMqttDataReaderListener : public NopDataReaderListener {
public:
  mqtt::client& mqtt_client_;

  ToMqttDataReaderListener(mqtt::client& mqtt_client)
    : mqtt_client_(mqtt_client)
  {
  }

  void on_data_available(DDS::DataReader_ptr reader)
  {
    MqttMessageDataReader_var reader_i = MqttMessageDataReader::_narrow(reader);
    if (!reader_i) {
      std::abort();
    }

    MqttMessageSeq messages;
    DDS::SampleInfoSeq infos;
    reader_i->take(messages, infos, DDS::LENGTH_UNLIMITED,
      DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    for (unsigned i = 0; i != messages.length(); ++i) {
      std::cout << "Sending " << messages[i].topic() << ": "
        << messages[i].message() << std::endl;
      auto mqtt_message = mqtt::make_message(messages[i].topic(), messages[i].message(), 2, true);
      mqtt_client_.publish(mqtt_message);
    }
  }
};

void usage(std::ostream& os)
{
  os << "generic-relay [OPENDDS_ARGUMENTS] BROKER" << std::endl;
}

int main(int argc, char* argv[])
{
  try {
    OpenddsWrapper opendds_wrapper(argc, argv);

    if (argc < 2) {
      std::cerr << "ERROR: At least one argument is required" << std::endl;
      usage(std::cerr);
      throw 1;
    }
    const std::string broker = argv[1];

    // Setup MQTT
    mqtt::client mqtt_client(broker, "opendds-mqtt-generic-relay");
    std::cout << "Connecting to the broker " << broker << "..." << std::flush;
    mqtt::connect_response rsp = mqtt_client.connect();
    std::cout << "OK\n" << std::endl;
    mqtt_client.subscribe("#", 2);

    // Setup OpenDDS
    auto mqtt_message_ts = opendds_wrapper.register_typesupport<MqttMessage>();
    auto to_mqtt_topic = mqtt_message_ts.create_topic(to_mqtt_topic_name);
    auto from_mqtt_topic = mqtt_message_ts.create_topic(from_mqtt_topic_name);
    auto writer = from_mqtt_topic.create_datawriter();
    DDS::DataReaderListener_var listener(new ToMqttDataReaderListener(mqtt_client));
    auto reader = to_mqtt_topic.create_datareader(listener);

    while (true) {
      auto msg = mqtt_client.consume_message();
      if (msg) {
        const MqttMessage dds_sample{msg->get_topic(), msg->to_string()};
        std::cout << "Received on " << dds_sample.topic() << ": "
          << dds_sample.message() << std::endl;
        check_rc(writer->write(dds_sample, DDS::HANDLE_NIL), "write");
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
