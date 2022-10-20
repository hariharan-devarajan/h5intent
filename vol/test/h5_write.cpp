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
* Created: h5_buffer_write.cpp
* June 5 2018
* Hariharan Devarajan <hdevarajan@hdfgroup.org>
*
* Purpose:to test the Write to Buffer on DMSH
*
*-------------------------------------------------------------------------
*/

#include <malloc.h>
#include <memory.h>
#include <hdf5.h>
#include <assert.h>
#include <math.h>
#include <chrono>
#include <random>
#include "util.h"
static size_t GetCount(int rank, size_t *dims) {
    size_t count = 1;
    for(int i = 0; i < rank; i++)
        count *= dims[i];
    return count;
}
template <typename T>
static T* UniformDistribution(int rank, size_t *dims, double lower, double upper) {
    static auto beginning = std::chrono::high_resolution_clock::now();
    size_t count = GetCount(rank, dims);
    T *dataset = (T*)malloc(count * sizeof(T));
    auto now = std::chrono::high_resolution_clock::now();

    std::default_random_engine generator(std::chrono::duration<double>(now - beginning).count()*10000);
    std::uniform_real_distribution<double> distribution(lower, upper);

    for(size_t i = 0; i < count; i++) {
        dataset[i] = (T)round(distribution(generator));
    }
    return dataset;
}
int main(int argc, char** argv)
{
    MPI_Init(&argc,&argv);
    struct InputArgs args = parse_opts(argc,argv);
    setup_env(args);
    char file_name[256];
    char* homepath = args.pfs_path;
    if(args.pfs_path == NULL){
        fprintf(stderr, "set pfs variable");
        exit(EXIT_FAILURE);
    }else{
        sprintf(file_name,"%s/test.h5",args.pfs_path);
    }
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if(rank==0 && args.debug){
        printf("ready for attach\n");
        fflush(stdout);
        getchar();
    }
    MPI_Barrier(MPI_COMM_WORLD);
    int rank_ = 2;
    int64_t type_ = H5T_NATIVE_INT;
    size_t num_elements=10;
    if(args.io_size_>0){
        double total_number_of_ints=args.io_size_*MB/(H5Tget_size(type_)*1.0);
        num_elements= (size_t) floor(sqrt(total_number_of_ints));
    }
    printf("num_elements %ld\n",num_elements);
    printf("Iteration %ld\n", args.iteration_);
    hsize_t dims[2]={num_elements,num_elements};
    hsize_t max_dims[2]={num_elements,num_elements};

    hid_t file_id = H5Fcreate(file_name, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    hid_t dataspaceId = H5Screate_simple(rank_, dims, max_dims);

    //97 106 89 97 107 97 94

    char dataset_name[256];
    for(int i=0;i<args.iteration_;i++) {
        sprintf(dataset_name,"/dset_%d",i);
        hid_t datasetId = H5Dcreate2(file_id, dataset_name, H5T_NATIVE_INT, dataspaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        int32_t* array=UniformDistribution<int32_t>(rank_, dims, 0, 1000);
        int32_t* array2= static_cast<int32_t *>(malloc(num_elements * num_elements * sizeof(int)));
        if(H5Dwrite(datasetId, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, array) < 0) {
            printf("Write failed\n");
            exit(1);
        }

        if(H5Dread(datasetId, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, array2) < 0) {
            printf("Read Failed\n");
        }

        if(strncmp((char*)array, (char*)array2, num_elements*num_elements*sizeof(int)) != 0)
            printf("Failed to restore array from Hermes. Data corrupted.\n");
        free(array);
        free(array2);
        H5Dclose(datasetId);
    }

    H5Sclose(dataspaceId);
    H5Fclose(file_id);
    //clean_env(args);
    printf("SUCCESS\n");
    MPI_Finalize();
    return 0;
}
