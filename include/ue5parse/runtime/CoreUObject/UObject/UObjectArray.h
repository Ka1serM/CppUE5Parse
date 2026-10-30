#pragma once

#include <mutex>

#include "ue5parse/runtime/CoreUObject/UObject/ObjectMacros.h"

namespace ue5parse::runtime {

class UObject;
class UObjectBase;

// One slot of the global object array: the object, and a serial number that weak pointers compare against, so that a
// slot reused by a new object does not resolve an old weak pointer.
struct FUObjectItem {
    UObjectBase* Object = nullptr;
    int32 SerialNumber = 0;
};

// GUObjectArray: every live object by index. Indices of destroyed objects are reused, last freed first.
class FUObjectArray {
public:
    int32 AllocateIndex(UObjectBase* Object);
    void FreeIndex(int32 Index);
    int32 GetObjectArrayNum() const { return static_cast<int32>(NumItems); }
    FUObjectItem* IndexToObject(int32 Index);
    // The serial number of a slot, given one when it has none: weak pointers call it as they are made.
    int32 AllocateSerialNumber(int32 Index);
    std::recursive_mutex& GetMutex() { return Mutex; }

private:
    static constexpr int32 ChunkSize = 64 * 1024;
    static constexpr int32 MaxChunks = 1024;

    std::recursive_mutex Mutex;
    FUObjectItem* Chunks[MaxChunks] = {};
    int32 NumItems = 0;
    TArray<int32> FreeIndices;
    int32 NextSerialNumber = 1;
};

extern FUObjectArray GUObjectArray;

}
