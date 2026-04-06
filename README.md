# MHGU Forge
An environment for linking, runtime hooking and code patching in Monster Hunter Generations Ultimate.

## Building
To build the module you'll need:

* [devkitPro](https://devkitpro.org/wiki/Getting_Started)
* [Python 3](https://www.python.org/downloads/)

Install devkitARM using pacman:

```sh
sudo (dkp-)pacman -Sy
sudo (dkp-)pacman -S devkitARM
```

Use `dkp-pacman` on macOS and Debian-based distros and `pacman` otherwise.

Then create a virtual environment and install python packages:

```sh
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r requirements.txt
```

Make sure to activate the python virtual environment again before trying to build.

Then you can build the module:

```sh
make
```

You'll find `subsdk.nso` and `main.npdm` in the project directory. Move `subsdk.nso` to `subsdk1` (for example) and `main.npdm` into an `exefs` folder in a mod directory.

## Credits
* [devkitPro](https://devkitpro.org) for a development environment
* [skyline-dev/skyline](https://github.com/skyline-dev/skyline) for the basis of the module
* [RicBent/codedx](https://github.com/RicBent/codedx) for building an NSO
* [xerpi/libnx](https://github.com/xerpi/libnx) for an AArch32 userland library
* [Magic1-Mods/And32InlineHook](https://github.com/Magic1-Mods/And32InlineHook) for AArch32 hooks
