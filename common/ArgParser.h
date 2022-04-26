#ifndef OPENDDS_MQTT_EXAMPLES_COMMON_ARG_PARSER_H
#define OPENDDS_MQTT_EXAMPLES_COMMON_ARG_PARSER_H

#include <vector>
#include <list>
#include <string>
#include <iostream>

template <typename Container>
std::string join(const Container& strings, const std::string& with)
{
  bool first = true;
  std::string joined;
  for (const std::string& string : strings) {
    if (first) {
      first = false;
    } else {
      joined += with;
    }
    joined += string;
  }
  return joined;
}

const std::set<std::string> help_option = {"-h", "--help"};

class ArgParser {
public:
  ArgParser(int argc, char* argv[], const std::string& program_name, const std::string& signature)
  : program_name_(program_name)
  , signature_(signature)
  {
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (help_option.count(arg)) {
        usage(std::cout);
        throw 0;
      }
      args_.push_back(arg);
    }
  }

  void error(const std::string& message)
  {
    std::cerr << "ERROR: " << message << std::endl;
    usage(std::cerr);
    throw 1;
  }

  bool get_next_pos_arg(const std::string& name, std::string& value)
  {
    if (args_.empty()) {
      return false;
    }
    value = args_.front();
    args_.pop_front();
    return true;
  }

  std::string get_next_pos_arg(const std::string& name)
  {
    std::string value;
    if (!get_next_pos_arg(name, value)) {
      error("missing " + name + " positional argument");
    }
    return value;
  }

  bool get_next_pos_arg_as_int(const std::string& name, int& value)
  {
    std::string str_value;
    if (!get_next_pos_arg(name, str_value)) {
      return false;
    }
    try {
      value = std::stoi(str_value);
    } catch (const std::exception& ex) {
      error(str_value + " is not a valid int for positional argument " + name);
    }
    return true;
  }

  int get_next_pos_arg_as_int(const std::string& name)
  {
    int value;
    if (!get_next_pos_arg_as_int(name, value)) {
      error("missing " + name + " positional argument");
    }
    return value;
  }

  void done()
  {
    if (args_.size()) {
      std::string args;
      for (const std::string& arg : args_) {
        args += " " + arg;
      }
      error("invalid arguments :" + join(args_, " "));
    }
  }

private:
  void usage(std::ostream& os) const
  {
    const std::string u = "usage: ";
    const std::string i(u.size(), ' ');
    os
      << u << program_name_ << " " << signature_ << std::endl
      << i << program_name_ << " -h|--help" << std::endl;
  }

  const std::string program_name_;
  const std::string signature_;
  std::list<std::string> args_;
};

#endif
