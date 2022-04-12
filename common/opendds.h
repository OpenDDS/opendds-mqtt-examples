#ifndef OPENDDS_MQTT_EXAMPLES_COMMON_OPENDDS_H
#define OPENDDS_MQTT_EXAMPLES_COMMON_OPENDDS_H

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/TypeSupportImpl.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>

#include <dds/DdsDcpsC.h>

#include <stdexcept>

void check_rc(DDS::ReturnCode_t rc, const std::string& message)
{
  if (rc != DDS::RETCODE_OK) {
    throw std::runtime_error(message + ": " + OpenDDS::DCPS::retcode_to_string(rc));
  }
}

template<typename TopicType>
struct Traits {
  typedef typename OpenDDS::DCPS::DDSTraits<TopicType>::TypeSupportImplType TypeSupportImpl;
  typedef typename TypeSupportImpl::_var_type TypeSupportImplVar;
  typedef typename OpenDDS::DCPS::DDSTraits<TopicType>::DataWriterType DataWriter;
  typedef TAO_Objref_Var_T<DataWriter> DataWriterVar;
  typedef typename OpenDDS::DCPS::DDSTraits<TopicType>::DataReaderType DataReader;
  typedef TAO_Objref_Var_T<DataReader> DataReaderVar;
};

struct OpenddsWrapper {
  DDS::DomainParticipantFactory_var participant_factory_;
  DDS::DomainParticipant_var participant_;
  DDS::Publisher_var publisher_;
  DDS::Subscriber_var subscriber_;

  OpenddsWrapper(int& argc, char* argv[])
  {
    // Make RTPS the default discovery and transport
    TheServiceParticipant->set_default_discovery(OpenDDS::DCPS::Discovery::DEFAULT_RTPS);
    OpenDDS::DCPS::TransportConfig_rch transport_config =
      TheTransportRegistry->create_config("default_rtps_transport_config");
    OpenDDS::DCPS::TransportInst_rch transport_inst =
      TheTransportRegistry->create_inst("default_rtps_transport", "rtps_udp");
    transport_config->instances_.push_back(transport_inst);
    TheTransportRegistry->global_config(transport_config);

    participant_factory_ = TheParticipantFactoryWithArgs(argc, argv);
    if (!participant_factory_) throw std::runtime_error("Failed to initialize OpenDDS!");

    participant_ = participant_factory_->create_participant(
      101, PARTICIPANT_QOS_DEFAULT, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!participant_) throw std::runtime_error("Failed to create DDS Participant!");

    // Create publisher whose default writer qos is durable
    publisher_ = participant_->create_publisher(
      PUBLISHER_QOS_DEFAULT, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!publisher_) throw std::runtime_error("Failed to create DDS Publisher!");
    DDS::DataWriterQos dw_qos;
    check_rc(publisher_->get_default_datawriter_qos(dw_qos), "get_default_datawriter_qos failed");
    dw_qos.durability.kind = DDS::TRANSIENT_DURABILITY_QOS;
    dw_qos.durability_service.history_kind = DDS::KEEP_LAST_HISTORY_QOS;
    dw_qos.durability_service.history_depth = 1;
    dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    dw_qos.resource_limits.max_samples_per_instance = 1;
    dw_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
    dw_qos.history.depth = 1;
    check_rc(publisher_->set_default_datawriter_qos(dw_qos), "set_default_datawriter_qos failed");

    // Create subscriber whose default reader qos is reliable
    subscriber_ = participant_->create_subscriber(
      SUBSCRIBER_QOS_DEFAULT, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    DDS::DataReaderQos dr_qos;
    check_rc(subscriber_->get_default_datareader_qos(dr_qos), "get_default_datareader_qos failed");
    dr_qos.durability.kind = DDS::TRANSIENT_DURABILITY_QOS;
    dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    check_rc(subscriber_->set_default_datareader_qos(dr_qos), "set_default_datareader_qos failed");
    if (!subscriber_) throw std::runtime_error("Failed to create DDS Subscriber!");
  }

  ~OpenddsWrapper()
  {
    participant_->delete_contained_entities();
    participant_factory_->delete_participant(participant_);
    TheServiceParticipant->shutdown();
  }

  template<typename TopicType>
  typename Traits<TopicType>::TypeSupportImplVar register_typesupport()
  {
    typename Traits<TopicType>::TypeSupportImplVar ts_var =
      new typename Traits<TopicType>::TypeSupportImpl;
    check_rc(ts_var->register_type(participant_, ""), "registering typesupport failed");
    return ts_var;
  }

  DDS::Topic_var create_topic(const char* name, const char* type_name)
  {
    DDS::Topic_var topic = participant_->create_topic(
      name, type_name, TOPIC_QOS_DEFAULT, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!topic) throw std::runtime_error(std::string("Failed to create DDS topic ") + name + "!");
    return topic;
  }

  template<typename TopicType>
  typename Traits<TopicType>::DataWriterVar create_datawriter(DDS::Topic_var topic)
  {
    typename Traits<TopicType>::DataWriterVar datawriter = Traits<TopicType>::DataWriter::_narrow(
      publisher_->create_datawriter(
        topic, DATAWRITER_QOS_DEFAULT, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK));
    if (!datawriter) throw std::runtime_error(std::string("Failed to create DDS DataWriter!"));
    return datawriter;
  }

  template<typename TopicType>
  typename Traits<TopicType>::DataReaderVar create_datareader(
    DDS::Topic_var topic, DDS::DataReaderListener_var listener = nullptr)
  {
    typename Traits<TopicType>::DataReaderVar datareader = Traits<TopicType>::DataReader::_narrow(
      subscriber_->create_datareader(
        topic, DATAREADER_QOS_DEFAULT, listener, OpenDDS::DCPS::DEFAULT_STATUS_MASK));
    if (!datareader) throw std::runtime_error(std::string("Failed to create DDS DataReader!"));
    return datareader;
  }
};

#endif // OPENDDS_MQTT_EXAMPLES_COMMON_OPENDDS
