# MHGU Forge

An environment for linking, runtime hooking and code patching in Monster Hunter Generations Ultimate.

## Installing
Download the latest release from the [releases page](https://github.com/Fexty12573/forge/releases) and extract the contents of the zip archive to your mod directory.

## Building

Be sure to check out submodules first:

```bash
git submodule update --init --recursive
```

To build the module you'll need [devkitPro](https://devkitpro.org/wiki/Getting_Started).

Install devkitARM using pacman:

```sh
sudo (dkp-)pacman -Sy
sudo (dkp-)pacman -S devkitARM
```

Use `dkp-pacman` on macOS and Debian-based distros and `pacman` otherwise.

Also install the `lz4` package:

```sh
# On Debian-based distros
sudo apt install liblz4-dev

# On Arch-based distros
sudo pacman -S lz4

# On macOS with Homebrew
brew install lz4
```

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
