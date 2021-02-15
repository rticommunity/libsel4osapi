# libsel4osapi

A library for the provisioning of basic system services on seL4.

# IMPORTANT BRANCH INFO
This branch (`micro2-sabre`) contains the libsel4osapi with the following features:
* Built on top of seL4 version 11.0.0-dev 
* Tested/target device: iMX6 (SabreLite)
* Use the companion libsel4osapi-workspace branch `micro2-sabre` to use this
  version.


-----------------------------------------------------
# Table of Contents

* [Introduction](#introduction)
* [Installation](#installation)
* [Building](#building)
* [Dependencies](#dependencies)
* [Issues with seL4 5.2.x](#issues52x)
* [License](#license)
* [Acknowledgement](#acknowledgment)

# Introduction

`libsel4osapi` provides a simple layer of basic system-level services
which can be used for the development of more portable seL4 applications,
thanks to higher-level abstractions, such as:
  - POSIX-like processes and threads.
  - Mutexes and semaphores.
  - Global system timer with support for user timeouts.
  - UDP/IP networking via socket-like API
  - read/write API for serial devices

The library must be handed resources received from the seL4 kernel,
and it will take care of bootstrapping the system with the help of other
sel4 user-space libraries.

Refer to the library's [implementation design document](docs/design.md) for
more information on the functionalities of `libsel4osapi` and how they are
provisioned.

# Installation

The easiest way to integrate `libsel4osapi` in your seL4 project is by
adding it to your `repo` manifest file:

```xml

<remote name="rti" fetch="https://github.com/rticommunity" />
<project name="libsel4osapi.git" remote="rti" path="libs/libsel4osapi">
  <linkfile src="liblwip/include"    dest="libs/liblwip/include"/>
  <linkfile src="liblwip/Kbuild"     dest="libs/liblwip/Kbuild"/>
  <linkfile src="liblwip/Kconfig"    dest="libs/liblwip/Kconfig"/>
  <linkfile src="liblwip/Makefile"   dest="libs/liblwip/Makefile"/>
</project>

```

This entry will clone the `libsel4osapi` repository in your project and make
it available as `libs/libsel4osapi` (NOTE: you might want to point the path to
the directory where you already store other seL4 user-space libraries, if
different than `libs/`).

Make sure to extract `lwip` as `libs/liblwip/lwip-1.4.1` (or at the very least
to build it using the provided build files), as shown in [Dependencies](#dependencies).

# Building

`libsel4osapi` can be built using seL4's `kbuild`-based build system.

You must include `libsel4osapi`'s `Kconfig` in your project's top-level
`Kconfig`, e.g.:

```

menu "seL4 Libraries"
  # Other libraries (e.g. libsel4, libmuslc, libsel4utils, etc.)

  source "libs/libsel4osapi/Kconfig"
endmenu

```

In order to trigger the compilation of `libsel4osapi` and include the library
in your final system image, you must modify you application's `Kbuild` file
so that it includes `libsel4osapi` in its list of dependencies, e.g.:

```makefile

apps-$(CONFIG_APP_ROOT_TASK) += root_task

# list of libraries the app needs to build
root_task-y = \
            # other user-space libraries
            libsel4osapi

```

# Dependencies

`libsel4osapi` relies on other sel4 user-space libraries to implement its
services.

The following libraries must be available and linked into your system image:
  - `libsel4`
  - `libmuslc`
  - `libsel4muslcsys`
  - `libsel4simple` (and one of `libsel4simple-stable` or `libsel4simple-default`)
  - `libsel4vka`
  - `libsel4allocman`
  - `libsel4utils`
  - `libsel4vspace`
  - `libplatsupport`
  - `libsel4platsupport`
  - `libcpio`
  - `libelf`
  - `libutils`
  - `liblwip` (if you wish to enable the IP/UDP networking support)
  - `libethdrivers` (if you actually want to use the network stack on some physical
    hardware - ie. the SabreLite i.MX6 board).

The library was developed using the `1.0.x-compatible` branch of the seL4 kernel
and the seL4 user-space libraries, then updated to seL4 branch '5.2.x-compatible'.

## Working with seL4 1.0.x branch:
If you are using the seL4 1.0.x branch, use the '1.0.x-compatible' branch of libsel4osapi.

The following is an example of the required `repo` manifest entries to support
`libsel4osapi` for seL4 1.0.x-compatible:

```xml
<remote name="seL4" fetch="https://github.com/sel4"/>
<remote name="savannah" fetch="git://git.savannah.nongnu.org"/>

<default revision="1.0.x-compatible"
         remote="seL4"/>

<!-- seL4 user-space libraries from Data61-->
<project name="seL4.git"                    path="kernel"
         revision="refs/tags/1.0.4">
    <linkfile src="libsel4"                 dest="libs/libsel4"/>
</project>
<project name="musllibc.git"                path="libs/libmuslc"/>
<project name="libsel4muslcsys.git"         path="libs/libsel4muslcsys"/>
<project name="libsel4platsupport.git"      path="libs/libsel4platsupport"/>
<project name="libsel4allocman.git"         path="libs/libsel4allocman"/>
<project name="libsel4vka.git"              path="libs/libsel4vka"/>
<project name="libsel4vspace.git"           path="libs/libsel4vspace"/>
<project name="libsel4utils.git"            path="libs/libsel4utils"/>
<project name="libcpio.git"                 path="libs/libcpio"/>
<project name="libelf.git"                  path="libs/libelf"/>
<project name="libsel4simple.git"           path="libs/libsel4simple"/>
<project name="libsel4simple-default.git"   path="libs/libsel4simple-default"/>
<project name="libsel4simple-stable.git"    path="libs/libsel4simple-stable"/>
<project name="libplatsupport.git"          path="libs/libplatsupport"/>
<project name="libutils.git"                path="libs/libutils"/>
<project name="libethdrivers"               path="libs/libethdrivers"
         revision="master"/>

<!-- lwip 1.4.1 --->
<project name="lwip.git" path="libs/liblwip/lwip-1.4.1"
    remote="savannah" revision="refs/tags/STABLE-1_4_1" />

```

## Working with seL4 5.2.x branch:
If you are using the seL4 5.2.x branch, use the '5.2.x-compatible' branch of libsel4osapi.

The following is an example of the required `repo` manifest entries to support
`libsel4osapi` for seL4 5.2.x-compatible:

```xml
<remote name="seL4" fetch="https://github.com/sel4"/>
<remote name="savannah" fetch="git://git.savannah.nongnu.org"/>

<default revision="5.2.x-compatible"        remote="seL4"/>

<!-- seL4 user-space libraries from Data61-->
<project name="seL4.git"                    path="kernel"
         revision="refs/tags/5.2.0">
    <linkfile src="libsel4"                 dest="libs/libsel4"/>
</project>
<project name="musllibc.git"                path="libs/libmuslc" revision="sel4"/>
<project name="seL4_libs.git" path="projects/seL4_libs">
    <linkfile src="libsel4allocman" dest="libs/libsel4allocman" />
    <linkfile src="libsel4debug" dest="libs/libsel4debug" />
    <linkfile src="libsel4muslcsys" dest="libs/libsel4muslcsys" />
    <linkfile src="libsel4platsupport" dest="libs/libsel4platsupport" />
    <linkfile src="libsel4simple" dest="libs/libsel4simple" />
    <linkfile src="libsel4simple-default" dest="libs/libsel4simple-default" />
    <linkfile src="libsel4test" dest="libs/libsel4test" />
    <linkfile src="libsel4utils" dest="libs/libsel4utils" />
    <linkfile src="libsel4vka" dest="libs/libsel4vka" />
    <linkfile src="libsel4vspace" dest="libs/libsel4vspace" />
    <linkfile src="libsel4sync" dest="libs/libsel4sync" />
</project>

<project name="util_libs.git" path="projects/util_libs">
    <linkfile src="libcpio" dest="libs/libcpio" />
    <linkfile src="libelf"  dest="libs/libelf" />
    <linkfile src="libplatsupport" dest="libs/libplatsupport" />
    <linkfile src="libutils" dest="libs/libutils" />
</project>

<project name="libethdrivers" path="libs/libethdrivers" revision="master"/>
<project name="lwip.git" remote="savannah"
          path="libs/liblwip/lwip-1.4.1" revision="refs/tags/STABLE-1_4_1" />

```




# Known issues with seL4 5.2.x
There are two known issues of libsel4osapi library that might cause problems with seL4 5.2.x-compatible and newer (there might be issues also on older version of seL4, depending on the version used).


## Dynamic memory
In libsel4utils project (version 5.2.x-compatible) the vspace functions uses malloc() to dynamically create a temporary reservation object.
Unfortunately if the system end up in an out of memory condition (where expand_heap() is called) and malloc() is configured to dynamicall map pages through vspace, the system might enter in an infinite loop.
Solution requires a patch to the vspace() to avoid circular dependency with malloc. At the time of the writing of this document, the seL4 community is aware of the problem and agreed to fix it.

## Network Driver
The network driver (libethdrivers) have some conflicts with version 5.2.x-compatible of libplatsupport when using the Sabre Lite i.MX6 board.
In librplasupport, the I/O of the board is mapped in function mux_sys_init() (file src/plat/imx6/mux.c). When the ethernet driver initialzes (function setup_iomux_enet in file libethdrivers/src/plat/imx6/uboot/mx6qsabrelite.c), it attempts to reserve the same I/O space, causing a capability fault. 
The driver need to be updated to use the new capability of the libplatsupport library, but as a temporary solution, you can disable the mapping of the I/O in the setup_iomux_enet() function by commenting out the following line:
```
	    MAP_IF_NULL(io_ops, IMX6_IOMUXC, _mux.iomuxc);
```


# License

`libsel4osapi` is released under license BSD-2-Clause.

See [LICENSE_BSD2.txt](LICENSE_BSD2.txt) for more information.

# Acknowldegment

`libsel4osapi` was developed with support from DARPA, under contracts 
number D15PC00144, and D16PC00101.

## Disclaimer

The views, opinions, and/or findings expressed are those of the author(s) and 
should not be interpreted as representing the official views or policies of the 
Department of Defense or the U.S. Government.

