#include "ue5parse/runtime/Engine/Animation/PropertyAccess.h"

#include <cstring>

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "ue5parse/runtime/CoreUObject/UObject/Stack.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

namespace {

uint32 GetPropertyOffset(const FProperty* InProperty, int32 InArrayIndex = 0) {
    return static_cast<uint32>(InProperty->GetOffset_ForInternal() + InProperty->ElementSize * InArrayIndex);
}

EPropertyAccessSegmentFlags GetUnmodifiedFlags(const FPropertyAccessSegment& Segment) {
    return static_cast<EPropertyAccessSegmentFlags>(Segment.Flags & ~static_cast<uint16>(EPropertyAccessSegmentFlags::ModifierFlags));
}

EPropertyAccessObjectType GetObjectType(EPropertyAccessSegmentFlags Flags) {
    switch (Flags) {
    case EPropertyAccessSegmentFlags::Object:
        return EPropertyAccessObjectType::Object;
    case EPropertyAccessSegmentFlags::WeakObject:
        return EPropertyAccessObjectType::WeakObject;
    case EPropertyAccessSegmentFlags::SoftObject:
        return EPropertyAccessObjectType::SoftObject;
    default:
        return EPropertyAccessObjectType::None;
    }
}

UObject* GetObject(void* Address, EPropertyAccessObjectType ObjectType) {
    switch (ObjectType) {
    case EPropertyAccessObjectType::Object:
        return *static_cast<UObject**>(Address);
    case EPropertyAccessObjectType::WeakObject:
        return static_cast<FWeakObjectPtr*>(Address)->Get();
    case EPropertyAccessObjectType::SoftObject:
        return static_cast<FSoftObjectPtr*>(Address)->Get();
    case EPropertyAccessObjectType::None:
        break;
    }
    UE5PARSE_CHECK(false);
    return nullptr;
}

void CallNativeAccessor(UObject* InObject, UFunction* InFunction, void* OutRetValue) {
    UE5PARSE_CHECK(InFunction->HasAnyFunctionFlags(FUNC_Native) && InFunction->GetReturnProperty() != nullptr && InFunction->NumParms == 1);
    FFrame Stack(InObject, InFunction, nullptr, nullptr, InFunction->ChildProperties);
    InFunction->Invoke(InObject, Stack, OutRetValue);
}

template <class SourceType, class DestinationType>
void CopyAndCastFloatingPointArray(const FArrayProperty* SourceArrayProperty, const void* SourceAddress, const FArrayProperty* DestinationArrayProperty,
                                   void* DestinationAddress) {
    FScriptArrayHelper SourceArrayHelper(SourceArrayProperty, SourceAddress);
    FScriptArrayHelper DestinationArrayHelper(DestinationArrayProperty, DestinationAddress);
    DestinationArrayHelper.Resize(SourceArrayHelper.Num());
    for (int32 i = 0; i < SourceArrayHelper.Num(); ++i)
        *reinterpret_cast<DestinationType*>(DestinationArrayHelper.GetRawPtr(i)) =
            static_cast<DestinationType>(*reinterpret_cast<const SourceType*>(SourceArrayHelper.GetRawPtr(i)));
}

template <class SourceType, class DestinationType>
void CopyAndCastFloatingPointMapValues(const FMapProperty* SourceMapProperty, const void* SourceAddress, const FMapProperty* DestinationMapProperty,
                                       void* DestinationAddress) {
    FScriptMapHelper SourceMapHelper(SourceMapProperty, SourceAddress);
    FScriptMapHelper DestinationMapHelper(DestinationMapProperty, DestinationAddress);
    DestinationMapHelper.EmptyValues();
    for (int32 i = 0; i < SourceMapHelper.Num(); ++i) {
        const DestinationType CastedType = static_cast<DestinationType>(*reinterpret_cast<const SourceType*>(SourceMapHelper.GetValuePtr(i)));
        DestinationMapHelper.AddPair(SourceMapHelper.GetKeyPtr(i), &CastedType);
    }
}

template <class DestinationPropertyType, class SourcePropertyType, class ValueType>
void Convert(const FProperty* InSrcProperty, const void* InSrcAddr, const FProperty* InDestProperty, void* InDestAddr) {
    static_cast<const DestinationPropertyType*>(InDestProperty)
        ->SetPropertyValue(InDestAddr, static_cast<ValueType>(static_cast<const SourcePropertyType*>(InSrcProperty)->GetPropertyValue(InSrcAddr)));
}

}

