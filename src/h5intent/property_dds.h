//
// Created by haridev on 10/26/22.
//

#ifndef H5INTENT_PROPERTY_DDS_H
#define H5INTENT_PROPERTY_DDS_H

#include <hdf5.h>

namespace h5intent {
struct DatasetAccessProperties {
  union append_flush {
    bool use;
    unsigned ndims;
    hsize_t* boundary;
  } append_flush;
  union chunk_cache {
    bool use;
    size_t rdcc_nslots;
    size_t rdcc_nbytes;
    double rdcc_w0;
  } chunk_cache;
  union virtual_view {
    bool use;
    H5D_vds_view_t view;
  } virtual_view;
  union filter_avail {
    bool use;
  } filter_avail;
  union gzip {
    bool use;
    bool deflate;
  } gzip;
  union layout {
    bool use;
    H5D_layout_t layout;
  } layout;
  union chunk {
    bool use;
    int max_ndims;
    hsize_t* dims;
    unsigned opts;
  } chunk;
  union szip {
    bool use;
    unsigned options_mask;
    unsigned pixels_per_block;
  } szip;
};

struct DatasetTransferProperties {
  union mpiio {
    bool use;
    H5FD_mpio_xfer_t xfer_mode;
    H5FD_mpio_collective_opt_t coll_opt_mode;
    H5FD_mpio_chunk_opt_t chunk_opt_mode;
    unsigned num_chunk_per_proc;
    unsigned percent_num_proc_per_chunk;
  } mpiio;
  union buffer {
    bool use;
    size_t size;
    std::string expression;
  } buffer;
  union edc_check {
    bool use;
    H5Z_EDC_t check;
  } edc_check;
  union hyper_vector {
    bool use;
    size_t* size;
    size_t ndims;
  } hyper_vector;
  union mem_manager {
    bool use;
  } mem_manager;
  union dataset_io_hyperslab_selection {
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
  union core {
    bool use;
    size_t increment;
    hbool_t backing_store;
  } core;
  union direct {
    bool use;
    size_t alignment;
    size_t block_size;
    size_t cbuf_size;
  } direct;
  union family {
    bool use;
    hsize_t memb_size;
    hid_t memb_fapl_id;
  } family;
  union log {
    bool use;
    std::string logfile;
    unsigned long long flags;
    size_t buf_size;
  } log;
  union mpiio {
    bool use;
    std::string comm;
  } mpiio;
  union split {
    bool use;
  } split;
  union stdio {
    bool use;
  } stdio;
  union cache {
    bool use;
    int mdc_nelmts;
    size_t rdcc_nslots;
    size_t rdcc_nbytes;
    double rdcc_w0;
  } cache;
  union write_tracking {
    bool use;
    size_t page_size;
  } write_tracking;
  union close {
    bool use;
    bool evict;
    H5F_close_degree_t degree;
  } close;
  union file_image {
    bool use;
    size_t buf_len;
  } file_image;
  union optimizations {
    bool use;
    bool file_locking;
    unsigned gc_ref;
    size_t sieve_buf_size;
    hsize_t small_data_block_size;
    bool enable_object_flush_cb;
  } optimizations;
  union metadata {
    bool use;
    H5AC_cache_config_t config_ptr;
    bool enable_logging;  // H5Pset_mdc_log_options
    hsize_t meta_block_size;
    bool enable_coll_metadata_write;  // H5Pset_coll_metadata_write
  } metadata;
  union page_buffer {
    bool use;
    size_t buf_size;
    unsigned min_meta_per;
    unsigned min_raw_per;
  } page_buffer;
};

struct FileCreationProperties {
  union file_space {
    bool use;
    hsize_t file_space_page_size;
    H5F_fspace_strategy_t strategy;
    hbool_t persist;
    hsize_t threshold;
  } file_space;
  union istore {
    bool use;
    unsigned ik;
  } istore;
  union sizes {
    bool use;
    size_t sizeof_addr;
    size_t sizeof_size;
  } sizes;
};

struct MapAccessProperties {
  union map_iterate {
    bool use;
    size_t key_prefetch_size;
    size_t key_alloc_size;
  } map_iterate;
};

struct ObjectCreationProperties {
  union track_times {
    bool use;
  } track_times;
};

#define TO_JSON(CAT, ATTR) j[#CAT][#ATTR] = p.CAT.ATTR
#define FROM_JSON(CAT, ATTR) j[#CAT].at(#ATTR).get_to(p.CAT.ATTR);

#define TO_JSON_D(ATTR) j[#ATTR] = p.ATTR
#define FROM_JSON_D(ATTR) j.at(#ATTR).get_to(p.ATTR);

#define TO_JSON_ARRAY(CAT, ATTR, SIZE) to_json(j[#CAT][#ATTR], p.CAT.ATTR, p.CAT.SIZE)
#define FROM_JSON_ARRAY(CAT, ATTR, SIZE) from_json(j[#CAT][#ATTR], p.CAT.ATTR, p.CAT.SIZE)

inline void to_json(json& j, const hsize_t* array, const unsigned len) {
  j = json::array();
  for (int i = 0; i < len; ++i) {
    j.push_back(array[i]);
  }
}
inline void from_json(const json& j, hsize_t*& array, int len) {
  auto vec = j.get<std::vector<hsize_t>>();
  len = vec.size();
  array = new hsize_t[len];
  for (int i = 0; i < len; ++i) {
    array[i] = vec[i];
  }
}
inline void to_json(json& j, const DatasetAccessProperties& p) {
  j = json();
  /* append_flush */
  j["append_flush"] = json();
  TO_JSON(append_flush, use);
  TO_JSON_ARRAY(append_flush, boundary, ndims);

  /* chunk_cache */
  j["chunk_cache"] = json();
  TO_JSON(chunk_cache, use);
  TO_JSON(chunk_cache, rdcc_nslots);
  TO_JSON(chunk_cache, rdcc_nbytes);
  TO_JSON(chunk_cache, rdcc_w0);

  /* virtual_view */
  j["virtual_view"] = json();
  TO_JSON(virtual_view, use);
  TO_JSON(virtual_view, view);

  /* filter_avail */
  j["filter_avail"] = json();
  TO_JSON(filter_avail, use);

  /* layout */
  j["layout"] = json();
  TO_JSON(layout, use);
  TO_JSON(layout, layout);

  /* chunk */
  j["chunk"] = json();
  TO_JSON(chunk, use);
  TO_JSON_ARRAY(chunk, dims, max_ndims);
  TO_JSON(chunk, opts);

  /* szip */
  j["szip"] = json();
  TO_JSON(szip, use);
  TO_JSON(szip, options_mask);
  TO_JSON(szip, pixels_per_block);
}

inline void from_json(const json& j, DatasetAccessProperties& p) {
  /* append_flush */
  FROM_JSON(append_flush, use);
  FROM_JSON_ARRAY(append_flush, boundary, ndims);

  /* chunk_cache */
  FROM_JSON(chunk_cache, use);
  FROM_JSON(chunk_cache, rdcc_nslots);
  FROM_JSON(chunk_cache, rdcc_nbytes);
  FROM_JSON(chunk_cache, rdcc_w0);

  /* virtual_view */
  FROM_JSON(virtual_view, use);
  FROM_JSON(virtual_view, view);

  /* filter_avail */
  FROM_JSON(filter_avail, use);

  /* layout */
  FROM_JSON(layout, use);
  FROM_JSON(layout, layout);

  /* chunk */
  FROM_JSON(chunk, use);
  FROM_JSON_ARRAY(chunk, dims, max_ndims);
  FROM_JSON(chunk, opts);

  /* szip */
  FROM_JSON(szip, use);
  FROM_JSON(szip, options_mask);
  FROM_JSON(szip, pixels_per_block);
}

inline void to_json(json& j, const DatasetTransferProperties& p) {
  j = json();
  /*mpiio*/
  j["mpiio"] = json();
  TO_JSON(mpiio, use);
  TO_JSON(mpiio, xfer_mode);
  TO_JSON(mpiio, coll_opt_mode);
  TO_JSON(mpiio, chunk_opt_mode);
  TO_JSON(mpiio, num_chunk_per_proc);
  TO_JSON(mpiio, percent_num_proc_per_chunk);

  /*buffer*/
  j["buffer"] = json();
  TO_JSON(buffer, use);
  TO_JSON(buffer, size);
  TO_JSON(buffer, expression);

  /*edc_check*/
  j["edc_check"] = json();
  TO_JSON(edc_check, use);
  TO_JSON(edc_check, check);

  /*hyper_vector*/
  j["hyper_vector"] = json();
  TO_JSON(hyper_vector, use);
  TO_JSON_ARRAY(hyper_vector, size, ndims);

  /*mem_manager*/
  j["mem_manager"] = json();
  TO_JSON(mem_manager, use);

  /*dataset_io_hyperslab_selection*/
  j["dataset_io_hyperslab_selection"] = json();
  TO_JSON(dataset_io_hyperslab_selection, use);
  TO_JSON(dataset_io_hyperslab_selection, rank);
  TO_JSON_ARRAY(dataset_io_hyperslab_selection, start, rank);
  TO_JSON_ARRAY(dataset_io_hyperslab_selection, stride, rank);
  TO_JSON_ARRAY(dataset_io_hyperslab_selection, count, rank);
  TO_JSON_ARRAY(dataset_io_hyperslab_selection, block, rank);
}
inline void from_json(const json& j, DatasetTransferProperties& p) {
  /*mpiio*/
  FROM_JSON(mpiio, use);
  FROM_JSON(mpiio, xfer_mode);
  FROM_JSON(mpiio, coll_opt_mode);
  FROM_JSON(mpiio, chunk_opt_mode);
  FROM_JSON(mpiio, num_chunk_per_proc);
  FROM_JSON(mpiio, percent_num_proc_per_chunk);

  /*buffer*/
  FROM_JSON(buffer, use);
  FROM_JSON(buffer, size);
  FROM_JSON(buffer, expression);

  /*edc_check*/
  FROM_JSON(edc_check, use);
  FROM_JSON(edc_check, check);

  /*hyper_vector*/
  FROM_JSON(hyper_vector, use);
  FROM_JSON_ARRAY(hyper_vector, size, ndims);

  /*mem_manager*/
  FROM_JSON(mem_manager, use);

  /*dataset_io_hyperslab_selection*/
  FROM_JSON(dataset_io_hyperslab_selection, use);
  FROM_JSON(dataset_io_hyperslab_selection, rank);
  FROM_JSON_ARRAY(dataset_io_hyperslab_selection, start, rank);
  FROM_JSON_ARRAY(dataset_io_hyperslab_selection, stride, rank);
  FROM_JSON_ARRAY(dataset_io_hyperslab_selection, count, rank);
  FROM_JSON_ARRAY(dataset_io_hyperslab_selection, block, rank);
}
inline void to_json(json& j, const FileAccessProperties& p) {
  j = json();
  /*core*/
  j["core"] = json();
  TO_JSON(core, use);
  TO_JSON(core, increment);
  TO_JSON(core, backing_store);

  /*family*/
  j["family"] = json();
  TO_JSON(family, use);
  TO_JSON(family, memb_size);
  TO_JSON(family, memb_fapl_id);

  /*log*/
  j["log"] = json();
  TO_JSON(log, use);
  TO_JSON(log, logfile);
  TO_JSON(log, flags);
  TO_JSON(log, buf_size);

  /*mpiio*/
  j["mpiio"] = json();
  TO_JSON(mpiio, use);
  TO_JSON(mpiio, comm);

  /*split*/
  j["split"] = json();
  TO_JSON(split, use);

  /*stdio*/
  j["stdio"] = json();
  TO_JSON(stdio, use);

  /*cache*/
  j["cache"] = json();
  TO_JSON(cache, use);
  TO_JSON(cache, mdc_nelmts);
  TO_JSON(cache, rdcc_nslots);
  TO_JSON(cache, rdcc_nbytes);
  TO_JSON(cache, rdcc_w0);

  /*write_tracking*/
  j["write_tracking"] = json();
  TO_JSON(write_tracking, use);
  TO_JSON(write_tracking, page_size);

  /*close*/
  j["close"] = json();
  TO_JSON(close, use);
  TO_JSON(close, evict);
  TO_JSON(close, degree);

  /*file_image*/
  j["file_image"] = json();
  TO_JSON(file_image, use);
  TO_JSON(file_image, buf_len);

  /*optimizations*/
  j["optimizations"] = json();
  TO_JSON(optimizations, use);
  TO_JSON(optimizations, file_locking);
  TO_JSON(optimizations, gc_ref);
  TO_JSON(optimizations, sieve_buf_size);
  TO_JSON(optimizations, small_data_block_size);
  TO_JSON(optimizations, enable_object_flush_cb);

  /*metadata*/
  j["metadata"] = json();
  TO_JSON(metadata, use);
  //TO_JSON(metadata, config_ptr);
  TO_JSON(metadata, enable_logging);
  TO_JSON(metadata, meta_block_size);
  TO_JSON(metadata, enable_coll_metadata_write);

  /*metadata*/
  j["page_buffer"] = json();
  TO_JSON(page_buffer, use);
  TO_JSON(page_buffer, buf_size);
  TO_JSON(page_buffer, min_meta_per);
  TO_JSON(page_buffer, min_raw_per);
}
inline void from_json(const json& j, FileAccessProperties& p) {
  /*core*/
  FROM_JSON(core, use);
  FROM_JSON(core, increment);
  FROM_JSON(core, backing_store);

  /*family*/
  FROM_JSON(family, use);
  FROM_JSON(family, memb_size);
  FROM_JSON(family, memb_fapl_id);

  /*log*/
  FROM_JSON(log, use);
  FROM_JSON(log, logfile);
  FROM_JSON(log, flags);
  FROM_JSON(log, buf_size);

  /*mpiio*/
  FROM_JSON(mpiio, use);
  FROM_JSON(mpiio, comm);

  /*split*/
  FROM_JSON(split, use);

  /*stdio*/
  FROM_JSON(stdio, use);

  /*cache*/
  FROM_JSON(cache, use);
  FROM_JSON(cache, mdc_nelmts);
  FROM_JSON(cache, rdcc_nslots);
  FROM_JSON(cache, rdcc_nbytes);
  FROM_JSON(cache, rdcc_w0);

  /*write_tracking*/
  FROM_JSON(write_tracking, use);
  FROM_JSON(write_tracking, page_size);

  /*close*/
  FROM_JSON(close, use);
  FROM_JSON(close, evict);
  FROM_JSON(close, degree);

  /*file_image*/
  FROM_JSON(file_image, use);
  FROM_JSON(file_image, buf_len);

  /*optimizations*/
  FROM_JSON(optimizations, use);
  FROM_JSON(optimizations, file_locking);
  FROM_JSON(optimizations, gc_ref);
  FROM_JSON(optimizations, sieve_buf_size);
  FROM_JSON(optimizations, small_data_block_size);
  FROM_JSON(optimizations, enable_object_flush_cb);

  /*metadata*/
  FROM_JSON(metadata, use);
  //FROM_JSON(metadata, config_ptr);
  FROM_JSON(metadata, enable_logging);
  FROM_JSON(metadata, meta_block_size);
  FROM_JSON(metadata, enable_coll_metadata_write);

  /*metadata*/
  FROM_JSON(page_buffer, use);
  FROM_JSON(page_buffer, buf_size);
  FROM_JSON(page_buffer, min_meta_per);
  FROM_JSON(page_buffer, min_raw_per);
}
inline void to_json(json& j, const H5AC_cache_config_t& p) {
  j = json();
  TO_JSON_D(version);
  TO_JSON_D(rpt_fcn_enabled);
  TO_JSON_D(open_trace_file);
  TO_JSON_D(close_trace_file);
  TO_JSON_D(trace_file_name);
  TO_JSON_D(evictions_enabled);
  TO_JSON_D(set_initial_size);
  TO_JSON_D(initial_size);
  TO_JSON_D(min_clean_fraction);
  TO_JSON_D(max_size);
  TO_JSON_D(min_size);
  TO_JSON_D(epoch_length);
  TO_JSON_D(incr_mode);
  TO_JSON_D(lower_hr_threshold);
  TO_JSON_D(increment);
  TO_JSON_D(apply_max_increment);
  TO_JSON_D(max_increment);
  TO_JSON_D(flash_incr_mode);
  TO_JSON_D(flash_multiple);
  TO_JSON_D(flash_threshold);
  TO_JSON_D(decr_mode);
  TO_JSON_D(upper_hr_threshold);
  TO_JSON_D(decrement);
  TO_JSON_D(apply_max_decrement);
  TO_JSON_D(max_decrement);
  TO_JSON_D(epochs_before_eviction);
  TO_JSON_D(apply_empty_reserve);
  TO_JSON_D(empty_reserve);
  TO_JSON_D(dirty_bytes_threshold);
  TO_JSON_D(metadata_write_strategy);
}
inline void from_json(const json& j, H5AC_cache_config_t& p) {
  FROM_JSON_D(version);
  FROM_JSON_D(rpt_fcn_enabled);
  FROM_JSON_D(open_trace_file);
  FROM_JSON_D(close_trace_file);
  FROM_JSON_D(trace_file_name);
  FROM_JSON_D(evictions_enabled);
  FROM_JSON_D(set_initial_size);
  FROM_JSON_D(initial_size);
  FROM_JSON_D(min_clean_fraction);
  FROM_JSON_D(max_size);
  FROM_JSON_D(min_size);
  FROM_JSON_D(epoch_length);
  FROM_JSON_D(incr_mode);
  FROM_JSON_D(lower_hr_threshold);
  FROM_JSON_D(increment);
  FROM_JSON_D(apply_max_increment);
  FROM_JSON_D(max_increment);
  FROM_JSON_D(flash_incr_mode);
  FROM_JSON_D(flash_multiple);
  FROM_JSON_D(flash_threshold);
  FROM_JSON_D(decr_mode);
  FROM_JSON_D(upper_hr_threshold);
  FROM_JSON_D(decrement);
  FROM_JSON_D(apply_max_decrement);
  FROM_JSON_D(max_decrement);
  FROM_JSON_D(epochs_before_eviction);
  FROM_JSON_D(apply_empty_reserve);
  FROM_JSON_D(empty_reserve);
  FROM_JSON_D(dirty_bytes_threshold);
  FROM_JSON_D(metadata_write_strategy);
}
}

#endif  // H5INTENT_PROPERTY_DDS_H
