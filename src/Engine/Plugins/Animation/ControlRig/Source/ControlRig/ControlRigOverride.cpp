#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/ControlRigOverride.h"

#include <limits>

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/ControlRigObjectVersion.h"

namespace ue5parse::runtime {

template <class T>
static void ReadOptional(FArchive& Ar, std::optional<T>& Value) {
    bool bIsSet = false;
    Ar << bIsSet;
    if (bIsSet)
        Ar << Value.emplace();
    else
        Value.reset();
}

FArchive& operator<<(FArchive& Ar, FControlRigOverrideValue& Value) {
    using Version = FControlRigObjectVersion;
    ReadOptional(Ar, Value.SubjectKey);
    if (Ar.CustomVerAtLeast(Version::GUID, Version::OverridesStorePathAndLeafPropertyOnly, EGame::GAME_UE5_8)) {
        Ar << Value.OwnerStructPath;
        ReadOptional(Ar, Value.Path);
    } else
        Ar << Value.Properties;

    const int64 Start = Ar.Tell();
    int64& Offset = Value.OffsetForData;
    Ar << Offset;
    if (!Ar.CustomVerAtLeast(Version::GUID, Version::OverridesStoreDatSkipOffsetAsInt64, EGame::GAME_UE5_8))
        Offset = static_cast<int32>(Offset);
    if (Ar.IsError() || Offset == sizeof(int64))
        return Ar;
    if (Offset == 0 && !Ar.CustomVerAtLeast(Version::GUID, Version::OverridesStoreDatSkipOffsetAsInt64, EGame::GAME_UE5_8))
        return Ar;
    if (Ar.CustomVerAtLeast(Version::GUID, Version::OverridesStoreLeafPropertyHashOnly, EGame::GAME_UE5_8))
        Ar << Value.PropertyHash;
    else if (Ar.CustomVerAtLeast(Version::GUID, Version::OverridesStoreTOCDataForProperties, EGame::GAME_UE5_7))
        Ar << Value.PropertySize << Value.PropertyHash;
    if (Ar.IsError())
        return Ar;
    const int64 HeaderSize = Ar.Tell() - Start;
    if (Offset < HeaderSize || Offset > Ar.TotalSize() - Start || Offset - HeaderSize > std::numeric_limits<int32>::max()) {
        Ar.SetError(ErrorCode::InvalidData, "FControlRigOverrideValue.OffsetForData");
        return Ar;
    }
    SerializeRaw(Ar, Value.Data, static_cast<int32>(Offset - HeaderSize));
    return Ar;
}

}