// Resolves the segments of one path to indirections.
bool FPropertyAccessLibrary::ResolveIndirectionsOnLoad(std::span<const FPropertyAccessSegment> Segments, FPropertyAccessIndirectionChain& Access) {
    TArray<FPropertyAccessIndirection> PathIndirections;
    for (std::size_t SegmentIndex = 0; SegmentIndex < Segments.size(); ++SegmentIndex) {
        const FPropertyAccessSegment& Segment = Segments[SegmentIndex];
        const bool bLastSegment = SegmentIndex == Segments.size() - 1;
        auto* SegmentProperty = CastField<FProperty>(Segment.Property.Get());
        if (SegmentProperty == nullptr) return false;

        const EPropertyAccessSegmentFlags UnmodifiedFlags = GetUnmodifiedFlags(Segment);
        if (Segment.Flags & static_cast<uint16>(EPropertyAccessSegmentFlags::Function)) {
            if (Segment.Function == nullptr) return false;
            FPropertyAccessIndirection Indirection;
            Indirection.Type = Segment.Function->HasAnyFunctionFlags(FUNC_Native) ? EPropertyAccessIndirectionType::NativeFunction
                                                                                  : EPropertyAccessIndirectionType::ScriptFunction;
            UE5PARSE_CHECK(UnmodifiedFlags == EPropertyAccessSegmentFlags::Struct || UnmodifiedFlags == EPropertyAccessSegmentFlags::Leaf ||
                           GetObjectType(UnmodifiedFlags) != EPropertyAccessObjectType::None);
            Indirection.ObjectType = GetObjectType(UnmodifiedFlags);
            Indirection.Function = Segment.Function;
            Indirection.Property = Segment.Function->GetReturnProperty();
            Indirection.ReturnBufferSize = SegmentProperty->GetSize();
            Indirection.ReturnBufferAlignment = SegmentProperty->GetMinAlignment();
            PathIndirections.Add(Indirection);
            continue;
        }

        const int32 ArrayIndex = Segment.ArrayIndex == INDEX_NONE ? 0 : Segment.ArrayIndex;
        switch (UnmodifiedFlags) {
        case EPropertyAccessSegmentFlags::Struct:
        case EPropertyAccessSegmentFlags::Leaf:
            PathIndirections.Add({.Offset = GetPropertyOffset(SegmentProperty, ArrayIndex), .Type = EPropertyAccessIndirectionType::Offset});
            break;
        case EPropertyAccessSegmentFlags::Object:
        case EPropertyAccessSegmentFlags::WeakObject:
        case EPropertyAccessSegmentFlags::SoftObject:
            PathIndirections.Add({.Offset = GetPropertyOffset(SegmentProperty, ArrayIndex),
                                  .ObjectType = GetObjectType(UnmodifiedFlags),
                                  .Type = EPropertyAccessIndirectionType::Object});
            break;
        case EPropertyAccessSegmentFlags::Array:
        case EPropertyAccessSegmentFlags::ArrayOfStructs:
        case EPropertyAccessSegmentFlags::ArrayOfObjects: {
            auto* ArrayProperty = CastField<FArrayProperty>(SegmentProperty);
            UE5PARSE_CHECK(ArrayProperty != nullptr);
            PathIndirections.Add({.Property = ArrayProperty,
                                  .ArrayIndex = ArrayIndex,
                                  .Offset = GetPropertyOffset(ArrayProperty),
                                  .Type = EPropertyAccessIndirectionType::Array});
            // An object array needs an object dereference when it is not the leaf.
            if (UnmodifiedFlags == EPropertyAccessSegmentFlags::ArrayOfObjects && !bLastSegment) {
                FPropertyAccessIndirection ExtraIndirection{.Type = EPropertyAccessIndirectionType::Object};
                if (CastField<FObjectProperty>(ArrayProperty->Inner)) ExtraIndirection.ObjectType = EPropertyAccessObjectType::Object;
                else if (CastField<FWeakObjectProperty>(ArrayProperty->Inner)) ExtraIndirection.ObjectType = EPropertyAccessObjectType::WeakObject;
                else if (CastField<FSoftObjectProperty>(ArrayProperty->Inner)) ExtraIndirection.ObjectType = EPropertyAccessObjectType::SoftObject;
                PathIndirections.Add(ExtraIndirection);
            }
            break;
        }
        default:
            UE5PARSE_CHECK(false);
            break;
        }
    }

    // Collapses adjacent offset indirections.
    for (int32 IndirectionIndex = 0; IndirectionIndex < PathIndirections.Num(); ++IndirectionIndex) {
        FPropertyAccessIndirection& StartIndirection = PathIndirections[IndirectionIndex];
        if (StartIndirection.Type != EPropertyAccessIndirectionType::Offset) continue;
        while (IndirectionIndex + 1 < PathIndirections.Num() && PathIndirections[IndirectionIndex + 1].Type == EPropertyAccessIndirectionType::Offset) {
            StartIndirection.Offset += PathIndirections[IndirectionIndex + 1].Offset;
            PathIndirections.RemoveAt(IndirectionIndex + 1);
        }
    }

    Access.IndirectionStartIndex = Indirections.Num();
    for (const FPropertyAccessIndirection& Indirection : PathIndirections) Indirections.Add(Indirection);
    Access.IndirectionEndIndex = Indirections.Num();

    const FPropertyAccessSegment& LastSegment = Segments.back();
    auto* LastProperty = CastField<FProperty>(LastSegment.Property.Get());
    switch (GetUnmodifiedFlags(LastSegment)) {
    case EPropertyAccessSegmentFlags::Array:
    case EPropertyAccessSegmentFlags::ArrayOfStructs:
    case EPropertyAccessSegmentFlags::ArrayOfObjects:
        Access.Property = CastField<FArrayProperty>(LastProperty)->Inner;
        break;
    default:
        Access.Property = LastProperty;
        break;
    }
    return true;
}

