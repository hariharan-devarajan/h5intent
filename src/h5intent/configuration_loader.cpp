//
// Created by haridev on 10/25/22.
//

#include <h5intent/configuration_loader.h>

#include <fstream>
#include "property_dds.h"
#include "singleton.h"
#include <filesystem>
std::string getexepath() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return std::string(result, (count > 0) ? count : 0);
}

std::string sh(std::string cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr) {
            result += buffer.data();
        }
    }
    return result;
}
void print_backtrace(void) {
    void *bt[1024];
    int bt_size;
    char **bt_syms;
    int i;

    bt_size = backtrace(bt, 1024);
    bt_syms = backtrace_symbols(bt, bt_size);
    std::regex re("\\[(.+)\\]");
    auto exec_path = getexepath();
    std::string addrs = "";
    for (i = 1; i < bt_size; i++) {
        std::string sym = bt_syms[i];
        std::smatch ms;
        if (std::regex_search(sym, ms, re)) {
            std::string m = ms[1];
            addrs += " " + m;
        }
    }
    auto r = sh("addr2line -e " + exec_path + " -f -C " + addrs);
    std::cout << r << std::endl;
    free(bt_syms);
}

extern void signal_handler(int sig){
    switch(sig) {
        case SIGHUP:{
            INTENT_LOGPRINT("hangup signal caught",0);
            break;
        }
        case SIGTERM:{
            INTENT_LOGPRINT("terminate signal caught",0);
            MPI_Finalize();
            exit(0);
            break;
        }
        default:{
            //print_backtrace();
            /*void *array[20];
            size_t size;
            // get void*'s for all entries on the stack
            size = backtrace(array, 20);
            // print out all the frames to stderr
            backtrace_symbols_fd(array, size, STDERR_FILENO);
            */
            void *trace[16];
            char **messages = (char **)NULL;
            int i, trace_size = 0;

            void *array[20];
            size_t size;
            size = backtrace(array, 20);
            messages = backtrace_symbols(array, size);
            /* skip first stack frame (points here) */
            std::stringstream myString;
            myString << "[bt] Execution path with signal "<<sig<< ":\n";
            for (i=1; i<size; ++i)
            {
                //printf("%s\n", messages[i]);
                std::string m_string(messages[i]);
                //./prog(myfunc3+0x5c) [0x80487f0]
                std::size_t open_paren = m_string.find_first_of("(");
                if (open_paren == std::string::npos) {
                    myString << "[bt] #"<< i <<" " << messages[i] << "\n";
                    continue;
                }
                std::size_t plus = m_string.find_first_of("+");
                std::size_t close_paren = m_string.find_first_of(")");
                std::size_t open_square = m_string.find_first_of("[");
                std::size_t close_square = m_string.find_first_of("]");
                std::string prog_name = m_string.substr(0, open_paren);
                std::string func_name = m_string.substr(open_paren + 1, plus - open_paren - 1);
                std::string offset = m_string.substr(plus + 1, close_paren - plus -1);
                std::string addr = m_string.substr(open_square + 1, close_square - open_square -1);
                if (func_name.empty()) {
                    myString << "[bt] #"<< i <<" " << messages[i] << "\n";
                    continue;
                }
                char command[256];
                sprintf(command,"nm %s | grep \"\\s%s$\" | awk '{print $1}'", prog_name.c_str(), func_name.c_str());
                std::string base_addr=sh(command);
                sprintf(command,"python2 -c \"print hex(0x%s+%s)\"", base_addr.c_str(), offset.c_str());
                std::string hex_val=sh(command);
                sprintf(command,"addr2line -e %s %s", prog_name.c_str(), hex_val.c_str());
                std::string line=sh(command); // line has a new line char already
                myString << "[bt] #"<< i <<" " << prog_name << "(" << func_name  << "+" << offset << ")" << line;
            }
            //std::string str = myString.str();
            //WISIO_LOGERROR("%s\n", str.c_str());
            std::string res = myString.str();
            std::cout << res;
            ::raise(SIGTERM);
        }

    }
}

extern void set_signal() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGABRT,&sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}
extern void load_configuration(const char* file) {
  h5intent::Singleton<h5intent::ConfigurationManager>::get_instance()->load_configuration(
      file);
}
extern char* fix_filename(const char* filename) {
    std::filesystem::path posix_path{filename};
    char* fixed = (char*)calloc(posix_path.generic_string().size(), sizeof(char));
    strcpy(fixed, posix_path.generic_string().c_str());
    return fixed;
}
void h5intent::ConfigurationManager::                                                                                                                                                                                                                                                                                                                         load_configuration(
    const std::string& configuration_file) {
  std::ifstream t(configuration_file);
  t.seekg(0, std::ios::end);
  size_t size = t.tellg();
  std::string buffer(size, ' ');
  t.seekg(0);
  t.read(&buffer[0], size);
  t.close();
  json read_json = json::parse(buffer);
  read_json.get_to(intents);
  printf("# of datasets %d, # of files %d, from conf %s\n", intents.datasets.size(),
         intents.files.size(),
         configuration_file.c_str());
}
DatasetProperties to_dataset_properties(DatasetIOIntents &intents) {
    return DatasetProperties();
}
FileProperties to_file_properties(FileIOIntents &intents) {
    return FileProperties();
}
bool get_dataset_properties(const char* dataset_name, struct DatasetProperties *datasetProperties) {
  auto intents = h5intent::Singleton<h5intent::ConfigurationManager>::get_instance()->intents;
  auto iter = intents.datasets.find(dataset_name);
  if (iter == intents.datasets.end()) return false;
  else {
    *datasetProperties = to_dataset_properties(iter->second);
    return true;
  }
}
bool get_file_properties(const char* filename, struct FileProperties* fileProperties) {
  std::filesystem::path posix_path{filename};
  auto intents = h5intent::Singleton<h5intent::ConfigurationManager>::get_instance()->intents;
  auto iter = intents.files.find(posix_path.generic_string());
  if (iter == intents.files.end()) return false;
  else {
    *fileProperties = to_file_properties(iter->second);
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
      auto conf_len = property.size();
      *selected_conf =
          static_cast<char*>(malloc(conf_len + 1));
      strcpy(*selected_conf, property.c_str());
      (*selected_conf)[conf_len] = '\0';
      return true;
    }
  }
  return false;
}
