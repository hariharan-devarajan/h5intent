//
// Created by haridev on 10/25/22.
//

#ifndef H5INTENT_CONFIGURATION_LOADER_H
#define H5INTENT_CONFIGURATION_LOADER_H

#ifdef __cplusplus
#include <nlohmann/json.hpp>
#include <h5intent/property_dds.h>
using json = nlohmann::json;
namespace h5intent {
class ConfigurationManager {
 public:
  HDF5Properties properties;
  ConfigurationManager() = default;
  ConfigurationManager(const ConfigurationManager& other) = default;
  ConfigurationManager(ConfigurationManager&& other)= default;
  ConfigurationManager& operator=(const ConfigurationManager& other) {
    this->properties = other.properties;
    return *this;
  }
  void load_configuration(const std::string& configuration_file);

};
}

extern "C" {
#endif
void load_configuration(const char* file);
bool get_dataset_properties(const char* dataset_name, struct DatasetProperties *properties);
bool get_file_properties(const char* file, struct FileProperties *properties);
bool select_correct_conf(const char* confs, char** selected_conf);
#ifdef __cplusplus
}
#endif
#endif  // H5INTENT_CONFIGURATION_LOADER_H
