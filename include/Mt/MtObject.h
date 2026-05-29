#pragma once
#ifdef __cplusplus

#include "Mt/MtPtr.h"
#include "switch/types.h"

class MtDti;
class MtPropertyList;

class MtObject {
public:
    virtual ~MtObject() = 0;
    virtual void createUi() const = 0;
    virtual bool isEnableInstance() const = 0;
    virtual void createProperty(MtPropertyList* props) = 0;
    virtual MtDti* getDti() const = 0;
};

#endif
