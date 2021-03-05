#include <errno.h>
#include <string>
#include <cstdint>

int changeIntegerValue(const int& requestedValue, int& currentValue)
{
    if (requestedValue >= 50)
    {
        return -EINVAL;
    }
    currentValue = requestedValue;
    return 1;
}


std::string unlockDoor(const std::string& keyVal){
    if (keyVal != "open sesame"){
        return "DoorLocked";
    }
    return "DoorUnlocked";
}