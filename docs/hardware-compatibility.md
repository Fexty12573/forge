# Switch hardware compatibility

These changes address startup and plugin-loading failures in the AArch32 MHGU
process. They are based on Forge v1.5.1, commit
`b23e114a23f41c9a8e2fd34b2f4df2a2102d0ee1`.

## Build and outputs

Use devkitARM GCC 16.1.0, GNU Make and Python 3:

```bash
git submodule update --init --recursive
bash build-hardware.sh
```

Set `PYTHON` to a Python 3 executable if it is not available as `python3`.
The script sets devkitARM paths only for its child processes and accepts normal
Make arguments. Outputs are `subsdk.elf`, `subsdk.nso` and `main.npdm`.
Deploy `subsdk.nso` as `exefs/subsdk9`, alongside `exefs/main.npdm`.

The compiler version matters because the vendored PIC container support sources
must match the installed libstdc++ headers. Existing third-party repositories
remain submodules at their original gitlink revisions. The build applies the
tracked patches to their working trees before compiling. An already-applied
patch is accepted; conflicting local changes stop the build instead of being
overwritten.

| Dependency patch | Required change |
| --- | --- |
| `patches/libnx-injected.patch` | Use the minimal ARM32 Makefile recursively, enable injection flags, use the game's process handle, and use SDK mutexes on SDK threads |
| `patches/iniparser-injected.patch` | Send errors to Forge instead of importing newlib stderr/TLS into the musl process |
| `patches/nnheaders-nvn-alignment.patch` | Give the NVN CommandBuffer its required eight-byte alignment |

## Changes and evidence

| Observed failure | Source change | Verification |
| --- | --- | --- |
| SDK relocation stores target RX/R pages | Exclude default CRT objects; compile the required C++ support subset as PIC; link with `-z text` | The ELF-to-NSO tool checks both inputs and outputs for text relocations and non-writable relocation targets |
| Early C++ allocation enters the game's uninitialized engine heap | Bind matching new/delete implementations locally, backed by C allocation functions | ARM regression checks execute startup allocation, constructors, matching frees and aligned arrays |
| Failed module loads are later used for symbol lookup | Remove failed modules before resolving callbacks; retain registration data while it is registered | ARM loader cases cover individual/all failures, duplicate input and a negative control |
| Hooks and patches write protected game code | Use writable aliases and synchronize modified code; handle cross-page writes and ARM/Thumb stubs | Protected-page write and hook cases execute the compiled code |
| NVN CommandBuffer layout and alignment differ from the SDK | Use 32-bit enums and an aligned CommandBuffer at offset 0x18 | ARM NVN regression checks execute the relevant SDK stores and verify control-memory setup |
| Font calculations use the wrong floating-point argument/return registers | Bridge 13 SDK math functions through `--wrap` and AAPCS-VFP declarations | ARM cases check the 13 bridges and font glyph dimensions |

The disk log uses the SDK's four-byte ARM32 FileHandle and is written to
`sd:/forge-loader.log`. Log setup failures do not prevent game startup.

`libs/libstdcxx-pic/` contains the small GCC support subset, with upstream source
hashes, GPLv3 text and the GCC Runtime Library Exception. The normal prebuilt
libstdc++/libm archives are not linked into the injected module.

## Version and NPDM metadata

The loader reports its actual Git-derived version. A source archive or fork
without release tags falls back to this tree's upstream base, 1.5.1. There is
no command-line override reporting 1.4.1 for v1.5.1-based code.

`metadata/main-hardware.npdm` is an intentional tracked build input. The generated
root `main.npdm` remains ignored. Before producing it, the build verifies the
template hash and the required permissions in both ACI0 and ACID:

| SVC | Purpose |
| --- | --- |
| 0x4B / 0x4C | Create and control code memory |
| 0x5F | Flush process data cache |
| 0x74 / 0x75 | Map and unmap process memory |

The complete tested title, service and capability fields are preserved. See
[`metadata/README.md`](../metadata/README.md) for provenance and the template hash.

## NRO plugin requirements

The loader fixes do not rewrite a plugin's relocation tables. NROs still need
ARM32 relocation types supported by MHGU's loader, with relocation writes in
writable data/BSS. Plugins should use SDK bindings matching the current public
Hook layout and declare a compatible Forge version.

For Rust/GCC builds, the configuration validated with the local plugins retains
`"relocation-model": "pic"` and adds this target JSON entry:

```json
"pre-link-args": {
    "gcc": [
        "-nostartfiles",
        "-Wl,-z,text"
    ]
}
```

`-nostartfiles` excludes default GCC startup objects. `-Wl,-z,text` rejects
read-only dynamic relocations during linking; it does not repair them. Other
build configurations can produce the same valid result. The Rust template
change belongs in the separate forge-rs repository.

## Validation scope

Every build validates ELF and NSO relocation destinations automatically. The
local ARM regression suite additionally uses independently supplied game SDK
images and font fixtures; those fixtures are not included in this repository.
The merge was checked against those existing cases, including failure controls.

The 2026-10-18 fork merge passed a forced full rebuild with GCC 16.1.0 and the
following local checks:

- No text relocations, read-only relocation destinations, external C++
  allocator imports or unresolved strong imports in the new loader.
- Four loader scenarios declaring Forge 1.5.1, plus a reproduced failure
  control; protected-page writes and ARM/Thumb hooks passed.
- Startup allocation/free cases, NVN CommandBuffer offset 0x18, all 13 math
  bridges and five font glyph cases passed.
- Strong imports from the seven local rebuilt NROs resolved against the game
  and the new loader. This checks symbol availability, not every plugin behavior.
- The NPDM output matched its input template byte for byte.

The earlier fixed build was reported to run normally in Ryujinx. A fresh build
from this fork still requires an in-game hardware test. Passing these code-level
checks does not establish compatibility with every third-party NRO.
