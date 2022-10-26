//
// Created by haridev on 10/25/22.
//

#ifndef H5INTENT_CONFIGURATION_LOADER_H
#define H5INTENT_CONFIGURATION_LOADER_H

#ifdef __cplusplus


#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ConfigurationLoader {
 public:
  void load_configuration(const std::string& configuration_file);
};
#else
void load_configuration(const char* file);
#endif

#endif  // H5INTENT_CONFIGURATION_LOADER_H
