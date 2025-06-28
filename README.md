# Linux Device Drivers for the Beaglebone Black REV A5

## Set up the host machine
- Installing required packages.
```
sudo apt-get update
sudo apt-get install build-essential lzop u-boot-tools net-tools bison flex libssl-dev libncurses5-dev libncursesw5-dev unzip chrpath xz-utils minicom
```
- Installing the cross compiler
```
sudo apt update
sudo apt install gcc-arm-linux-gnueabihf
```
- Clone the kernel source (v4.19 for BBB default debian image)
```
git clone -b v4.19.94-ti-r74 git@github.com:beagleboard/linux.git linux_4.19
```
- Kernel compilations steps:
```
# Remove all the temporary folder
make ARCH=arm distclean

# Creates a .config file
make ARCH=arm bb.org_defconfig

# Run menuconfig (optional)
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- menuconfig

# Compile uImage and dtbs
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- uImage dtbs LOADADDR=0x80008000 -j4

# Compile the in-tree loadable kernel module
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules -j4

# Install all generated .ko files
sudo make ARCH=arm modules_install
```