void FPropertyAccessLibrary::PatchPropertyOffsets() {
    Indirections.Empty();
    const auto Resolve = [this](const TArray<FPropertyAccessPath>& Paths, TArray<FPropertyAccessIndirectionChain>& Accesses) {
        Accesses.Empty(Paths.Num());
        for (const FPropertyAccessPath& Path : Paths) {
            FPropertyAccessIndirectionChain Access;
            const std::span<const FPropertyAccessSegment> Segments(PathSegments.GetData() + Path.PathSegmentStartIndex,
                                                                   static_cast<std::size_t>(Path.PathSegmentCount));
            if (!ResolveIndirectionsOnLoad(Segments, Access)) Access = {};
            Accesses.Add(Access);
        }
    };
    Resolve(SrcPaths, SrcAccesses);
    Resolve(DestPaths, DestAccesses);
    bHasBeenPostLoaded = true;
}

// Gets the address of an access and passes it on: a function's return value lives in a buffer on this stack.
template <class AddressFunctionType>
void FPropertyAccessLibrary::GetAccessAddress(void* InContainer, const FPropertyAccessIndirectionChain& InAccess,
                                              AddressFunctionType&& InAddressFunction) const {
    alignas(16) uint8 InlineBuffer[64];
    TArray<uint8> HeapBuffer;
    void* Address = InContainer;
    for (int32 IndirectionIndex = InAccess.IndirectionStartIndex; Address != nullptr && IndirectionIndex < InAccess.IndirectionEndIndex;
         ++IndirectionIndex) {
        const FPropertyAccessIndirection& Indirection = Indirections.GetData()[IndirectionIndex];
        switch (Indirection.Type) {
        case EPropertyAccessIndirectionType::Offset:
            Address = static_cast<uint8*>(Address) + Indirection.Offset;
            break;
        case EPropertyAccessIndirectionType::Object:
            Address = GetObject(static_cast<uint8*>(Address) + Indirection.Offset, Indirection.ObjectType);
            break;
        case EPropertyAccessIndirectionType::Array: {
            FScriptArrayHelper Helper(static_cast<const FArrayProperty*>(Indirection.Property), static_cast<uint8*>(Address) + Indirection.Offset);
            Address = Helper.IsValidIndex(Indirection.ArrayIndex) ? Helper.GetRawPtr(Indirection.ArrayIndex) : nullptr;
            break;
        }
        case EPropertyAccessIndirectionType::ScriptFunction:
        case EPropertyAccessIndirectionType::NativeFunction: {
            auto* CalleeObject = static_cast<UObject*>(Address);
            const std::size_t BufferSize = static_cast<std::size_t>(Indirection.ReturnBufferSize + Indirection.ReturnBufferAlignment);
            void* Buffer = InlineBuffer;
            if (BufferSize > sizeof(InlineBuffer)) {
                HeapBuffer.SetNum(static_cast<int32>(BufferSize));
                Buffer = HeapBuffer.GetData();
            }
            const auto Alignment = static_cast<std::uintptr_t>(Indirection.ReturnBufferAlignment);
            Address = reinterpret_cast<void*>((reinterpret_cast<std::uintptr_t>(Buffer) + Alignment - 1) & ~(Alignment - 1));
            Indirection.Property->InitializeValue(Address);
            if (Indirection.Type == EPropertyAccessIndirectionType::NativeFunction) CallNativeAccessor(CalleeObject, Indirection.Function, Address);
            else CalleeObject->ProcessEvent(Indirection.Function, Address);
            // A function can return an object, whose pointer the next indirection follows.
            if (Indirection.ObjectType != EPropertyAccessObjectType::None) Address = GetObject(Address, Indirection.ObjectType);
            break;
        }
        }
    }
    if (Address != nullptr) InAddressFunction(Address);
}

