import os
import darshan
import numpy as np
import json
from pathlib import Path
import argparse

'''
Constants
'''
GB = 1024 * 1024 * 1024
AVAIL_NODE_MEMORY_BYTES = 200 * GB

''' encoder '''


class NpEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, np.bool_):
            return bool(obj)
        if isinstance(obj, np.integer):
            return int(obj)
        if isinstance(obj, np.floating):
            return float(obj)
        if isinstance(obj, np.ndarray):
            return obj.tolist()
        if hasattr(obj, 'json'):
            return obj.json()
        return super(NpEncoder, self).default(obj)


'''
Data structures
'''


class DatasetAccessProperties:
    def __init__(self):
        self.append_flush = None
        self.chunk_cache = None
        self.virtual_view = None
        self.filter_avail = None
        self.gzip = None
        self.layout = None
        self.chunk = None
        self.szip = None

    def __repr__(self):
        return str(self.json())

    def json(self):
        return {
            'append_flush': self.append_flush,
            'chunk_cache': self.chunk_cache,
            'virtual_view': self.virtual_view,
            'filter_avail': self.filter_avail,
            'gzip': self.gzip,
            'layout': self.layout,
            'chunk': self.chunk,
            'szip': self.szip,
        }


class DatasetTransferProperties:
    def __init__(self):
        self.dmpiio = None
        self.buffer = None
        self.edc_check = None
        self.hyper_vector = None
        self.mem_manager = None
        self.dataset_io_hyperslab_selection = None

    def __repr__(self):
        return str(self.json())

    def json(self):
        return {
            'dmpiio': self.dmpiio,
            'buffer': self.buffer,
            'edc_check': self.edc_check,
            'hyper_vector': self.hyper_vector,
            'mem_manager': self.mem_manager,
            'dataset_io_hyperslab_selection': self.dataset_io_hyperslab_selection
        }


class DataSet:
    def __init__(self):
        self.filename = None
        self.dataset_name = None
        self.access = DatasetAccessProperties()
        self.transfer = DatasetTransferProperties()

    def __repr__(self):
        return str(self.json())

    def json(self):
        return {
            'filename': self.filename,
            'dataset_name': self.dataset_name,
            'access': self.access.json(),
            'transfer': self.transfer.json()
        }


class FileAccessProperties:
    def __init__(self):
        self.core = None
        self.direct = None
        self.family = None
        self.log = None
        self.fmpiio = None
        self.split = None
        self.stdio = None
        self.cache = None
        self.write_tracking = None
        self.close = None
        self.file_image = None
        self.optimizations = None
        self.metadata = None
        self.page_buffer = None

    def __repr__(self):
        return str(self.json())

    def json(self):
        return {
            'core': self.core,
            'direct': self.direct,
            'family': self.family,
            'log': self.log,
            'fmpiio': self.fmpiio,
            'split': self.split,
            'stdio': self.stdio,
            'cache': self.cache,
            'write_tracking': self.write_tracking,
            'close': self.close,
            'file_image': self.file_image,
            'optimizations': self.optimizations,
            'metadata': self.metadata,
            'page_buffer': self.page_buffer
        }


class FileCreationProperties:
    def __init__(self):
        self.file_space = None
        self.istore = None
        self.sizes = None

    def __repr__(self):
        return str(self.json())

    def json(self):
        return {
            'file_space': self.file_space,
            'istore': self.istore,
            'sizes': self.sizes,
        }


class File:
    def __init__(self):
        self.filename = None
        self.access = FileAccessProperties()
        self.creation = FileCreationProperties()

    def __repr__(self):
        return str(self.json())

    def json(self):
        return {
            'filename': self.filename,
            'access': self.access.json(),
            'creation': self.creation.json()
        }


class Configuration:
    def __init__(self):
        self.files = {}
        self.datasets = {}

    def __repr__(self):
        return str(self.json())

    def json(self):
        return {
            'files': self.files,
            'datasets': self.datasets
        }


'''
Main Class
'''


