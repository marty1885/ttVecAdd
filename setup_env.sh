export ARCH_NAME=grayskull                                                                   
export TT_METAL_HOME=~/Desktop/devs/tt-metal/
export TT_METAL_ENV=dev

mkdir build

ln -s $(pwd)/vecadd_kernels $TT_METAL_HOME/ttvecadd_kernels