void FPropertyAccessLibrary::PerformCopy(const FPropertyAccessCopy& Copy, const FProperty* InSrcProperty, const void* InSrcAddr,
                                         const FProperty* InDestProperty, void* InDestAddr, FPostCopyOperation InPostCopyOperation) {
    switch (Copy.Type) {
    case EPropertyAccessCopyType::Plain:
        std::memcpy(InDestAddr, InSrcAddr, static_cast<std::size_t>(InSrcProperty->ElementSize));
        break;
    case EPropertyAccessCopyType::Complex:
        InSrcProperty->CopyCompleteValue(InDestAddr, InSrcAddr);
        break;
    case EPropertyAccessCopyType::Bool:
        static_cast<const FBoolProperty*>(InDestProperty)->SetPropertyValue(InDestAddr, static_cast<const FBoolProperty*>(InSrcProperty)->GetPropertyValue(InSrcAddr));
        break;
    case EPropertyAccessCopyType::Struct:
        static_cast<const FStructProperty*>(InDestProperty)->Struct->CopyScriptStruct(InDestAddr, InSrcAddr);
        break;
    case EPropertyAccessCopyType::Object:
        static_cast<const FObjectPropertyBase*>(InDestProperty)
            ->SetObjectPropertyValue(InDestAddr, static_cast<const FObjectPropertyBase*>(InSrcProperty)->GetObjectPropertyValue(InSrcAddr));
        break;
    case EPropertyAccessCopyType::Name:
        static_cast<const FNameProperty*>(InDestProperty)->SetPropertyValue(InDestAddr, static_cast<const FNameProperty*>(InSrcProperty)->GetPropertyValue(InSrcAddr));
        break;
    case EPropertyAccessCopyType::Array: {
        const auto* SrcArrayProperty = static_cast<const FArrayProperty*>(InSrcProperty);
        FScriptArrayHelper SourceArrayHelper(SrcArrayProperty, InSrcAddr);
        FScriptArrayHelper DestArrayHelper(static_cast<const FArrayProperty*>(InDestProperty), InDestAddr);
        // The minimum number of elements, without resizing the destination.
        const int32 MinSize = std::min(SourceArrayHelper.Num(), DestArrayHelper.Num());
        for (int32 ElementIndex = 0; ElementIndex < MinSize; ++ElementIndex)
            SrcArrayProperty->Inner->CopySingleValue(DestArrayHelper.GetRawPtr(ElementIndex), SourceArrayHelper.GetRawPtr(ElementIndex));
        break;
    }
    case EPropertyAccessCopyType::PromoteBoolToByte:
        Convert<FByteProperty, FBoolProperty, uint8>(InSrcProperty, InSrcAddr, InDestProperty, InDestAddr);
        break;
    case EPropertyAccessCopyType::PromoteBoolToInt32:
        Convert<FIntProperty, FBoolProperty, int32>(InSrcProperty, InSrcAddr, InDestProperty, InDestAddr);
        break;
    case EPropertyAccessCopyType::PromoteBoolToInt64:
        Convert<FInt64Property, FBoolProperty, int64>(InSrcProperty, InSrcAddr, InDestProperty, InDestAddr);
        break;
    case EPropertyAccessCopyType::PromoteBoolToFloat:
        Convert<FFloatProperty, FBoolProperty, float>(InSrcProperty, InSrcAddr, InDestProperty, InDestAddr);
        break;
    case EPropertyAccessCopyType::PromoteBoolToDouble:
        Convert<FDoubleProperty, FBoolProperty, double>(InSrcProperty, InSrcAddr, InDestProperty, InDestAddr);
        break;
    case EPropertyAccessCopyType::PromoteByteToInt32:
        Convert<FIntProperty, FByteProperty, int32>(InSrcProperty, InSrcAddr, InDestProperty, InDestAddr);
        break;
    case EPropertyAccessCopyType::PromoteByteToInt64:
        Convert<FInt64Property, FByteProperty, int64>(InSrcProperty, InSrcAddr, InDestProperty, InDestAddr);
        break;
    case EPropertyAccessCopyType::PromoteByteToFloat:
        Convert<FFloatProperty, FByteProperty, float>(InSrcProperty, InSrcAddr, InDestProperty, InDestAddr);
        break;
    case EPropertyAccessCopyType::PromoteByteToDouble:
        Convert<FDoubleProperty, FByteProperty, double>(InSrcProperty, InSrcAddr, InDestProperty, InDestAddr);
        break;
    case EPropertyAccessCopyType::PromoteInt32ToInt64:
        Convert<FInt64Property, FIntProperty, int64>(InSrcProperty, InSrcAddr, InDestProperty, InDestAddr);
        break;
    case EPropertyAccessCopyType::PromoteInt32ToFloat:
        Convert<FFloatProperty, FIntProperty, float>(InSrcProperty, InSrcAddr, InDestProperty, InDestAddr);
        break;
    case EPropertyAccessCopyType::PromoteInt32ToDouble:
        Convert<FDoubleProperty, FIntProperty, double>(InSrcProperty, InSrcAddr, InDestProperty, InDestAddr);
        break;
    case EPropertyAccessCopyType::PromoteFloatToDouble:
        Convert<FDoubleProperty, FFloatProperty, double>(InSrcProperty, InSrcAddr, InDestProperty, InDestAddr);
        break;
    case EPropertyAccessCopyType::DemoteDoubleToFloat:
        Convert<FFloatProperty, FDoubleProperty, float>(InSrcProperty, InSrcAddr, InDestProperty, InDestAddr);
        break;
    case EPropertyAccessCopyType::PromoteArrayFloatToDouble:
        CopyAndCastFloatingPointArray<float, double>(static_cast<const FArrayProperty*>(InSrcProperty), InSrcAddr,
                                                     static_cast<const FArrayProperty*>(InDestProperty), InDestAddr);
        break;
    case EPropertyAccessCopyType::DemoteArrayDoubleToFloat:
        CopyAndCastFloatingPointArray<double, float>(static_cast<const FArrayProperty*>(InSrcProperty), InSrcAddr,
                                                     static_cast<const FArrayProperty*>(InDestProperty), InDestAddr);
        break;
    case EPropertyAccessCopyType::PromoteMapValueFloatToDouble:
        CopyAndCastFloatingPointMapValues<float, double>(static_cast<const FMapProperty*>(InSrcProperty), InSrcAddr,
                                                         static_cast<const FMapProperty*>(InDestProperty), InDestAddr);
        break;
    case EPropertyAccessCopyType::DemoteMapValueDoubleToFloat:
        CopyAndCastFloatingPointMapValues<double, float>(static_cast<const FMapProperty*>(InSrcProperty), InSrcAddr,
                                                         static_cast<const FMapProperty*>(InDestProperty), InDestAddr);
        break;
    case EPropertyAccessCopyType::None:
        UE5PARSE_CHECK(false);
        break;
    }
    if (InPostCopyOperation != nullptr) InPostCopyOperation(InDestProperty, InDestAddr);
}

