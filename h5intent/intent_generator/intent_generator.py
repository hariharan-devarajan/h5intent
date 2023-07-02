'''
Import
'''
import darshan
from enum import Enum
import json
import argparse
import os
import numpy as np
from pathlib import Path
'''
Data Structures
'''
class SharingPattern(Enum):
    INDEPENDENT=0
    COLLECTIVE=1
    OTHER=2
class FileMode(Enum):
    WRITE_ONLY=0
    READ_ONLY=1
    READ_WRITE=2
    APPEND=3
class AccessPatternType(Enum):
    WRITE_ONLY = 0
    READ_ONLY = 1
    RAW = 2
    OTHER = 3
class MultiSessionIO:
    open_timestamp: tuple() = (0,0)
    close_timestamp: tuple() = (0,0)
    read_timestamp: tuple() = (0,0)
    write_timestamp: tuple() = (0,0)
        
    def __repr__(self):
        return str(self.json())

    def json(self):
        return {
            'open_timestamp': [self.open_timestamp[0], self.open_timestamp[1]],
            'close_timestamp': [self.close_timestamp[0], self.close_timestamp[1]],
            'read_timestamp': [self.read_timestamp[0], self.read_timestamp[1]],
            'write_timestamp': [self.write_timestamp[0], self.write_timestamp[1]],
        }

class DatasetIOIntents:
    #metadata
    filename = None
    dataset_name = None
    ndims = 1
    # primary
    session_io: MultiSessionIO = MultiSessionIO()
    type: AccessPatternType = AccessPatternType.OTHER
    top_accessed_segments = {}
    transfer_size_dist = {}
    process_sharing = []
    fs_size = 0
    sharing_pattern:SharingPattern = SharingPattern.OTHER
    # secondary
    mode: FileMode = FileMode.READ_WRITE
    def __repr__(self):
        return str(self.json())

    def json(self):
        return {
            'filename': self.filename,
            'dataset_name': self.dataset_name,
            'ndims': self.ndims,
            'session_io': self.session_io.json(),
            'type': self.type.value,
            'top_accessed_segments': self.top_accessed_segments,
            'transfer_size_dist': self.transfer_size_dist,
            'process_sharing': self.process_sharing,
            'fs_size': self.fs_size,
            'sharing_pattern': self.sharing_pattern.value,
            'mode': self.mode.value,
        }

class FileIOIntents:
    #metadata
    filename = None
    # primary
    session_io: MultiSessionIO = MultiSessionIO()
    mode: FileMode = FileMode.READ_WRITE
    fs_size = 0
    sharing_pattern:SharingPattern = SharingPattern.OTHER
    # secondary
    ap_distribution = {}
    top_accessed_segments = {}
    transfer_size_dist = {}
    process_sharing = []
    ds_size_dist = {}
    def __repr__(self):
        return str(self.json())

    def json(self):
        return {
            'session_io': self.session_io.json(),
            'mode': self.mode.value,
            'process_sharing': self.process_sharing,
            'fs_size': self.fs_size,
            'sharing_pattern': self.sharing_pattern.value,
            'ap_distribution': self.ap_distribution,
            'top_accessed_segments': self.top_accessed_segments,
            'transfer_size_dist': self.transfer_size_dist,
            'process_sharing': self.process_sharing,
            'ds_size_dist': self.ds_size_dist,
        }
