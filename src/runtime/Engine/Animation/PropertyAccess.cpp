#include "ue5parse/runtime/Engine/Animation/PropertyAccess.h"

#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

bool FPropertyAccessLibrary::ResolvePath(UObject* Object, const FPropertyAccessPath& Path,
        const FProperty*& Property, void*& Address) const {
    Property = nullptr;
    Address = nullptr;
    if (!Object || Path.PathSegmentStartIndex < 0 || Path.PathSegmentCount <= 0 ||
        Path.PathSegmentStartIndex + Path.PathSegmentCount > PathSegments.Num()) return false;

    const UStruct* CurrentStruct = Object->GetClass();
    void* CurrentData = Object;
    for (int32 Index = 0; Index < Path.PathSegmentCount; ++Index) {
        const FPropertyAccessSegment& Segment = PathSegments[Path.PathSegmentStartIndex + Index];
        if (Segment.Function || !CurrentStruct || !CurrentData) return false;
        Property = CurrentStruct->FindPropertyByName(Segment.Name);
        if (!Property) return false;
        Address = Property->ContainerPtrToValuePtr<void>(CurrentData);
        if (Segment.ArrayIndex >= 0) {
            if (const auto* Array = CastField<FArrayProperty>(Property)) {
                FScriptArrayHelper Elements(Array, Address);
                if (!Elements.IsValidIndex(Segment.ArrayIndex)) return false;
                Address = Elements.GetRawPtr(Segment.ArrayIndex);
                Property = Array->Inner;
            } else {
                if (Segment.ArrayIndex >= Property->ArrayDim) return false;
                Address = Property->ContainerPtrToValuePtr<void>(CurrentData, Segment.ArrayIndex);
            }
        }
        if (Index + 1 == Path.PathSegmentCount) return true;
        if (const auto* Struct = CastField<FStructProperty>(Property)) {
            CurrentStruct = Struct->Struct;
            CurrentData = Address;
        } else if (const auto* Reference = CastField<FObjectPropertyBase>(Property)) {
            CurrentData = Reference->GetObjectPropertyValue(Address);
            CurrentStruct = CurrentData ? static_cast<UObject*>(CurrentData)->GetClass() : nullptr;
        } else {
            return false;
        }
    }
    return false;
}

bool FPropertyAccessLibrary::ProcessCopy(UObject* Object, int32 CopyIndex, bool bNegate) const {
    if (!CopyBatchArray.IsValidIndex(0) || !CopyBatchArray[0].Copies.IsValidIndex(CopyIndex)) return false;
    const FPropertyAccessCopy& Copy = CopyBatchArray[0].Copies[CopyIndex];
    if (!SrcPaths.IsValidIndex(Copy.AccessIndex) || Copy.DestAccessStartIndex < 0 ||
        Copy.DestAccessEndIndex > DestPaths.Num() || Copy.DestAccessEndIndex <= Copy.DestAccessStartIndex) return false;

    const FProperty* SourceProperty = nullptr;
    void* Source = nullptr;
    if (!ResolvePath(Object, SrcPaths[Copy.AccessIndex], SourceProperty, Source)) return false;
    for (int32 Index = Copy.DestAccessStartIndex; Index < Copy.DestAccessEndIndex; ++Index) {
        const FProperty* DestinationProperty = nullptr;
        void* Destination = nullptr;
        if (!ResolvePath(Object, DestPaths[Index], DestinationProperty, Destination)) return false;
        if (bNegate) {
            const auto* SourceBool = CastField<FBoolProperty>(SourceProperty);
            const auto* DestinationBool = CastField<FBoolProperty>(DestinationProperty);
            if (!SourceBool || !DestinationBool) return false;
            DestinationBool->SetPropertyValue(Destination, !SourceBool->GetPropertyValue(Source));
        } else if (DestinationProperty->SameType(SourceProperty)) {
            DestinationProperty->CopyCompleteValue(Destination, Source);
        } else {
            const auto* SourceNumber = CastField<FNumericProperty>(SourceProperty);
            const auto* DestinationNumber = CastField<FNumericProperty>(DestinationProperty);
            if (!SourceNumber || !DestinationNumber) return false;
            const double Value = SourceNumber->IsFloatingPoint()
                ? SourceNumber->GetFloatingPointPropertyValue(Source)
                : static_cast<double>(SourceNumber->GetSignedIntPropertyValue(Source));
            if (DestinationNumber->IsFloatingPoint()) DestinationNumber->SetFloatingPointPropertyValue(Destination, Value);
            else DestinationNumber->SetIntPropertyValue(Destination, static_cast<int64>(Value));
        }
    }
    return true;
}

}
