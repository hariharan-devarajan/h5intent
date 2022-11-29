//
// Created by haridev on 11/28/22.
//

#include <catch_config.h>
#include <test_utils.h>

#include <fstream>
#include <iostream>
#include <unordered_set>
#include <h5intent/configuration_loader.h>

namespace h5intent::test {}
namespace it = h5intent::test;
namespace h5intent::test {
struct Arguments {
  std::string json_file;
  bool debug;
};
}
it::Arguments args;
/**
 * Overridden methods for catch
 */
int init(int *argc, char ***argv) {
  return 0;
}
int finalize() {
  return 0;
}

cl::Parser define_options() {
  auto arg = cl::Opt(args.json_file,
                      "json_file")["--json_file"]("json_file.") |
             cl::Opt(args.debug, "debug")["--debug"]("Enable debugging.");
  return arg;
}

TEST_CASE("TestConfig", CONVERT_STR(workflow, args.json_file)){
  auto config_loader = h5intent::ConfigurationLoader();
  config_loader.load_configuration(args.json_file);
  printf("# of file %d, # of datasets %d in %s\n",
         config_loader.properties.files.size(),
         config_loader.properties.datasets.size(),
         args.json_file.c_str());
}
