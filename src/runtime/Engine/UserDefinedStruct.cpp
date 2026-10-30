#include "ue5parse/runtime/Engine/Engine/UserDefinedStruct.h"

#include <cstring>

#include "ue5parse/runtime/Core/Serialization/Archive.h"

// UE's UserDefinedStruct.cpp (UE 5.3).

namespace ue5parse::runtime {

void UUserDefinedStruct::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || HasAnyFlags(RF_ClassDefaultObject) || Status != UDSS_UpToDate) return;
    // The default instance: a value of the struct with the defaults of each member.
    if (DefaultStructInstance == nullptr) {
        const std::size_t Size = static_cast<std::size_t>(GetStructureSize());
        DefaultStructInstance = static_cast<uint8*>(RuntimeMalloc(Size > 0 ? Size : 1, static_cast<std::size_t>(GetMinAlignment())));
        std::memset(DefaultStructInstance, 0, Size);
        UStruct::InitializeStruct(DefaultStructInstance);
    }
    SerializeItem(Ar, DefaultStructInstance);
}

void UUserDefinedStruct::FinishDestroy() {
    if (DefaultStructInstance != nullptr) {
        DestroyStruct(DefaultStructInstance);
        RuntimeFree(DefaultStructInstance);
        DefaultStructInstance = nullptr;
    }
    Super::FinishDestroy();
}

void UUserDefinedStruct::InitializeStruct(void* Dest, int32 ArrayDim) const {
    Super::InitializeStruct(Dest, ArrayDim);
    if (DefaultStructInstance == nullptr) return;
    const int32 Stride = GetStructureSize();
    for (int32 ArrayIndex = 0; ArrayIndex < ArrayDim; ArrayIndex++)
        CopyScriptStruct(static_cast<uint8*>(Dest) + static_cast<std::ptrdiff_t>(Stride) * ArrayIndex, DefaultStructInstance);
}

}
