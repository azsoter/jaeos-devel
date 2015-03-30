This is the Zynq/ZebBoard port of the JaeOS RTOS.
It now contains code to run on both CPU cores in the Zynq (SMP).

SMP features are only compiled in if JaeOS is configured to do SMP, otherwise things revert to the single processor version.

To initialize the second CPU core initialization code is included in the file init_cpu1.S. 
You only need it for SMP operation and it should not be used on a single processor set up.

The file init_cpu1.S does contain lines of code derived from Xilinx's CPU initialization code which was published by Xilinx Inc. under a license similar to the MIT
license used for JaeOS, but with the restriction that it can only be used on or in conjuction with Xilinx products.

See the detailed copyright notice inside init_cpu1.S for details.

If you are porting JaeOS to a non-Xilinx multicore ARM product you cannot use the code in init_cpu1.S and need to find a replacement (probably from ARM).

