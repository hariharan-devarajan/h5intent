//
// Created by haridev on 10/25/22.
//

#ifndef H5INTENT_CONFIGURATION_LOADER_H
#define H5INTENT_CONFIGURATION_LOADER_H

#ifdef __cplusplus
#include <nlohmann/json.hpp>
#include <h5intent/property_dds.h>
#include <cpp-logger/logger.h>


#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <regex>
#include <iostream>
#define INTENT_LOGGER cpplogger::Logger::Instance("H5INTENT")
#define INTENT_LOGINFO(format, ...) \
  INTENT_LOGGER->log(cpplogger::LOG_INFO, format, __VA_ARGS__);
#define INTENT_LOGWARN(format, ...) \
  INTENT_LOGGER->log(cpplogger::LOG_WARN, format, __VA_ARGS__);
#define INTENT_LOGERROR(format, ...) \
  INTENT_LOGGER->log(cpplogger::LOG_ERROR, format, __VA_ARGS__);
#define INTENT_LOGPRINT(format, ...) \
  INTENT_LOGGER->log(cpplogger::LOG_PRINT, format, __VA_ARGS__);





using json = nlohmann::json;
namespace h5intent {
class ConfigurationManager {
 public:
  Intents intents;
  ConfigurationManager():intents() {}
  ConfigurationManager(const ConfigurationManager& other):intents(other.intents){}
  ConfigurationManager(ConfigurationManager&& other):intents(other.intents){}
  ConfigurationManager& operator=(const ConfigurationManager& other) {
    this->intents = other.intents;
    return *this;
  }
  void load_configuration(const std::string& configuration_file);

};
}

extern "C" {
#endif
#include <signal.h>
void load_configuration(const char* file);
char* fix_filename(char* file);
bool get_dataset_properties(const char* dataset_name, struct DatasetProperties *properties);
bool get_file_properties(const char* file, struct FileProperties* properties);
bool select_correct_conf(const char* confs, char** selected_conf);
void signal_handler(int sig);
void set_signal();
#ifdef __cplusplus
}
#endif
#endif  // H5INTENT_CONFIGURATION_LOADER_H
