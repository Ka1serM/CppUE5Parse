#pragma once

#include <mutex>

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectMacros.h"

namespace ue5parse::runtime {

class UObject;
class UObjectBase;

struct FUObjectItem {
    UObjectBase* Object = nullptr;
    int32 SerialNumber = 0;
};

class FUObjectArray {
public:
    int32 AllocateIndex(UObjectBase* Object);
    void FreeIndex(int32 Index);
    int32 GetObjectArrayNum() const { return static_cast<int32>(NumItems); }
    FUObjectItem* IndexToObject(int32 Index);
    int32 AllocateSerialNumber(int32 Index);

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