class IntentGenerator:
    def __init__(self, base_path, darshan_logs, property_json, workflow):
        self.base_path = base_path
        self.darshan_logs = darshan_logs
        self.property_json = property_json
        self.workflow = workflow
        self.app = {}

    def parse_apps(self):
        folder = f"{self.base_path}/{self.darshan_logs}/{self.workflow}"
        for file in os.listdir(folder):
            if file.endswith(".darshan"):
                app_name = "_".join(file.split("_")[1:-3])
                self.app[app_name] = {'path': os.path.join(folder, file),
                                      'report': None,
                                      'relevant_ids': [],
                                      'name_to_id_map': {},
                                      'num_processes': 0,
                                      'file_agg': {},
                                      'h5f_df_c': None,
                                      'h5d_df_c': None,
                                      'h5f_df_fc': None,
                                      'h5d_df_fc': None,
                                      'configuration': Configuration()
                                      }
        # print(self.app)
        return self.app

    def load_apps(self):
        for app_name, value in self.app.items():
            self.load_darshan_log(app_name)
            if self.found_hdf5:
                self.read_dataset(app_name)
                self.read_file(app_name)
        return self.app

    def write_configurations(self):
        json_files = []
        for app_name, value in self.app.items():
            if self.found_hdf5:
                json_object = json.dumps(self.app[app_name]['configuration'], cls=NpEncoder, indent=2)
                folder = f"{self.base_path}/{self.property_json}/{self.workflow}"
                if not os.path.exists(folder):
                    Path(folder).mkdir(parents=True)
                json_filename = f"{folder}/{app_name}.json"
                json_files.append(json_filename)
                with open(json_filename, "w") as outfile:
                    outfile.write(json_object)
        return json_files

    def load_darshan_log(self, app_name):
        report = darshan.DarshanReport(self.app[app_name]['path'], read_all=True)
        self.app[app_name]['num_processes'] = report.data['metadata']['job']['nprocs']

        for key, value in report.data['name_records'].items():
            if "/p/gpfs1" in value or "/home/haridev/temp" in value:
                self.app[app_name]['relevant_ids'].append(key)
                self.app[app_name]['name_to_id_map'][value] = key
        print(report.modules.keys())
        self.found_hdf5 = True
        if "H5F" not in report.modules:
            print(f"No HDF5 Module found for {self.workflow}")
            self.found_hdf5 = False
        # report.mod_read_all_records('POSIX')
        # report.mod_read_all_records('STDIO')
        # report.mod_read_all_records('MPI-IO')
        if self.found_hdf5:
            report.mod_read_all_records('H5F')
            report.mod_read_all_records('H5D')
            h5f_df_c = report.records['H5F'].to_df()['counters']
            h5f_df_fc = report.records['H5F'].to_df()['fcounters']
            h5d_df_c = report.records['H5D'].to_df()['counters']
            h5d_df_fc = report.records['H5D'].to_df()['fcounters']
            self.app[app_name]['h5f_df_c'] = h5f_df_c[h5f_df_c['id'].isin(self.app[app_name]['relevant_ids'])]
            self.app[app_name]['h5d_df_c'] = h5d_df_c[h5d_df_c['id'].isin(self.app[app_name]['relevant_ids'])]
            self.app[app_name]['h5f_df_fc'] = h5f_df_fc[h5f_df_fc['id'].isin(self.app[app_name]['relevant_ids'])]
            self.app[app_name]['h5d_df_fc'] = h5d_df_fc[h5d_df_fc['id'].isin(self.app[app_name]['relevant_ids'])]
        self.app[app_name]['report'] = report
        return report

    def read_dataset(self, app_name):
        h5f_df_c = self.app[app_name]['h5f_df_c']
        h5d_df_c = self.app[app_name]['h5d_df_c']
        h5f_df_fc = self.app[app_name]['h5f_df_fc']
        h5d_df_fc = self.app[app_name]['h5d_df_fc']
        report = self.app[app_name]['report']
        file_agg = {}
        for ind in h5d_df_c.index:
            ds_id = h5d_df_c['id'][ind]
            dataset_fqn = report.data['name_records'][ds_id]
            print(dataset_fqn)
            dataset = DataSet()
            dset_split_fqn = dataset_fqn.split(":")
            dataset.filename = dset_split_fqn[0]
            file_id = self.app[app_name]['name_to_id_map'][dataset.filename]
            if file_id not in file_agg:
                file_agg[file_id] = {
                    'file_size': 0,
                    'write-only': True,
                    'read-only': True,
                    'raw': False,
                    'close': 0,
                    'ts_sum': 0,
                    'ts_count': 0

                }
            find = h5d_df_fc[h5d_df_fc['id'] == ds_id].index[0]
            # print(find, h5d_df_fc['id'][find])
            file_agg[file_id]['close'] = h5d_df_fc['H5D_F_CLOSE_END_TIMESTAMP'][find] if \
            h5d_df_fc['H5D_F_CLOSE_END_TIMESTAMP'][find] > file_agg[file_id]['close'] else file_agg[file_id]['close']
            file_agg[file_id]['file_size'] = file_agg[file_id]['file_size'] + h5d_df_c['H5D_BYTES_WRITTEN'][ind]

            dataset.dataset_name = dset_split_fqn[1]
            dtype_size = h5d_df_c['H5D_DATATYPE_SIZE'][ind]
            # print(h5d_df_c['H5D_DATATYPE_SIZE'][ind], h5d_df_c['H5D_BYTES_WRITTEN'][ind], h5d_df_c['H5D_BYTES_READ'][ind])
            num_elements_written = h5d_df_c['H5D_BYTES_WRITTEN'][ind] / dtype_size
            num_elements_read = h5d_df_c['H5D_BYTES_READ'][ind] / dtype_size

            if num_elements_written == 0 and num_elements_read > 0:
                file_agg[file_id]['write-only'] = False
            else:
                file_agg[file_id]['read-only'] = False

            if h5d_df_fc['H5D_F_READ_START_TIMESTAMP'][find] > h5d_df_fc['H5D_F_WRITE_END_TIMESTAMP'][find]:
                file_agg[file_id]['raw'] = True

            ndims = h5d_df_c['H5D_DATASPACE_NDIMS'][ind]
            boundary = [0] * ndims
            chunks = [0] * ndims
            stride = [0] * ndims
            start = [0] * ndims
            count = [1] * ndims
            ts = 1
            for dim_ind in range(0, ndims):
                end_dim = 5 - dim_ind
                boundary[dim_ind] = h5d_df_c[f'H5D_ACCESS1_LENGTH_D{end_dim}'][ind] * h5d_df_c['H5D_ACCESS1_COUNT'][ind]
                chunks[dim_ind] = h5d_df_c[f'H5D_ACCESS1_LENGTH_D{end_dim}'][ind]
                stride[dim_ind] = h5d_df_c[f'H5D_ACCESS1_STRIDE_D{end_dim}'][ind]
                ts = ts * chunks[dim_ind]
            file_agg[file_id]['ts_sum'] = file_agg[file_id]['ts_sum'] + ts
            file_agg[file_id]['ts_count'] = file_agg[file_id]['ts_count'] + 1

            dataset.access.append_flush = {'use': True,
                                           'ndims': ndims,
                                           'boundary': boundary
                                           }
            rdcc_w0 = 0

            if num_elements_read == h5d_df_c['H5D_DATASPACE_NPOINTS'][ind] or num_elements_written == \
                    h5d_df_c['H5D_DATASPACE_NPOINTS'][ind]:
                rdcc_w0 = 1
            elif num_elements_written == 0 or num_elements_read == 0:
                rdcc_w0 = 1
            else:
                per_re_read = num_elements_written * 1.0 / (num_elements_written + num_elements_read)
                per_re_write = num_elements_read * 1.0 / (num_elements_written + num_elements_read)
                rdcc_w0 = per_re_read if per_re_read > per_re_write else per_re_write
            dataset.access.chunk_cache = {'use': True,
                                          'rdcc_nslots': h5d_df_c['H5D_ACCESS1_COUNT'][ind],
                                          'rdcc_nbytes': h5d_df_c['H5D_DATATYPE_SIZE'][ind] *
                                                         h5d_df_c['H5D_DATASPACE_NPOINTS'][ind],
                                          'rdcc_w0': rdcc_w0
                                          }
            raw_data_size = h5d_df_c['H5D_DATASPACE_NPOINTS'][ind] * dtype_size
            # TODO(hari) set  H5D_CONTIGUOUS VS H5D_CHUNKED based on for certain things we need 
            # chunking (filtering extending, compression) row-major access is better is better 
            # contiguous vs if we access chunk or strides then we should chunk dataset according.
            # TODO(hari) for H5Pset_dxpl_mpio_chunk_opt, H5Pset_dxpl_mpio_chunk_opt_num, 
            # H5Pset_dxpl_mpio_chunk_opt_ratio See page 83 ff. of this presentation
            # https://urldefense.us/v3/__https://www.alcf.anl.gov/files/Parallel_HDF5_1.pdf__;!!G2kpM7uM-TzIFchu!h30Sfdy1_Q-CXHTkSCJ-VzPvGcxB5_djNTSI2Five-xaaE8j2tyan9AYZJQZUAsm-h4W$ 
            # 
            layout = 0
            if raw_data_size <= 65520:
                layout = 1
            elif raw_data_size <= 4 * 1024 * 1024 * 1024:
                layout = 2
            else:
                layout = 3
            dataset.access.layout = {
                'use': True,
                'layout': layout
            }
            dataset.access.chunk = {
                'use': True,
                'ndims': ndims,
                'dim': chunks,
            }
            procs_accessing_ds = 1
            if h5d_df_c['rank'][ind] == -1:
                procs_accessing_ds = self.app[app_name]['num_processes']

            dataset.transfer.dmpiio = {
                'use': True,
                'xfer_mode': h5d_df_c['H5D_USE_MPIIO_COLLECTIVE'][ind],
                'coll_opt_mode': h5d_df_c['H5D_USE_MPIIO_COLLECTIVE'][ind],
                # TODO(hari): set chunk_opt_mode
                'num_chunk_per_proc': h5d_df_c['H5D_ACCESS1_COUNT'][ind] / procs_accessing_ds,
                'percent_num_proc_per_chunk': procs_accessing_ds * 100 / self.app[app_name]['num_processes']
            }
            # TODO(hari): set this for specific apps where read needs to be correct
            dataset.transfer.edc_check = {
                'use': True,
                'check': False
            }
            dataset.transfer.hyper_vector = {
                'use': True,
                'size': boundary,
                'ndims': ndims
            }
            # TODO(hari): Check if datatype of dataset is variable on runtime.
            dataset.transfer.mem_manager = {
                'use': False
            }

            ops = -1
            if num_elements_written > 0 and num_elements_read == 0:
                ops = 0
            elif num_elements_read > 0 and num_elements_written == 0:
                ops = 1
            else:
                ops = 2

            dataset.transfer.dataset_io_hyperslab_selection = {
                'use': True,
                'rank': ndims,
                'op': 1,  # H5S_SELECT_OR
                'start': start,
                'stride': stride,
                'count': count,
                'block': chunks
            }
            self.app[app_name]['configuration'].datasets[dataset.dataset_name] = dataset
        self.app[app_name]['file_agg'] = file_agg
        return self.app[app_name]['configuration'], self.app[app_name]['file_agg']

    def read_file(self, app_name):
        h5f_df_c = self.app[app_name]['h5f_df_c']
        h5d_df_c = self.app[app_name]['h5d_df_c']
        h5f_df_fc = self.app[app_name]['h5f_df_fc']
        h5d_df_fc = self.app[app_name]['h5d_df_fc']
        report = self.app[app_name]['report']
        file_agg = self.app[app_name]['file_agg']
        for ind in h5f_df_c.index:
            file_id = h5f_df_c['id'][ind]
            file_agg_item = file_agg[file_id]
            file_item = File()
            file_item.filename = report.data['name_records'][file_id]

            if file_agg[file_id]['file_size'] < AVAIL_NODE_MEMORY_BYTES:
                file_item.access.core = {
                    'use': True,
                    'increment': file_agg[file_id]['file_size'],
                    'backing_store': not file_agg[file_id]['read-only']
                }
            file_item.access.fmpiio = {
                'use': True,
                'comm': 'MPI_COMM_WORLD' if h5f_df_c['rank'][ind] == -1 else 'MPI_COMM_SELF'
            }
            file_item.access.split = {
                'use': True if file_agg[file_id]['read-only'] else False
            }
            file_item.access.stdio = {
                'use': True if file_agg[file_id]['write-only'] else False
            }
            file_item.access.cache = {
                'use': False,
                'rdcc_nslots': 0,
                'rdcc_nbytes': 0,
                'rdcc_w0': 0
            }
            is_stride_used = False
            for key, value in self.app[app_name]['configuration'].datasets.items():
                cache = self.app[app_name]['configuration'].datasets[key].access.chunk_cache
                file_item.access.cache['use'] = file_item.access.cache['use'] and cache['use']
                file_item.access.cache['rdcc_nslots'] = file_item.access.cache['rdcc_nslots'] + cache['rdcc_nslots']
                file_item.access.cache['rdcc_nbytes'] = file_item.access.cache['rdcc_nbytes'] + cache['rdcc_nbytes']
                file_item.access.cache['rdcc_w0'] = max(file_item.access.cache['rdcc_w0'], cache['rdcc_w0'])
                hyperslab_selection = self.app[app_name]['configuration'].datasets[
                    key].transfer.dataset_io_hyperslab_selection
                for dim in range(hyperslab_selection['rank']):
                    if hyperslab_selection['stride'][dim] > 0:
                        is_stride_used = True
            # TODO: write_tracking when to enable
            file_item.access.close = {
                'use': True,
                'evict': True if file_agg[file_id]['read-only'] or file_agg[file_id]['write-only'] else False,
                'degree': 'H5F_CLOSE_STRONG' if file_agg[file_id]['close'] < h5f_df_fc['H5F_F_CLOSE_END_TIMESTAMP'][
                    ind] else 'H5F_CLOSE_WEAK',
            }
            file_item.access.file_image = {
                'use': file_agg[file_id]['file_size'] < AVAIL_NODE_MEMORY_BYTES,
                'buf_len': file_agg[file_id]['file_size']
            }
            file_item.access.optimization = {
                'use': True,
                'file_locking': not file_agg[file_id]['read-only'],
                'gc_ref': False,  # TODO(hari) fix me
                'sieve_buf_size': 0 if not is_stride_used else file_agg[file_id]['ts_sum'] * 1.0 / file_agg[file_id][
                    'ts_count'],
                'small_data_block_size': 0 if file_agg[file_id]['file_size'] > GB else file_agg[file_id]['file_size'],
                'enable_object_flush_cb': True if not file_agg[file_id]['read-only'] else False
            }
            file_item.access.page_buffer = {
                'use': True,
                'buf_size': file_agg[file_id]['file_size'] if file_agg[file_id][
                                                                  'file_size'] < AVAIL_NODE_MEMORY_BYTES else AVAIL_NODE_MEMORY_BYTES,
                'min_raw_per': 100 if file_agg[file_id]['file_size'] < AVAIL_NODE_MEMORY_BYTES else int(
                    file_agg[file_id]['file_size'] * 100.0 / AVAIL_NODE_MEMORY_BYTES),
                'min_meta_per': 0,  # TODO(hari) fix me
            }
            file_item.creation.file_space = {
                'use': True,
                'file_space_page_size': file_agg[file_id]['ts_sum'] * 1.0 / file_agg[file_id]['ts_count'],
                'strategy': 1,
                'persist': True,
                'threshold': file_agg[file_id]['ts_sum'] * 1.0 / file_agg[file_id]['ts_count'],
            }
            offset_bytes_needed = 0
            offset = file_agg[file_id]['ts_sum']
            while offset != 0:
                offset = offset >> 8;
                offset_bytes_needed = offset_bytes_needed + 1
            length_bytes_needed = 0
            ts = int(file_agg[file_id]['ts_sum'] * 1.0 / file_agg[file_id]['ts_count'])
            while ts != 0:
                ts = ts >> 8;
                length_bytes_needed = length_bytes_needed + 1;

            file_item.creation.sizes = {
                'use': True,
                'sizeof_addr': offset_bytes_needed,
                'sizeof_size': length_bytes_needed + 1
            }
            self.app[app_name]['configuration'].files[file_item.filename] = file_item
        return self.app[app_name]['configuration']


def parse_args():
    parser = argparse.ArgumentParser(description='Generate H5Bench Config')
    parser.add_argument("--base-path", default="", type=str,
                        help="Base path where darshan logs are present.")
    parser.add_argument("--darshan-logs", default="", type=str, help="Darshan log dir relative to base path")
    parser.add_argument("--property-json", default="", type=str, help="Property json dir relative to base path")
    return parser.parse_args()


def main():
    args = parse_args()
    folder = f"{args.base_path}/{args.darshan_logs}"
    for workflow in os.listdir(folder):
        print(f"Generating config for workflow {workflow}")
        generator = IntentGenerator(args.base_path, args.darshan_logs, args.property_json, workflow)
        generator.parse_apps()
        generator.load_apps()
        generator.write_configurations()


if __name__ == '__main__':
    main()
    exit(0)
