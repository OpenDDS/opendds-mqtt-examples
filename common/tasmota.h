#ifndef OPENDDS_MQTT_EXAMPLES_COMMON_TASMOTA_H
#define OPENDDS_MQTT_EXAMPLES_COMMON_TASMOTA_H

#include <tasmotaTypeSupportImpl.h>

#include <rapidjson/document.h>

#include <map>
#include <string>
#include <stdexcept>
#include <cctype>

bool starts_with(const std::string& what, const std::string& prefix)
{
  return !what.compare(0, prefix.size(), prefix);
}

bool ends_with(const std::string& what, const std::string& suffix)
{
  return what.size() >= suffix.size() &&
    !what.compare(what.size() - suffix.size(), suffix.size(), suffix);
}

std::string to_lower(std::string s)
{
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
  return s;
}

std::string replace(
  const std::string& input, const std::map<std::string, std::string>& replace_map)
{
  std::string output;
  output.reserve(input.size());
  std::string name;
  bool in_name = false;
  for (size_t i = 0; i < input.size(); ++i) {
    const char c = input[i];
    if (c == '%') {
      if (in_name) {
        auto it = replace_map.find(name);
        if (it == replace_map.end()) {
          throw std::runtime_error("No value for %" + name + "%");
        }
        output += it->second;
        in_name = false;
        name = "";
      } else {
        in_name = true;
        continue;
      }
    } else if (in_name) {
      name += c;
    } else {
      output += c;
    }
  }
  return output;
}

std::string get_string_from_object(rapidjson::Value& object, const char* name)
{
  if (!object.HasMember(name) || !object[name].IsString()) {
    throw std::runtime_error(std::string("Couldn't get string member ") + name);
  }
  return object[name].GetString();
}

bool is_tasmota_config(const std::string& topic)
{
  return starts_with(topic, "tasmota/discovery/") && ends_with(topic, "/config");
}

tasmota::Config get_tasmota_config(const std::string& json)
{
  tasmota::Config config;
  rapidjson::Document document;
  document.Parse(json.c_str());
  config.dn() = get_string_from_object(document, "dn");
  config.ft() = get_string_from_object(document, "ft");
  config.t() = get_string_from_object(document, "t");
  return config;
}

inline std::string get_tasmota_topic(
  const tasmota::Config& config, const std::string& prefix, const std::string& suffix)
{
  return replace(config.ft(), {
    {"prefix", prefix},
    {"topic", config.t()},
  }) + suffix;
}

using Configs = std::map<std::string, tasmota::Config>;

static const std::set<std::string> on_values{"on", "1", "true"};
static const std::set<std::string> off_values{"off", "0", "false"};

tasmota::Power get_tasmota_power(const std::string& device, const std::string& message)
{
  tasmota::Power power;
  power.device() = device;
  const std::string lc = to_lower(message);
  if (on_values.count(lc)) {
    power.on() = true;
  } else if (off_values.count(lc)) {
    power.on() = false;
  } else {
    throw std::runtime_error(std::string("Unrecognized power value: ") + message);
  }
  return power;
}

std::string toggle_tasmota_power(const tasmota::Power& power)
{
  return *(power.on() ? off_values : on_values).begin();
}

#endif // OPENDDS_MQTT_EXAMPLES_COMMON_TASMOTA_H