void FPropertyAccessLibrary::ProcessCopy(UObject* Object, int32 BatchId, int32 CopyIndex, FPostCopyOperation PostCopyOperation) const {
    if (!bHasBeenPostLoaded || !CopyBatchArray.IsValidIndex(BatchId)) return;
    const FPropertyAccessCopy& Copy = CopyBatchArray[BatchId].Copies[CopyIndex];
    const FPropertyAccessIndirectionChain& SrcAccess = SrcAccesses[Copy.AccessIndex];
    const FProperty* SourceProperty = SrcAccess.Property;
    if (SourceProperty == nullptr) return;
    GetAccessAddress(Object, SrcAccess, [this, Object, &Copy, SourceProperty, PostCopyOperation](void* InSrcAddress) {
        for (int32 DestAccessIndex = Copy.DestAccessStartIndex; DestAccessIndex < Copy.DestAccessEndIndex; ++DestAccessIndex) {
            const FPropertyAccessIndirectionChain& DestAccess = DestAccesses[DestAccessIndex];
            const FProperty* DestProperty = DestAccess.Property;
            if (DestProperty == nullptr) continue;
            GetAccessAddress(Object, DestAccess, [&](void* InDestAddress) {
                PerformCopy(Copy, SourceProperty, InSrcAddress, DestProperty, InDestAddress, PostCopyOperation);
            });
        }
    });
}

void FPropertyAccessLibrary::ProcessCopies(UObject* Object, int32 BatchId) const {
    if (!bHasBeenPostLoaded || !CopyBatchArray.IsValidIndex(BatchId)) return;
    const int32 NumCopies = CopyBatchArray[BatchId].Copies.Num();
    for (int32 CopyIndex = 0; CopyIndex < NumCopies; ++CopyIndex) ProcessCopy(Object, BatchId, CopyIndex);
}

}
