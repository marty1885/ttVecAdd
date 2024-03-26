# ttVecAdd

A simple vector-addition on Tenstorrent devices using tt-Metalium.

## How to use


Before building this project. Setup the enviroment variables that Metalium needs

```bash
export ARCH_NAME=grayskull                                                                   
export TT_METAL_HOME=/path/to/your/root/of/tt-metal/
export TT_METAL_ENV=dev
```

Then build the project

```bash
cd ttPEAK
# HACK: Metalium only searches kernels under it's directories
ln -s vecadd_kernels $TT_METAL_HOME/vecadd_kernels

mkdir build
cd build
cmake ..
make
```

To run

```
➜ ./ttvecadd 
                  Metal | INFO     | Initializing device 0
                 Device | INFO     | Opening user mode device driver
2024-03-26 10:24:40.877 | INFO     | SiliconDriver   - Detected 1 PCI device : [0]
                  Metal | INFO     | AI CLK for device 0 is:   1300 MHz
                  Metal | INFO     | DPRINT enabled on device 0, worker core (x=0,y=0) (physical (x=1,y=1)).
                  Metal | INFO     | DPRINT Server attached device 0
                  Verif | INFO     | Created a random vector of size 512
                  Verif | INFO     | Created a random vector of size 512
Kernel execution finished
Partial results: (note we are running under BFP16. It's going to be less accurate)
  17.625 + 8.1875 = 25.875
  6.34375 + 17.125 = 23.5
  5.0625 + 4.3125 = 9.375
  17.375 + 4.53125 = 21.875
  3.25 + 10.5625 = 13.8125
  17 + 3.25 = 20.25
  7.40625 + 6.8125 = 14.25
  17.875 + 0.369141 = 18.25
  15.75 + 10.75 = 26.5
  15.1875 + 15.8125 = 31
                  Metal | INFO     | Closing device 0
                  Metal | INFO     | DPRINT Server dettached device 0
                 Device | INFO     | Closing user mode device drivers
```