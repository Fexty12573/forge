
#include <nn/types.h>

namespace nn::diag {

void GetBacktrace(uintptr_t* buffer, int max) noexcept;
uintptr_t GetSymbolName(char* outBuffer, size_t bufferSize, uintptr_t addr) noexcept;

}
