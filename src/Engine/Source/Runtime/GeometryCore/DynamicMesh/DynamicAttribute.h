#pragma once

#include "CppUE5Parse/Core/FName.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"

namespace ue5parse::runtime {

class TDynamicAttributeBase {
public:
    void Serialize(FArchive& Ar) {
        if (!Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::DynamicMeshAttributesWeightMapsAndNames, EGame::GAME_UE5_1))
            return;
        FString NameString;
        Ar << NameString;
        Name = MakeName(NameString);
    }

    FName GetName() const { return Name; }

private:
    FName Name;
};

}
