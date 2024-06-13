# Instrument the binary 
 omnitrace-instrument -o gpu_daxpy.inst -- gpu_daxpy

# Run omnitrace with the instrumented binary
srun -n 2 omnitrace-run -- gpu_daxpy.inst 1000 10000