#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FRigVMObjectVersion {
    static constexpr FGuid GUID{0xDC49959B, 0x53C04DE7, 0x9156EA88, 0x5E7C5D39};
    enum Type : int32 {
        BeforeCustomVersionWasAdded = 0,
        AddedVMHashChecks,
        PredicatesAddedToExecuteOps,
        VMStoringUserDefinedStructMap,
        VMStoringUserDefinedEnumMap,
        HostStoringUserDefinedData,
        VMMemoryStorageStructSerialized,
        VMMemoryStorageDefaultsGeneratedAtVM,
        VMBytecodeStorePublicContextPath,
        VMRemoveTooltipFromFunctionHeader,
        RemoveLibraryNodeReferenceFromFunctionIdentifier,
        AddVariantToFunctionIdentifier,
        AddVariantToRigVMAssets,
        FunctionHeaderStoresLayout,
        FunctionHeaderLayoutStoresPinIndexInCategory,
        FunctionHeaderLayoutStoresCategoryExpansion,
        RigVMSaveSerializedGraphInGraphFunctionDataAsByteArray,
        VMBytecodeStorePublicContextPathAsTopLevelAssetPath,
        ByteCodeCleanup,
        LocalizedRegistry,
        LocalizedRegistryWithRelativeSeekOffset,
        FunctionArgumentCanRepresentInputVariable,
        ObjectArchiveVersionContainerSerialization,
        DebugOperandMappingSimplified,
        RigVMCallables,
        GuidForVariables,
        NativizedFunctionAsCallables,
        GuidForFunctions,
    };

    static bool AtLeast(const FArchive& Ar, Type Version) { return Ar.CustomVerAtLeast(GUID, Version, Since(Version)); }

private:
    static EGame Since(Type Version) {
        if (Version >= NativizedFunctionAsCallables)
            return EGame::GAME_UE5_9;
        if (Version >= LocalizedRegistryWithRelativeSeekOffset)
            return EGame::GAME_UE5_8;
        if (Version >= LocalizedRegistry)
            return EGame::GAME_UE5_7;
        if (Version >= RigVMSaveSerializedGraphInGraphFunctionDataAsByteArray)
            return EGame::GAME_UE5_6;
        if (Version >= RemoveLibraryNodeReferenceFromFunctionIdentifier)
            return EGame::GAME_UE5_5;
        if (Version >= VMStoringUserDefinedStructMap)
            return EGame::GAME_UE5_4;
        return EGame::GAME_UE5_3;
    }
};

}
