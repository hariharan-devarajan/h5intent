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
bool get_file_properties(const char* filename, struct FileProperties* fileProperties) {
  auto properties = h5intent::Singleton<h5intent::ConfigurationManager>::get_instance()->properties;
  auto iter = properties.files.find(filename);
  if (iter == properties.files.end()) return false;
  else {
    *fileProperties = iter->second;
    return true;
  }
}
struct tokens: std::ctype<char>
{
  tokens(): std::ctype<char>(get_table()) {}

  static std::ctype_base::mask const* get_table()
  {
    typedef std::ctype<char> cctype;
    static const cctype::mask *const_rc= cctype::classic_table();

    static cctype::mask rc[cctype::table_size];
    std::memcpy(rc, const_rc, cctype::table_size * sizeof(cctype::mask));

    rc[';'] = std::ctype_base::space;
    return &rc[0];
  }
};
bool select_correct_conf(const char* confs, char** selected_conf) {
  std::stringstream ss(confs);
  std::string s;
  std::vector<std::string> v;
  while (getline(ss, s, ':')) {
    v.push_back(s);
  }
  std::string sp;
  std::ifstream("/proc/self/cmdline") >> sp;
  std::replace( sp.begin(), sp.end() - 1, '\000', ' ');
  size_t firstIndex = sp.find_first_of(" ");
  std::string path = sp.substr(0, firstIndex);
  size_t lastIndex = path.find_last_of("/");
  std::string exec = path.substr(lastIndex + 1, -1);
  for (auto property: v) {
    size_t lastindex = property.find_last_of(".");
    std::string rawname = property.substr(0, lastindex);
    lastindex = rawname.find_last_of("/");
    std::string json_name = rawname.substr(lastindex + 1, -1);
    if (strcmp(json_name.c_str(),exec.c_str()) == 0) {
      *selected_conf =
          static_cast<char*>(malloc(property.size()));
      strcpy(*selected_conf, property.c_str());
      return true;
    }
  }
  return false;
}
