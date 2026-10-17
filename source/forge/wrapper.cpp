#include "forge/wrapper.h"

#include <nn/os.h>

extern "C" void nnosInitializeMutex(nn::os::MutexType* mutex, bool recursive, int lock_level);

namespace nn::os {
bool TryWaitLightEvent(LightEventType* event) noexcept;
}

static_assert(sizeof(nn::os::MutexType) == FORGE_NNOS_MUTEX_SIZE);

extern "C" void forge_nnosInitializeLightEvent(void* event, bool initially_signaled, int clear_mode)
{
    nn::os::InitializeLightEvent(
        static_cast<nn::os::LightEventType*>(event),
        initially_signaled,
        static_cast<nn::os::EventClearMode>(clear_mode));
}

extern "C" void forge_nnosFinalizeLightEvent(void* event)
{
    nn::os::FinalizeLightEvent(static_cast<nn::os::LightEventType*>(event));
}

extern "C" void forge_nnosSignalLightEvent(void* event)
{
    nn::os::SignalLightEvent(static_cast<nn::os::LightEventType*>(event));
}

extern "C" void forge_nnosWaitLightEvent(void* event)
{
    nn::os::WaitLightEvent(static_cast<nn::os::LightEventType*>(event));
}

extern "C" bool forge_nnosTryWaitLightEvent(void* event)
{
    return nn::os::TryWaitLightEvent(static_cast<nn::os::LightEventType*>(event));
}

extern "C" bool forge_nnosTimedWaitLightEvent(void* event, u64 timeout)
{
    return nn::os::TimedWaitLightEvent(
        static_cast<nn::os::LightEventType*>(event),
        nn::TimeSpan::FromNanoSeconds(timeout));
}

extern "C" void forge_nnosClearLightEvent(void* event)
{
    nn::os::ClearLightEvent(static_cast<nn::os::LightEventType*>(event));
}

extern "C" void forge_nnosInitializeMutex(void* mutex, bool recursive)
{
    nnosInitializeMutex(static_cast<nn::os::MutexType*>(mutex), recursive, 0);
}

extern "C" void forge_nnosLockMutex(void* mutex)
{
    nn::os::LockMutex(static_cast<nn::os::MutexType*>(mutex));
}

extern "C" void forge_nnosUnlockMutex(void* mutex)
{
    nn::os::UnlockMutex(static_cast<nn::os::MutexType*>(mutex));
}

extern "C" bool forge_nnosIsMutexLockedByCurrentThread(const void* mutex)
{
    return nn::os::IsMutexLockedByCurrentThread(static_cast<const nn::os::MutexType*>(mutex));
}
