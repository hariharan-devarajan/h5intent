//
// Created by haridev on 10/26/22.
//

#ifndef H5INTENT_PROPERTY_DDS_H
#define H5INTENT_PROPERTY_DDS_H
#include <hdf5.h>

struct DatasetAccessProperties {
  struct append_flush {
    bool use;
    unsigned ndims;
    hsize_t* boundary;
  } append_flush;
  struct chunk_cache {
    bool use;
    size_t rdcc_nslots;
    size_t rdcc_nbytes;
    double rdcc_w0;
  } chunk_cache;
  struct virtual_view {
    bool use;
    H5D_vds_view_t view;
  } virtual_view;
  struct filter_avail {
    bool use;
  } filter_avail;
  struct gzip {
    bool use;
    bool deflate;
  } gzip;
  struct layout {
    bool use;
    H5D_layout_t layout;
  } layout;
  struct chunk {
    bool use;
    int ndims;
    hsize_t* dim;
    unsigned opts;
  } chunk;
  struct szip {
    bool use;
    unsigned options_mask;
    unsigned pixels_per_block;
  } szip;
};
struct DatasetTransferProperties {
  struct dmpiio {
    bool use;
    H5FD_mpio_xfer_t xfer_mode;
    H5FD_mpio_collective_opt_t coll_opt_mode;
    H5FD_mpio_chunk_opt_t chunk_opt_mode;
    unsigned num_chunk_per_proc;
    unsigned percent_num_proc_per_chunk;
  } dmpiio;
  struct buffer {
    bool use;
    size_t size;
    char expression[256];
  } buffer;
  struct edc_check {
    bool use;
    bool check;
  } edc_check;
  struct hyper_vector {
    bool use;
    unsigned ndims;
    hsize_t* size;
  } hyper_vector;
  struct mem_manager {
    bool use;
  } mem_manager;
  struct dataset_io_hyperslab_selection {
    bool use;
    unsigned rank;
    H5S_seloper_t op;
    hsize_t* start;
    hsize_t* stride;
    hsize_t* count;
    hsize_t* block;
  } dataset_io_hyperslab_selection;
};

struct FileAccessProperties {
  struct alignment {
    bool use;
    size_t threshold;
    hbool_t alignment_value;
  } alignment;
  struct core {
    bool use;
    size_t increment;
    hbool_t backing_store;
  } core;
  struct direct {
    bool use;
    size_t alignment;
    size_t block_size;
    size_t cbuf_size;
  } direct;
  struct family {
    bool use;
    hsize_t memb_size;
    hid_t memb_fapl_id;
  } family;
  struct log {
    bool use;
    char logfile[256];
    unsigned long long flags;
    size_t buf_size;
  } log;
  struct fmpiio {
    bool use;
    char comm[256];
  } fmpiio;
  struct split {
    bool use;
  } split;
  struct stdio {
    bool use;
  } stdio;
  struct cache {
    bool use;
    int mdc_nelmts;
    size_t rdcc_nslots;
    size_t rdcc_nbytes;
    double rdcc_w0;
  } cache;
  struct write_tracking {
    bool use;
    size_t page_size;
  } write_tracking;
  struct close {
    bool use;
    bool evict;
    char degree[256];
  } close;
  struct file_image {
    bool use;
    size_t buf_len;
  } file_image;
  struct optimizations {
    bool use;
    bool file_locking;
    unsigned gc_ref;
    size_t sieve_buf_size;
    hsize_t small_data_block_size;
    bool enable_object_flush_cb;
  } optimizations;
  struct metadata {
    bool use;
    H5AC_cache_config_t config_ptr;
    bool enable_logging;  // H5Pset_mdc_log_options
    hsize_t meta_block_size;
    bool enable_coll_metadata_write;  // H5Pset_coll_metadata_write
  } metadata;
  struct page_buffer {
    bool use;
    size_t buf_size;
    unsigned min_meta_per;
    unsigned min_raw_per;
  } page_buffer;
};

struct FileCreationProperties {
  struct file_space {
    bool use;
    hsize_t file_space_page_size;
    H5F_fspace_strategy_t strategy;
    hbool_t persist;
    hsize_t threshold;
  } file_space;
  struct istore {
    bool use;
    unsigned ik;
  } istore;
  struct sizes {
    bool use;
    size_t sizeof_addr;
    size_t sizeof_size;
  } sizes;
};

