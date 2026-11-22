#include "GameTypes/Concord/PMTimelineEvent.h"

namespace ue5parse::runtime {

void UPMTimelineEvent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (!Ar.IsError())
        SerializeStructFallback(Ar, FName("PMTimelineEventExecutionProperties"), Properties);
}

bool FPMFloatMapping::Serialize(FArchive& Ar) {
    const uint8 Type = Ar.Read<uint8>();
    switch (Type) {
    case 0:
        return true;
    case 1:
        Ar << mMappingDef;
        return true;
    case 2:
        Ar << mConstant;
        return true;
    default:
        Ar.SetError(ErrorCode::InvalidData, "FPMFloatMapping.Type");
        return true;
    }
}

bool FPMTimelineObjectBindingDef::Serialize(FArchive& Ar) {
    StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(this));
    if (!Ar.IsError())
        NonConstStruct.Serialize(Ar);
    return true;
}

bool FGameplayEffectApplicationDataHandle::Serialize(FArchive& Ar) {
    Ar << StructType;
    UnknownValues.SetNum(4);
    for (int32& Value : UnknownValues)
        Ar << Value;
    return true;
}

}
