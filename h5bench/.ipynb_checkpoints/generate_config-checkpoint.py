import argparse
import os
import shutil
import json


def clean_dir(dirname):
    if os.path.exists(dirname) and os.path.isdir(dirname):
        shutil.rmtree(dirname)


def str2bool(v):
    if isinstance(v, bool):
        return v
    if v.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')


def parse_args():
    parser = argparse.ArgumentParser(description='Generate H5Bench Config')
    parser.add_argument("-ppn", "--processes-per-node", default=1, type=int,
                        help="Number of processes per node in job.")
    parser.add_argument("-s", "--sync", default=True, type=str2bool, help="sync mode. y/n")
    parser.add_argument("-d", "--data-dir", default="./data", type=str, help="Directory to produce logs and data")
    parser.add_argument("-sd", "--sample-dir", default="./samples", type=str, help="Original samples from h5bench")
    parser.add_argument("-p", "--profiler", default="", type=str, help="Profiler SO")
    return parser.parse_args()


def main():
    args = parse_args()
    env = ""
    profile_type = "none"
    print(args.profiler)
    if args.profiler:
        env = f"--env LD_PRELOAD={args.profiler}"
        profile_type = os.path.splitext(args.profiler.split("/")[-1])[0]
    print(profile_type)
    mpi = {
        "command": "jsrun",
        "configuration": f"-r 1 -c {args.processes_per_node} -a {args.processes_per_node} {env}"
    }
    vol = {}
    filesystem = {}
    if args.sync:
        sync_str = "sync"
    else:
        sync_str = "async"
    dirname = f"{sync_str}_{args.processes_per_node}_{profile_type}"
    new_sample_dir = os.path.join(os.getcwd(), dirname)
    clean_dir(new_sample_dir)
    os.makedirs(new_sample_dir, exist_ok=True)
    for filename in os.listdir(args.sample_dir):
        only_name = os.path.splitext(filename)[0]
        source = os.path.join(args.sample_dir, filename)
        if os.path.isfile(source) and filename.startswith(sync_str):
            with open(source) as file:
                configuration = json.loads(file.read())
            configuration['mpi'] = mpi
            configuration['vol'] = vol
            configuration['file-system'] = filesystem
            configuration['directory'] = os.path.join(args.data_dir, only_name)
            destination = os.path.join(new_sample_dir, filename)
            with open(destination, 'w') as file:
                json.dump(configuration, file)
            print(f"written configuration for file {filename}")


if __name__ == '__main__':
    main()
    exit(0)
