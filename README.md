# ttVecAdd

A simple vector-addition demo on Tenstorrent devices using tt-Metalium.

This demo works on grayskull e150 and e75.

## Setup
Before building this project. Setup the enviroment variables in `setup_env.sh` that Metalium needs, e.g.:

```bash
export ARCH_NAME=grayskull                                                                   
export TT_METAL_HOME=/path/to/your/root/of/tt-metal/
export TT_METAL_ENV=dev
```

`ln -s $(pwd)/vecadd_kernels $TT_METAL_HOME/ttvecadd_kernels` in `setup_env.sh` will also create a link in the root of TT_METAL_HOME (this is needed at this time because tt-metal looks in its root dir for kernels).

## Build
```bash
cd build
cmake ..
make
```

## Run
```bash 
$ ./ttvecadd 
                  Metal | INFO     | Initializing device 0
                 Device | INFO     | Opening user mode device driver
2024-04-07 19:02:21.965 | INFO     | SiliconDriver   - Detected 1 PCI device : {0}
                  Metal | INFO     | AI CLK for device 0 is:   1202 MHz
                  Verif | INFO     | Created a random vector of size 32768
                  Verif | INFO     | Created a random vector of size 32768
Kernel execution finished
Partial results: (note we are running under BFP16. It's going to be less accurate)
  2.32812 + 2.26562 = 4.59375
  4.25 + 3.46875 = 7.71875
  0.0114746 + 7.4375 = 7.4375
  4.09375 + 6.9375 = 11.0625
  3.89062 + 7.03125 = 10.9375
  6.84375 + 6.0625 = 12.9375
  0.0195312 + 3.71875 = 3.73438
  9.4375 + 7.6875 = 17.125
  6.8125 + 4.96875 = 11.8125
  4.25 + 5.46875 = 9.75
                  Metal | INFO     | Closing device 0
                 Device | INFO     | Closing user mode device drivers
```