struct MapAccessProperties {
  struct map_iterate {
    bool use;
    size_t key_prefetch_size;
    size_t key_alloc_size;
  } map_iterate;
};

struct ObjectCreationProperties {
  struct track_times {
    bool use;
  } track_times;
};

struct DatasetProperties {
  struct DatasetAccessProperties access;
  struct DatasetTransferProperties transfer;
};
struct FileProperties {
  struct FileAccessProperties access;
  struct FileCreationProperties creation;
};

#ifdef __cplusplus
#include <nlohmann/json.hpp>
#include <string>
#include <any>
struct HDF5Properties {
  std::unordered_map<std::string,DatasetProperties> datasets;
  std::unordered_map<std::string,FileProperties> files;
  MapAccessProperties mAccess;
  ObjectCreationProperties oCreation;
  HDF5Properties() : datasets(), files(), mAccess(), oCreation() {}
  HDF5Properties(const HDF5Properties& other)
      : datasets(other.datasets),
        files(other.files),
        mAccess(other.mAccess),
        oCreation(other.oCreation) {}
  HDF5Properties(const HDF5Properties&& other)
      : datasets(other.datasets),
        files(other.files),
        mAccess(other.mAccess),
        oCreation(other.oCreation) {}
  HDF5Properties& operator=(const HDF5Properties& other) {
    this->datasets = other.datasets;
    this->files = other.files;
    this->mAccess = other.mAccess;
    this->oCreation = other.oCreation;
    return *this;
  }
};
enum SharingPattern {
    INDEPENDENT=0,
    COLLECTIVE=1,
    OTHER=2
};

enum FileMode{
    FILE_WRITE_ONLY=0,
    FILE_READ_ONLY=1,
    FILE_READ_WRITE=2,
    FILE_APPEND=3,
};
enum AccessPatternType{
    AP_WRITE_ONLY = 0,
    AP_READ_ONLY = 1,
    AP_RAW = 2,
    AP_OTHER = 3
};

struct MultiSessionIO {
    float* open_timestamp;
    float* close_timestamp;
    float* read_timestamp;
    float* write_timestamp;
};

typedef std::unordered_map<std::string, std::unordered_map<std::string, std::any>> TopAccessedSegments;

struct DatasetIOIntents {
    std::string filename;
    std::string dataset_name;
    size_t ndims;
    MultiSessionIO multiSessionIo;
    AccessPatternType type;
    TopAccessedSegments top_accessed_segments;
    std::unordered_map<std::string, size_t> transfer_size_dist;
    std::vector<size_t> process_sharing;
    size_t fs_size;
    SharingPattern sharing_pattern;
    FileMode mode;
};

struct FileIOIntents {
    std::string filename;
    MultiSessionIO session_io;
    FileMode mode;
    size_t fs_size;
    SharingPattern sharing_pattern;
    std::unordered_map<std::string, size_t> ap_distribution;
    std::unordered_map<std::string, std::unordered_map<std::string,size_t>> transfer_size_dist;
    std::vector<size_t> process_sharing;
    std::unordered_map<std::string,size_t> ds_size_dist;
};

struct Intents {
    std::unordered_map<std::string, DatasetIOIntents> datasets;
    std::unordered_map<std::string, FileIOIntents> files;
    Intents() : datasets(), files() {}
    Intents(const Intents& other)
            : datasets(other.datasets),
              files(other.files) {}
    Intents(const Intents&& other)
            : datasets(other.datasets),
              files(other.files) {}
    Intents& operator=(const Intents& other) {
        this->datasets = other.datasets;
        this->files = other.files;
        return *this;
    }
};

using json = nlohmann::json;

