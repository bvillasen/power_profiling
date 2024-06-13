module swap PrgEnv-cray PrgEnv-gnu
module load craype-accel-amd-gfx90a
module load cray-python/3.11.5
module load rocm/6.1.1
module load papi

export OMNITRACE_ROOT=$HOME/util/omnitrace_rocm6.1.1
mkdir -p $OMNITRACE_ROOT
cd $OMNITRACE_ROOT

# Create and activate a new python environment
python3 -m venv $OMNITRACE_ROOT/pyenv
source $OMNITRACE_ROOT/pyenv/bin/activate

# Textinfo
export TEXINFO_DIR=$OMNITRACE_ROOT/textinfo
mkdir -p $TEXINFO_DIR
cd $TEXINFO_DIR
wget https://ftp.gnu.org/gnu/texinfo/texinfo-7.0.2.tar.gz
tar -xzvf texinfo-7.0.2.tar.gz
cd texinfo-7.0.2
./configure --prefix=$TEXINFO_DIR && make && make install
export PATH=$TEXINFO_DIR/bin:$PATH

cd $OMNITRACE_ROOT
git clone -b v1.11.2 https://github.com/AMDResearch/omnitrace.git omnitrace-source
# Apply a fix
grep -RiIl 'opt\/rocm' | xargs sed -i 's/opt\/rocm/opt\/rocm-6.1.1/g'

cmake                                       \\
    -B omnitrace-build                      \\
    -D CMAKE_INSTALL_PREFIX=$OMNITRACE_ROOT \\
    -D OMNITRACE_USE_HIP=ON                 \\
    -D OMNITRACE_USE_ROCM_SMI=ON            \\
    -D OMNITRACE_USE_ROCTRACER=ON           \\
    -D OMNITRACE_USE_PYTHON=ON              \\
    -D OMNITRACE_USE_OMPT=ON                \\
    -D OMNITRACE_USE_MPI=ON                 \\
    -D OMNITRACE_USE_MPI_HEADERS=ON         \\
    -D OMNITRACE_BUILD_PAPI=OFF             \\
    -D OMNITRACE_BUILD_LIBUNWIND=ON         \\
    -D OMNITRACE_BUILD_DYNINST=ON           \\
    -D DYNINST_BUILD_TBB=ON                 \\
    -D DYNINST_BUILD_BOOST=ON               \\
    -D DYNINST_BUILD_ELFUTILS=ON            \\
    -D DYNINST_BUILD_LIBIBERTY=ON           \\
    -D CMAKE_C_COMPILER=gcc                 \\
    -D CMAKE_CXX_COMPILER=g++               \\
    omnitrace-source
cmake --build omnitrace-build --target all --parallel 32
cmake --build omnitrace-build --target install
source $OMNITRACE_ROOT/share/omnitrace/setup-env.sh
