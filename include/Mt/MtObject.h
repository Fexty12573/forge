#pragma once
#ifdef __cplusplus

#include "Mt/MtPropertyList.h"
#include "Mt/MtPtr.h"
#include "switch/types.h"

class MtDti;

class MtObject {
public:
    virtual ~MtObject() = 0;
    virtual void destroy(bool deallocate = true) = 0;
    virtual void createUi() const = 0;
    virtual bool isEnableInstance() const = 0;
    virtual void createProperty(MtPropertyList* props) = 0;
    virtual MtDti* getDti() const = 0;
};

#endif
