"""Validate and copy the existing tested MHGU NPDM without changing metadata."""
from pathlib import Path
import hashlib
import struct

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = "4e449887b88c980670b6407bf6d65a22f20d67752f8d4fb53fb78a6a9b3db36d"


def prepare():
    image = (ROOT/"metadata/main-hardware.npdm").read_bytes()
    if hashlib.sha256(image).hexdigest() != EXPECTED:
        raise ValueError("The tested NPDM template changed")
    for name, header, fields in [("ACI0",0x70,0x30),("ACID",0x78,0x230)]:
        start = struct.unpack_from("<I",image,header)[0]
        offset,size = struct.unpack_from("<II",image,start+fields)
        allowed = set()
        for pos in range(start+offset,start+offset+size,4):
            cap = struct.unpack_from("<I",image,pos)[0]
            if cap & 31 == 15:
                allowed.update((cap>>29)*24+bit for bit in range(24) if cap & (1<<(bit+5)))
        if not {0x4b,0x4c,0x5f,0x74,0x75} <= allowed:
            raise ValueError(name+" lacks the required code-memory/cache/mapping calls")
    (ROOT/"main.npdm").write_bytes(image)
    print("main.npdm matches the tested metadata: "+EXPECTED)


if __name__=="__main__": prepare()
