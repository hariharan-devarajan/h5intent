//
// Created by haridev on 10/19/22.
//

#ifndef H5INTENT_UTIL_H
#define H5INTENT_UTIL_H
/*
 * Copyright (C) 2019  SCS Lab <scs-help@cs.iit.edu>, Hariharan
 * Devarajan <hdevarajan@hawk.iit.edu>, Luke Logan
 * <llogan@hawk.iit.edu>, Xian-He Sun <sun@iit.edu>
 *
 * This file is part of HCompress
 *
 * HCompress is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
/*-------------------------------------------------------------------------
 *
 * Created: util.h
 * June 11 2018
 * Hariharan Devarajan <hdevarajan@hdfgroup.org>
 *
 * Purpose:This contains common utility functions for the test cases
 *
 *
 * INPUTS:
 * -l
 *[layer_count]#[capacity_mb]_[bandwidth]_[is_memory]_[mount_point]_[io_size]#...
 * -i [io_size_]
 * -n [iteration_]
 * -f [pfs_path]
 * -d [direct_io]
 * -b [enable_hermes]
 * -c [compression_library]
 *-------------------------------------------------------------------------
 */
#include <assert.h>
#include <execinfo.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zconf.h>

#include <cstdint>

const uint32_t MB = 1024 * 1024;

/**
 * Handles signals and prints stack trace.
 *
 * @param sig
 */
void handler_c(int sig) {
  void* array[10];
  size_t size;
  // get void*'s for all entries on the stack
  size = backtrace(array, 300);
  int rank, comm_size;
  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(0);
}
int run_command(char* cmd) {
  int ret;
  ret = system(cmd);
  return ret >> 8;
}

typedef struct InputArgs {
  size_t io_size_;
  char* pfs_path;
  size_t iteration_;
  size_t direct_io_;
  int computation_ms;
  bool debug;
} InputArgs;

static char** str_split(char* a_str, const char a_delim) {
  char** result = 0;
  size_t count = 0;
  char* tmp = a_str;
  char* last_comma = 0;
  char delim[2];
  delim[0] = a_delim;
  delim[1] = 0;

  /* Count how many elements will be extracted. */
  while (*tmp) {
    if (a_delim == *tmp) {
      count++;
      last_comma = tmp;
    }
    tmp++;
  }

  /* Add space for trailing token. */
  count += last_comma < (a_str + strlen(a_str) - 1);

  /* Add space for terminating null string so caller
     knows where the list of returned strings ends. */
  count++;

  result = (char**)malloc(sizeof(char*) * count);

  if (result) {
    size_t idx = 0;
    char* token = strtok(a_str, delim);

    while (token) {
      assert(idx < count);
      *(result + idx++) = strdup(token);
      token = strtok(0, delim);
    }
    assert(idx == count - 1);
    *(result + idx) = 0;
  }

  return result;
}

static struct InputArgs parse_opts(int argc, char* argv[]) {
  int flags, opt;
  int nsecs, tfnd;

  nsecs = 0;
  tfnd = 0;
  flags = 0;
  struct InputArgs args;
  args.io_size_ = 1;
  args.pfs_path = nullptr;
  args.iteration_ = 1;
  args.direct_io_ = true;
  args.debug = false;
  args.computation_ms = 0;

  while ((opt = getopt(argc, argv, "i:b:n:f:d:z:")) != -1) {
    switch (opt) {
      case 'i': {
        args.io_size_ = (size_t)atoi(optarg);
        break;
      }
      case 'b': {
        args.computation_ms = atoi(optarg);
        break;
      }
      case 'n': {
        args.iteration_ = (size_t)atoi(optarg);
        break;
      }
      case 'f': {
        args.pfs_path = optarg;
        break;
      }
      case 'd': {
        args.direct_io_ = atoi(optarg);
        break;
      }
      case 'z': {
        args.debug = (bool)atoi(optarg);
        break;
      }
      default: /* '?' */
        fprintf(stderr, "Invalid input string: %s\n", argv[0]);
        fprintf(stderr, "Bad input: %c\n", opt);
        fprintf(stderr, "-i [io_size_per_request]\n");
        fprintf(stderr, "-b [computation time in ms]\n");
        fprintf(stderr, "-n [iterations]\n");
        fprintf(stderr, "-f [pfs_path]\n");
        fprintf(stderr, "-d [direct io true/false]\n");
        fprintf(stderr, "-r [rank per server]\n");
        exit(EXIT_FAILURE);
    }
  }
  return args;
}

static void setup_env(struct InputArgs args) {
  char* homepath = args.pfs_path;
  if (homepath == nullptr) {
    fprintf(stderr, "set pfs variable");
    exit(EXIT_FAILURE);
  }
  ssize_t len = snprintf(NULL, 0, "mkdir -p %s/pfs %s/nvme %s/bb %s/ramfs",
                         homepath, homepath, homepath, homepath);
  char* command = (char*)malloc(len + 1);
  snprintf(command, len + 1, "mkdir -p %s/pfs %s/nvme %s/bb %s/ramfs", homepath,
           homepath, homepath, homepath);
  run_command(command);
  ssize_t len2 =
      snprintf(NULL, 0, "rm -rf %s/pfs/* %s/nvme/* %s/bb/* %s/ramfs/*",
               homepath, homepath, homepath, homepath);
  char* command2 = (char*)malloc(len2 + 1);
  snprintf(command2, len2 + 1, "rm -rf %s/pfs/* %s/nvme/* %s/bb/* %s/ramfs/*",
           homepath, homepath, homepath, homepath);
  run_command(command2);
}

static void clean_env(struct InputArgs args) {
  char* homepath = args.pfs_path;
  if (homepath == nullptr) {
    fprintf(stderr, "set pfs variable");
    exit(EXIT_FAILURE);
  }
  ssize_t len2 =
      snprintf(NULL, 0, "rm -rf %s/pfs/* %s/nvme/* %s/bb/* %s/ramfs/*",
               homepath, homepath, homepath, homepath);
  char* command2 = (char*)malloc(len2 + 1);
  snprintf(command2, len2 + 1, "rm -rf %s/pfs/* %s/nvme/* %s/bb/* %s/ramfs/*",
           homepath, homepath, homepath, homepath);
  run_command(command2);
}
class Timer {
 public:
  Timer() : elapsed_time(0) {}
  void resumeTime() { t1 = std::chrono::high_resolution_clock::now(); }
  double pauseTime() {
    auto t2 = std::chrono::high_resolution_clock::now();
    elapsed_time += std::chrono::duration<double>(t2 - t1).count();
    return elapsed_time;
  }
  double getElapsedTime() { return elapsed_time; }

 private:
  std::chrono::high_resolution_clock::time_point t1;
  double elapsed_time;
};
#endif  // H5INTENT_UTIL_H
