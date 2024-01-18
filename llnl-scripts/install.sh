#!/bin/bash

host=$(hostname)
host=${host//[0-9]/}

#builds with Shared LIBS off, examples does not do anything

if [[ $host == "borax" ]]; then
  WITH_CUDA="OFF"
elif [[ $host == "lassen" ]]; then
  WITH_CUDA="ON"
fi

mkdir -p build_${host};
cd build_${host}
build_dir=build_${host}
install_dir=${build_dir}/install
rm -rf ./*
echo $AMS_UMPIRE_PATH
cmake \
-Damqpcpp_DIR=$AMS_AMQPCPP_PATH \
-DBUILD_SHARED_LIBS=On \
-DCMAKE_PREFIX_PATH=$INSTALL_DIR \
-DWITH_CALIPER=On \
-DWITH_EXAMPLES=On \
-DWITH_HDF5=On \
-DWITH_RMQ=On \
-DHDF5_Dir=$AMS_HDF5_PATH \
-DCMAKE_INSTALL_PREFIX=./install \
-DCMAKE_BUILD_TYPE=Debug \
-DUMPIRE_DIR=$AMS_UMPIRE_PATH \
-DMFEM_DIR=$AMS_MFEM_PATH \
-DWITH_FAISS=On \
-DFAISS_DIR=$AMS_FAISS_PATH \
-DWITH_AMS_DEBUG=On \
-DWITH_WORKFLOW=On \
-DTorch_DIR=$AMS_TORCH_PATH \
-DWITH_TORCH=On \
-DWITH_TESTS=On \
-DAMS_CUDA_ARCH=${AMS_CUDA_ARCH} \
-DWITH_CUDA=${WITH_CUDA} \
-DWITH_MPI=On \
../

make -j

#-DWITH_FAISS=On \

#-DFAISS_DIR=$AMS_FAISS_PATH \
#-DFAISS_DIR=/usr/workspace/koparasy/faiss_install/usr/ \
