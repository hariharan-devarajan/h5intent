//
// Created by haridev on 10/25/22.
//

#include <h5intent/configuration_loader.h>

#include <fstream>
#include "property_dds.h"
#include "singleton.h"

extern void load_configuration(const char* file) {
  h5intent::Singleton<h5intent::ConfigurationManager>::get_instance()->load_configuration(
      file);
}
void h5intent::ConfigurationManager::load_configuration(
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
  printf("# of datasets %d, # of files %d\n", properties.datasets.size(), properties.files.size());
}
bool get_dataset_properties(const char* dataset_name, DatasetProperties *datasetProperties) {
  auto properties = h5intent::Singleton<h5intent::ConfigurationManager>::get_instance()->properties;
  auto iter = properties.datasets.find(dataset_name);
  if (iter == properties.datasets.end()) return false;
  else {
    *datasetProperties = iter->second;
    return true;
  }
}
