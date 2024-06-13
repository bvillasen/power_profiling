

export OMNITRACE_ROOT=$HOME/util/omnitrace_rocm5.4.3
mkdir -p $OMNITRACE_ROOT
cd $OMNITRACE_ROOT

# Create and activate a new python environment
python3 -m venv $OMNITRACE_ROOT/pyenv
source $OMNITRACE_ROOT/pyenv/bin/activate

wget https://github.com/ROCm/omnitrace/releases/latest/download/omnitrace-install.py
python ./omnitrace-install.py --prefix $HOME/util/omnitrace --rocm 5.4