# MHGU NPDM template

`main-hardware.npdm` is a build input preserving the complete process metadata
from the tested MHGU setup. Its SHA-256 is
`4e449887b88c980670b6407bf6d65a22f20d67752f8d4fb53fb78a6a9b3db36d`.

`tools/prepare_npdm.py` validates that hash and checks both ACI0 and ACID for
SVCs 0x4B, 0x4C, 0x5F, 0x74 and 0x75 before copying the template to the ignored
root output `main.npdm`. This keeps the tested title, service and capability
fields together. `config.json` also contains the required permissions for
developers who regenerate metadata intentionally.

`source-inputs.json` records the upstream commits and archive hashes used to
assemble the hardware-fix sources, based on Forge v1.5.1 commit `b23e114`.
Existing dependencies remain Git submodules at their upstream-pinned commits;
the build applies the tracked injection fixes from `patches/` to those checkouts.
The additional GCC PIC sources have their own origins and licenses under
`libs/libstdcxx-pic/`.