class Intents:
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
Constants
'''
KB = 1024
MB = 1024 * 1024
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
Main Class
'''
class IntentGenerator:
    def __init__(self, base_path, darshan_logs, property_json, workflow, data_dirs):
        self.base_path = base_path
        self.darshan_logs = darshan_logs
        self.property_json = property_json
        self.workflow = workflow
        self.app = {}
        self.data_dirs = data_dirs.split(":")

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
                                      'configuration': Intents()
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
            for data_dir in self.data_dirs:
                #print(data_dir, value)
                if data_dir in value:
                    self.app[app_name]['relevant_ids'].append(key)
                    self.app[app_name]['name_to_id_map'][value] = key                
        print(report.modules.keys())
        self.found_hdf5 = True
        if "H5F" not in report.modules:
            print(f"No HDF5 Module found for {self.workflow}")
            self.found_hdf5 = False
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
    
    def read_dataset(self, app_name, initial=True):
        h5f_df_c = self.app[app_name]['h5f_df_c']
        h5d_df_c = self.app[app_name]['h5d_df_c']
        h5f_df_fc = self.app[app_name]['h5f_df_fc']
        h5d_df_fc = self.app[app_name]['h5d_df_fc']
        report = self.app[app_name]['report']
        file_agg = {}
        #print(h5d_df_c)
        for ind in h5d_df_c.index:
            ds_id = h5d_df_c['id'][ind]
            dataset_fqn = report.data['name_records'][ds_id]
            #print(dataset_fqn)
            dset_split_fqn = dataset_fqn.split(":")
            
            dataset_intents = DatasetIOIntents()
            dataset_intents.filename = dset_split_fqn[0]
            dataset_intents.dataset_name = dataset_fqn
            file_id = self.app[app_name]['name_to_id_map'][dataset_intents.filename]
            if file_id not in file_agg:
                file_agg[file_id] = {
                    'fs_size': 0,
                    'mode': {str(FileMode.READ_ONLY.value): 0,
                            str(FileMode.WRITE_ONLY.value): 0,
                            str(FileMode.READ_WRITE.value): 0,
                            str(FileMode.APPEND.value): 0},
                    'ap_distribution': {str(AccessPatternType.READ_ONLY.value): 0,
                                        str(AccessPatternType.WRITE_ONLY.value): 0,
                                        str(AccessPatternType.RAW.value): 0,
                                        str(AccessPatternType.OTHER.value): 0},
                    'top_accessed_segments': {},
                    'transfer_size_dist': {"1":{"sum":0, "count":0}, "2":{"sum":0, "count":0},
                                            "3":{"sum":0, "count":0},"4":{"sum":0, "count":0}},
                    'process_sharing': set(),
                    'ds_size_dist': {"sum":0, "count":0},
                }
            
            
            find = h5d_df_fc[h5d_df_fc['id'] == ds_id].index[0]
            dataset_intents.session_io.open_timestamp = (h5d_df_fc['H5D_F_OPEN_START_TIMESTAMP'][find],
                                                        h5d_df_fc['H5D_F_OPEN_END_TIMESTAMP'][find])
            dataset_intents.session_io.close_timestamp = (h5d_df_fc['H5D_F_CLOSE_START_TIMESTAMP'][find],
                                                        h5d_df_fc['H5D_F_CLOSE_END_TIMESTAMP'][find])
            dataset_intents.session_io.read_timestamp = (h5d_df_fc['H5D_F_READ_START_TIMESTAMP'][find],
                                                        h5d_df_fc['H5D_F_READ_END_TIMESTAMP'][find])
            dataset_intents.session_io.write_timestamp = (h5d_df_fc['H5D_F_WRITE_START_TIMESTAMP'][find],
                                                        h5d_df_fc['H5D_F_WRITE_END_TIMESTAMP'][find])
            dtype_size = h5d_df_c['H5D_DATATYPE_SIZE'][ind]
            num_elements_written = h5d_df_c['H5D_BYTES_WRITTEN'][ind] / dtype_size
            num_elements_read = h5d_df_c['H5D_BYTES_READ'][ind] / dtype_size
            
            if num_elements_written == 0 and num_elements_read > 0:
                dataset_intents.mode = FileMode.READ_ONLY
                dataset_intents.type = AccessPatternType.READ_ONLY
                file_agg[file_id]['ap_distribution'][str(AccessPatternType.READ_ONLY.value)] += 1
                file_agg[file_id]['mode'][str(FileMode.READ_ONLY.value)] += 1
            elif num_elements_written > 0 and num_elements_read == 0:
                dataset_intents.mode = FileMode.WRITE_ONLY
                dataset_intents.type = AccessPatternType.WRITE_ONLY
                file_agg[file_id]['ap_distribution'][str(AccessPatternType.WRITE_ONLY.value)] += 1
                file_agg[file_id]['mode'][str(FileMode.WRITE_ONLY.value)] += 1
            else:
                dataset_intents.mode = FileMode.READ_WRITE
                file_agg[file_id]['mode'][str(FileMode.READ_WRITE.value)] += 1
                if h5d_df_fc['H5D_F_READ_START_TIMESTAMP'][find] > h5d_df_fc['H5D_F_WRITE_END_TIMESTAMP'][find]:
                    dataset_intents.type = AccessPatternType.RAW
                    file_agg[file_id]['ap_distribution'][str(AccessPatternType.RAW.value)] += 1
                else:
                    dataset_intents.type = AccessPatternType.OTHER
                    file_agg[file_id]['ap_distribution'][str(AccessPatternType.OTHER.value)] += 1
            dataset_intents.ndims = h5d_df_c['H5D_DATASPACE_NDIMS'][ind]
            dataset_intents.top_accessed_segments = {}
            dataset_intents.transfer_size_dist = {}
            for i in range(1,4):
                dataset_intents.top_accessed_segments[str(i)] = {"length": [],
                                           "count": h5d_df_c[f'H5D_ACCESS{i}_COUNT'][ind],
                                           "stride": [],
                                           "access": h5d_df_c[f'H5D_ACCESS{i}_ACCESS'][ind]}
                dataset_intents.transfer_size_dist[str(i)] = 1
                for dim_ind in range(0, dataset_intents.ndims):
                    end_dim = 5 - dim_ind
                    dataset_intents.top_accessed_segments[str(i)]["length"].append(h5d_df_c[f'H5D_ACCESS{i}_LENGTH_D{end_dim}'][ind])
                    dataset_intents.top_accessed_segments[str(i)]["stride"].append(h5d_df_c[f'H5D_ACCESS{i}_STRIDE_D{end_dim}'][ind])
                    dataset_intents.transfer_size_dist[str(i)] *= h5d_df_c[f'H5D_ACCESS{i}_LENGTH_D{end_dim}'][ind]
                file_agg[file_id]['transfer_size_dist'][str(i)]["sum"] += dataset_intents.transfer_size_dist[str(i)]
                file_agg[file_id]['transfer_size_dist'][str(i)]["count"] += 1
                
            dataset_intents.process_sharing = [h5d_df_c['rank'][ind]]
            dataset_intents.sharing_pattern = SharingPattern.INDEPENDENT
            if h5d_df_c['rank'][ind] == -1:
                dataset_intents.sharing_pattern = SharingPattern.COLLECTIVE
                dataset_intents.process_sharing = list(range(self.app[app_name]['num_processes']))
            file_agg[file_id]['process_sharing'].update(dataset_intents.process_sharing)
            dataset_intents.fs_size = h5d_df_c['H5D_BYTES_WRITTEN'][ind] \
                                        if h5d_df_c['H5D_BYTES_WRITTEN'][ind] > h5d_df_c['H5D_BYTES_READ'][ind] \
                                        else h5d_df_c['H5D_BYTES_READ'][ind]
            file_agg[file_id]["fs_size"] += dataset_intents.fs_size
            file_agg[file_id]["ds_size_dist"]["sum"] += dataset_intents.fs_size
            file_agg[file_id]["ds_size_dist"]["count"] += 1
            self.app[app_name]['configuration'].datasets[dataset_intents.dataset_name] = dataset_intents
            #print(dataset_intents)
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
            find = h5f_df_fc[h5f_df_fc['id'] == file_id].index[0]
            file_item = FileIOIntents()
            file_item.filename = report.data['name_records'][file_id]
            file_item.session_io.open_timestamp = (h5f_df_fc['H5F_F_OPEN_START_TIMESTAMP'][find],
                                                    h5f_df_fc['H5F_F_OPEN_END_TIMESTAMP'][find])
            file_item.session_io.close_timestamp = (h5f_df_fc['H5F_F_CLOSE_START_TIMESTAMP'][find],
                                                    h5f_df_fc['H5F_F_CLOSE_END_TIMESTAMP'][find])
            if file_agg_item["mode"][str(FileMode.READ_ONLY.value)] == 0:
                file_item.mode = FileMode.WRITE_ONLY
            elif file_agg_item["mode"][str(FileMode.WRITE_ONLY.value)] == 0:
                file_item.mode = FileMode.READ_ONLY
            else:
                file_item.mode = FileMode.READ_WRITE
            file_item.fs_size = file_agg_item["fs_size"]
            file_item.process_sharing = list(file_agg_item["process_sharing"])
            file_item.sharing_pattern = SharingPattern.INDEPENDENT
            if len(file_item.process_sharing) > 1:
                file_item.sharing_pattern = SharingPattern.COLLECTIVE
            file_item.ap_distribution = file_agg_item["ap_distribution"]
            file_item.top_accessed_segments = file_agg_item["top_accessed_segments"]
            file_item.transfer_size_dist = file_agg_item["transfer_size_dist"]
            file_item.ds_size_dist = file_agg_item["ds_size_dist"]
            self.app[app_name]['configuration'].files[file_item.filename] = file_item
        return self.app[app_name]['configuration']
def parse_args():
    parser = argparse.ArgumentParser(description='Generate H5Bench Config')
    parser.add_argument("--base-path", default="", type=str,
                        help="Base path where darshan logs are present.")
    parser.add_argument("--darshan-logs", default="", type=str, help="Darshan log dir relative to base path")
    parser.add_argument("--property-json", default="", type=str, help="Property json dir relative to base path")
    parser.add_argument("--data-dirs", default="/p/gpfs", type=str, help="Directory to include in analysis")
    return parser.parse_args()


def main():
    args = parse_args()
    folder = f"{args.base_path}/{args.darshan_logs}"
    for workflow in os.listdir(folder):
        print(f"Generating config for workflow {workflow}")
        generator = IntentGenerator(args.base_path, args.darshan_logs, args.property_json, workflow, args.data_dirs)
        generator.parse_apps()
        generator.load_apps()
        generator.write_configurations()

        
if __name__ == '__main__':
    main()
    exit(0)
