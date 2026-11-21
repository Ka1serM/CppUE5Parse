#include "Engine/Source/Runtime/Engine/Animation/AttributeCurve.h"

namespace ue5parse::runtime {

bool FAttributeCurve::Serialize(FArchive& Ar) {
    const int32 NumKeys = Ar.ReadCount(sizeof(float));
    Keys.SetNum(NumKeys);
    for (FAttributeKey& Key : Keys)
        Ar << Key.Time;
    Ar << ScriptStructPath;
    if (Ar.IsError() || ScriptStructPath.IsNull())
        return true;
    UScriptStruct* Struct = Cast<UScriptStruct>(ScriptStructPath.ResolveObject());
    if (Struct == nullptr) {
        Ar.SetError(ErrorCode::MissingSchema, "FAttributeCurve.ScriptStructPath");
        return true;
    }
    for (FAttributeKey& Key : Keys) {
        Key.Value.InitializeAs(Struct);
        Struct->SerializeItem(Ar, Key.Value.GetMutableMemory());
        if (Ar.IsError())
            break;
    }
    return true;
}

}
