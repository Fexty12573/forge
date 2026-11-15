# MHGU Forge

An environment for linking, runtime hooking and code patching in Monster Hunter Generations Ultimate.

## Installing (Ryujinx)

1. Download the latest release from the [releases page](https://github.com/Fexty12573/forge/releases)
2. Open Ryujinx, right click MHGU and select "Open Mods Directory"
3. Extract the contents of the downloaded archive into that folder
4. In Ryujinx, right click MHGU and select "Create Custom Configuration" (or "Edit Custom Configuration" if you already did so before)
5. Go to the CPU section and **disable** the "PPTC (Profiled Persistent Translation Cache)" option
6. Click "Save".
7. Right click MHGU and select "Cache Management > Purge PPTC Cache" and "Cache Management > Purge Shader Cache"
8. Restart Ryujinx

### Keyboard Input

To make keyboard input work, you need to enable "Direct Keyboard Access" in the Ryujinx Input Settings.

## Building

Build with devkitARM GCC 16.1.0, GNU Make and Python 3. The small PIC C++ support
subset in `libs/libstdcxx-pic/` must match the compiler's container headers.

```bash
git submodule update --init --recursive
bash build-hardware.sh
```

Set `PYTHON` to your Python executable if `python3` is unavailable. The script
sets devkitARM paths only for its own process. The build checks ELF/NSO
relocations and outputs `subsdk.nso` and `main.npdm` here.

The build applies the small injection-specific patches under `patches/` to the
pinned submodule working trees. Their Git revisions remain unchanged; the patch
files are part of this repository and are safe to apply repeatedly.

For the current installation, `subsdk.nso` is deployed as `exefs/subsdk9`.
The NPDM is copied from the tested template under `metadata/` after validating
its hash and required SVC permissions. Do not regenerate it from the old JSON
unless you intend to change the game metadata.

The build derives the loader version from Git tags and uses 1.5.1 for source
archives or forks without release tags. It does not override the version with
an older hardware-build label.

See [hardware compatibility](docs/hardware-compatibility.md) for the source
changes, NPDM permissions, plugin requirements and validation scope.

## Credits

* [devkitPro](https://devkitpro.org) for a development environment
* [skyline-dev/skyline](https://github.com/skyline-dev/skyline) for the basis of the module
* [RicBent/codedx](https://github.com/RicBent/codedx) for building an NSO
* [xerpi/libnx](https://github.com/xerpi/libnx) for an AArch32 userland library
* [Magic1-Mods/And32InlineHook](https://github.com/Magic1-Mods/And32InlineHook) for AArch32 hooks
