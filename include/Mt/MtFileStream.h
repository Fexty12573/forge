#pragma once
#ifdef __cplusplus

#include "Mt/MtFile.h"
#include "Mt/MtStream.h"

class MtFileStream : public MtStream {
public:
    MtFile* file;

    virtual void open(MtFile* file) = 0;
};

#endif
