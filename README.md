# Pokoyo imager

Pokoyo imager is an analogue of Rufus written for Linux.
It repeats the original design and functionality


## Installation
### Downloads packages
``` bash
sudo apt update && sudo apt install -y \
build-essential \
qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
rsync syslinux syslinux-common \
dosfstools ntfs-3g exfatprogs \
parted util-linux e2fsprogs
```

### Compile
```bash
git clone https://github.com/Danil1234-cmd/pokoyo_imager.git
cd pokoyo_imager
mkdir build
cd build
cmake ..
make
sudo ./pokoyo_imager
```

## Usage

Select the desired disk and image, as well as other options for your device. Use 
``` 
Quick format = true 
``` 
for standard boot disk creation, or 
``` 
Quick format = false
``` 
for a pre-test that can take up to several hours

## !!! Warning !!!
This is the initial stage of project development, which is being tested. Therefore, the software may not work properly on some devices, as well as damage the removable drives to which the recording is made. Please take this into account when using the software

## Contributing

Pull requests are welcome. For major changes, please open an issue first
to discuss what you would like to change.

Please make sure to update tests as appropriate.