template <typename Enumeration>
auto as_integer(Enumeration const value) -> typename std::underlying_type<Enumeration>::type {
    return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

#define TO_JSON_D_ENUM(ATTR) j[#ATTR] = as_integer(p.ATTR)
#define FROM_JSON_D_ENUM(ATTR) \
  if (j.contains(#ATTR) && !j.at(#ATTR).is_null()) \
      j.at(#ATTR).get_to(p.ATTR);
#define TO_JSON(CAT, ATTR) j[#CAT][#ATTR] = p.CAT.ATTR
#define TO_JSON_S(CAT, ATTR) j[#CAT][#ATTR] = std::string(p.CAT.ATTR)
#define FROM_JSON(CAT, ATTR) \
  if (j.contains(#CAT) && !j[#CAT].is_null() && \
      j[#CAT].contains(#ATTR) && !j[#CAT].at(#ATTR).is_null()) \
      j[#CAT].at(#ATTR).get_to(p.CAT.ATTR);
#define FROM_JSON_S(CAT, ATTR) \
  if (j.contains(#CAT) && !j[#CAT].is_null() && \
      j[#CAT].contains(#ATTR) && !j[#CAT].at(#ATTR).is_null()) \
      strcpy(p.CAT.ATTR, j[#CAT].at(#ATTR).get_ref<const std::string&>().c_str());

#define TO_JSON_D(ATTR) j[#ATTR] = p.ATTR
#define TO_JSON_D_OBJ(ATTR)  to_json(j[#ATTR], p.ATTR)
#define FROM_JSON_D_OBJ(ATTR)  from_json(j[#ATTR], p.ATTR)
#define FROM_JSON_D(ATTR) \
  if (j.contains(#ATTR) && !j.at(#ATTR).is_null()) \
    j.at(#ATTR).get_to(p.ATTR);
#define FROM_JSON_D_V(ATTR) \
  if (j.contains(#ATTR) && !j.at(#ATTR).is_null()) \
    p.ATTR = j.at(#ATTR).get<decltype(p.ATTR)>();

#define TO_JSON_ARRAY(CAT, ATTR, SIZE) \
  to_json(j[#CAT][#ATTR], p.CAT.ATTR, p.CAT.SIZE)
#define TO_JSON_D_ARRAY(ATTR, SIZE) \
  to_json(j[#ATTR], p.ATTR, p.ATTR.SIZE)
#define TO_JSON_D_ARRAY_FIXED(ATTR, SIZE) \
  to_json(j[#ATTR], p.ATTR, SIZE)

#define FROM_JSON_ARRAY(CAT, ATTR, SIZE) \
  if (j.contains(#CAT) && !j[#CAT].is_null() && \
      j[#CAT].contains(#ATTR) && !j[#CAT].at(#ATTR).is_null()) \
    from_json(j[#CAT][#ATTR], p.CAT.ATTR, p.CAT.SIZE)

#define FROM_JSON_D_ARRAY(ATTR, SIZE) \
  if (j.contains(#ATTR) && !j.at(#ATTR).is_null()) \
    from_json(j[#ATTR], p.ATTR, p.SIZE)

#define FROM_JSON_D_ARRAY_FIXED(ATTR, SIZE) \
  if (j.contains(#ATTR) && !j.at(#ATTR).is_null()) \
    from_json(j[#ATTR], p.ATTR, SIZE)

template <class myType>
inline void to_json(json& j, myType* array, const unsigned len) {
    j = json::array();
    for (int i = 0; i < len; ++i) {
        j.push_back(array[i]);
    }
}
template <class myType>
inline void from_json(const json& j, myType*& array, int expected_length) {
    auto vec = j.get<std::vector<myType>>();
    int actual_length = vec.size();
    assert(actual_length == expected_length);
    array = new myType[actual_length];
    for (int i = 0; i < actual_length; ++i) {
        array[i] = vec[i];
    }
}

inline void from_json(const json& j, long unsigned int& p) {
    j.get_to(p);
}

template <class Value>
inline void from_json(const json& j, std::vector<Value>& p) {
    auto vec = j.get<std::vector<Value>>();
    p = std::vector<Value>();
    p.insert(p.end(), vec.begin(), vec.end());
}
inline void from_json(const json& j, std::any& p) {
    if (j.type() == json::value_t::array) {
        auto vec = std::vector<int>();
        from_json(j, vec);
        p = vec;
    } else if (j.type() == json::value_t::number_unsigned) {
        int a;
        j.get_to(a);
        p = a;
    }
}
template <class Key, class Value>
inline void from_json(const json& j, std::unordered_map<Key,Value>& p) {
    //auto jmap = j.get<std::unordered_map<Key,json>>();
    p = std::unordered_map<Key,Value>();
    for (auto& el : j.items()){
        Value val;
        if (el.value().type() == json::value_t::object or std::is_same<Value, std::any>::value) {
            from_json(el.value(), val);
        } else {
            el.value().get_to(val);
        }

        p.emplace(el.key(), val);
    }
}


inline void to_json(json& j, const long unsigned int& p) {
    j = json();
    j = p;
}
#define ANY_CAST(lhs, rhs, TYPE) \
if (rhs.type() == typeid(TYPE)) lhs = std::any_cast<TYPE>(rhs);
inline void to_json(json& j, const std::any& p) {
    j = json();
    ANY_CAST(j,p,int);
    ANY_CAST(j,p,size_t);
    ANY_CAST(j,p,std::vector<int>);
}
template <class Key, class Value>
inline void to_json(json& j, const std::unordered_map<Key,Value>& p) {
    j = json();
    for (auto it: p) {
        to_json(j[it.first], it.second);
    }
}
template <class Value>
inline void to_json(json& j, const std::vector<Value>& p) {
    j = json::array();
    for (auto val:p) {
        j.push_back(val);
    }

}

inline void to_json(json& j, const MultiSessionIO& p) {
    j = json();
    TO_JSON_D_ARRAY_FIXED(open_timestamp, 2);
    TO_JSON_D_ARRAY_FIXED(close_timestamp, 2);
    TO_JSON_D_ARRAY_FIXED(read_timestamp, 2);
    TO_JSON_D_ARRAY_FIXED(write_timestamp, 2);
}

inline void from_json(const json& j, MultiSessionIO& p) {
    FROM_JSON_D_ARRAY_FIXED(open_timestamp, 2);
    FROM_JSON_D_ARRAY_FIXED(close_timestamp, 2);
    FROM_JSON_D_ARRAY_FIXED(read_timestamp, 2);
    FROM_JSON_D_ARRAY_FIXED(write_timestamp, 2);
}

inline void to_json(json& j, const DatasetIOIntents& p) {
  j = json();
  TO_JSON_D(filename);
  TO_JSON_D(dataset_name);
  TO_JSON_D(ndims);
  TO_JSON_D(multiSessionIo);
  TO_JSON_D_ENUM(type);
  TO_JSON_D_OBJ(top_accessed_segments);
  TO_JSON_D_OBJ(transfer_size_dist);
  TO_JSON_D_OBJ(process_sharing);
  TO_JSON_D(fs_size);
  TO_JSON_D_ENUM(sharing_pattern);
  TO_JSON_D_ENUM(mode);
}

inline void from_json(const json& j, DatasetIOIntents& p) {
  FROM_JSON_D(filename);
  FROM_JSON_D(dataset_name);
  FROM_JSON_D(ndims);
  FROM_JSON_D(multiSessionIo);
  FROM_JSON_D_ENUM(type);
  FROM_JSON_D_OBJ(top_accessed_segments);
  FROM_JSON_D_OBJ(transfer_size_dist);
  FROM_JSON_D_OBJ(process_sharing);
  FROM_JSON_D(fs_size);
  FROM_JSON_D_ENUM(sharing_pattern);
  FROM_JSON_D_ENUM(mode);
}

inline void to_json(json& j, const FileIOIntents& p) {
    j = json();
    TO_JSON_D(filename);
    TO_JSON_D(session_io);
    TO_JSON_D_ENUM(mode);
    TO_JSON_D(fs_size);
    TO_JSON_D_ENUM(sharing_pattern);
    TO_JSON_D_OBJ(ap_distribution);
    TO_JSON_D_OBJ(transfer_size_dist);
    TO_JSON_D_OBJ(process_sharing);
    TO_JSON_D_OBJ(ds_size_dist);
}
inline void from_json(const json& j, FileIOIntents& p) {
    FROM_JSON_D(filename);
    FROM_JSON_D(session_io);
    FROM_JSON_D_ENUM(mode);
    FROM_JSON_D(fs_size);
    FROM_JSON_D_ENUM(sharing_pattern);
    FROM_JSON_D_OBJ(ap_distribution);
    FROM_JSON_D_OBJ(transfer_size_dist);
    FROM_JSON_D_OBJ(process_sharing);
    FROM_JSON_D_OBJ(ds_size_dist);
}
inline void to_json(json& j, const Intents& p) {
  j = json();
  TO_JSON_D(files);
  TO_JSON_D(datasets);
}
inline void from_json(const json& j, Intents& p) {
  FROM_JSON_D(files);
  FROM_JSON_D(datasets);
}
#endif

#endif  // H5INTENT_PROPERTY_DDS_H
