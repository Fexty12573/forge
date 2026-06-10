#include "forge/wrapper.h"

#include <nn/os.h>

namespace nn::os {
bool TryWaitLightEvent(LightEventType* event) noexcept;
}

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
