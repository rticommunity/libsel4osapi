# libsel4osapi

A library for the provisioning of basic system services on seL4.

# Table of Contents

* [Introduction](#introduction)
* [Installation](#installation)
* [Building](#building)
* [Dependencies](#dependencies)
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

Refer to [support/build/Kconfig](support/build/Kconfig) for an example
of how to include `libsel4osapi` in your build configuration.


In order to trigger the compilation of `libsel4osapi` and include the library
in your final system image, you must modify you application's `Kbuild` file
so that it includes `sel4osapi` in its list of dependencies, e.g.:

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
    hardware - ie. the SabreListe i.MX6 board).

The library was developed using the `1.0.x-compatible` branch of the seL4 kernel
and the seL4 user-space libraries.

Refer to (Newer kernel support)(#newer-kernel-support) for more information on
how to port `libsel4osapi` to more recent versions of the seL4 kernel.

The following is an example of the required `repo` manifest entries to support
`libsel4osapi`:

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

