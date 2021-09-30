#include <stdint.h>

#include "device.h"
#include "common.h"
#include "helpers.h"

static device_t **uiDevices;
static uint8_t deviceCount;
static unsigned long deviceTimeout[16] = {0};
static unsigned long nextDeviceTimeout = 0;

static connectionState_e lastConnectionState = disconnected;

void initDevices(device_t **devices, uint8_t count)
{
    uiDevices = devices;
    deviceCount = count;
    for(size_t i=0 ; i<count ; i++) {
        if (uiDevices[i]->initialize) {
            (uiDevices[i]->initialize)();
        }
    }
}

void startDevices()
{
    unsigned long now = millis();
    for(size_t i=0 ; i<deviceCount ; i++)
    {
        if (uiDevices[i]->start)
        {
            int delay = (uiDevices[i]->start)();
            deviceTimeout[i] = delay == DURATION_NEVER ? 0xFFFFFFFF : now + delay;
            nextDeviceTimeout = min(nextDeviceTimeout, deviceTimeout[i]);
        }
    }
}

void handleDevices(unsigned long now, bool eventFired, std::function<void ()> setSpam)
{
    for(size_t i=0 ; i<deviceCount ; i++)
    {
        if ((eventFired || lastConnectionState != connectionState) && uiDevices[i]->event)
        {
            int delay = (uiDevices[i]->event)(setSpam);
            if (delay != DURATION_IGNORE)
            {
                deviceTimeout[i] = delay == DURATION_NEVER ? 0xFFFFFFFF : now + delay;
                nextDeviceTimeout = min(nextDeviceTimeout, deviceTimeout[i]);
            }
        }
    }
    lastConnectionState = connectionState;
    for(size_t i=0 ; i<deviceCount ; i++)
    {
        if (now > deviceTimeout[i] && uiDevices[i]->timeout)
        {
            int delay = (uiDevices[i]->timeout)(setSpam);
            deviceTimeout[i] = delay == DURATION_NEVER ? 0xFFFFFFFF : now + delay;
            nextDeviceTimeout = min(nextDeviceTimeout, deviceTimeout[i]);
        }
    }
}
