//
// Created by haridev on 10/25/22.
//

#include <h5intent/configuration_loader.h>

#include <fstream>
#include "property_dds.h"
#include "singleton.h"

void load_configuration(const char* file) {
  h5intent::Singleton<h5intent::ConfigurationLoader>::get_instance()->load_configuration(
      file);
}
void h5intent::ConfigurationLoader::load_configuration(
    const std::string& configuration_file) {
  std::ifstream t(configuration_file);
  t.seekg(0, std::ios::end);
  size_t size = t.tellg();
  std::string buffer(size, ' ');
  t.seekg(0);
  t.read(&buffer[0], size);
  t.close();
  json read_json = json::parse(buffer);
  read_json.get_to(properties);
}
