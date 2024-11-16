# Escape from the Facility

Based on the **decompilation** of ***The Legend of Zelda: Ocarina of Time*** by [ZeldaRET](https://discord.zelda64.dev).

## Installation

Use WSL on Windows or native Linux. Instructions below:

* [Windows](#Windows), with and without WSL
* [macOS](docs/BUILDING_MACOS.md)
* [Linux](#Linux-Native-or-under-WSL--VM), natively or using WSL / VM
* [Docker](docs/BUILDING_DOCKER.md)

### Windows

For Windows 10 or 11, install WSL and a distribution by following this
[WSL Installation Guide](https://docs.microsoft.com/en-us/windows/wsl/install).
Ubuntu 20.04 works best as the Linux distribution for this repository.

For older versions of Windows, install a Linux VM or refer to either [Cygwin](docs/BUILDING_CYGWIN.md) or [Docker](docs/BUILDING_DOCKER.md) instructions.


### Linux (Native or under WSL / VM)

#### 1. Install build dependencies

The build process has the following package requirements:

* git
* build-essential
* binutils-mips-linux-gnu
* python3
* libpng-dev
* gcc-mips-linux-gnu

Under Debian / Ubuntu, you can install them with the following commands:

```bash
sudo apt-get update
sudo apt-get install git build-essential binutils-mips-linux-gnu python3 libpng-dev gcc-mips-linux-gnu
```

#### 2. Clone the repository

**N.B.** If using WSL, clone into WSL's Linux filesystem using Linux's `git`.
Cloning into the Windows filesystem will result in much slower read/write speeds, and often causes issues when Windows copies the files with the wrong line endings, which the compiler IDO cannot handle correctly.

Clone `https://github.com/CDi-Fails/escape_room_public.git` where you wish to have the project, with a command such as:

```bash
git clone https://github.com/CDi-Fails/escape_room_public.git
```

This will copy the GitHub repository contents into a new folder in the current directory called `escape_room_public`. Change into this directory before doing anything else:

```bash
cd escape_room_public
```

#### 3. Prepare a base ROM

Copy over your copy of the Master Quest (Debug) ROM inside the root of this new project directory.
Rename the file to "baserom_original.z64", "baserom_original.n64" or "baserom_original.v64", depending on the original extension.

#### 4. Setup the ROM and build process

Setup and extract everything from your ROM with the following command:

```bash
make setup
```

This will generate a new ROM called "baserom.z64" that will have the overdump removed and the header patched.
It will also extract the individual assets from the ROM.

#### 5. Build the ROM

Run make to build the ROM.
Make sure your path to the project is not too long, otherwise this process may error.

```bash
make
```

If all goes well, a new ROM called "zelda_ocarina_mq_debug.z64" should be built.

To build with debug features, pass ENABLE_DEBUG_FEATURES=1 like so:

```bash
make ENABLE_DEBUG_FEATURES=1
```

**NOTE:** to speed up the build, you can either:

* pass `-jN` to `make setup` and `make`, where N is the number of threads to use in the build. The generally-accepted wisdom is to use the number of virtual cores your computer has.
* pass `-j` to `make setup` and `make`, to use as many threads as possible, but beware that this can use too much memory on lower-end systems.
