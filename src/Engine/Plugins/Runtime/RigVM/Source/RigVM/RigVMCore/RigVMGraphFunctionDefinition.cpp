#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMGraphFunctionDefinition.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

namespace {

bool HasDebugMap(FArchive& Ar) {
    return Ar.CustomVerAtLeast(FUE5ReleaseStreamObjectVersion::GUID, FUE5ReleaseStreamObjectVersion::RigVMSaveDebugMapInGraphFunctionData, EGame::GAME_UE5_3)
           || Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::RigVMSaveDebugMapInGraphFunctionData,
                                  EGame::GAME_UE5_4);
}

//! UE loads the operand to debug register map with byte counts and discards it.
void SerializeOperandToDebugRegisters(FArchive& Ar) {
    uint8 NumKeys = 0;
    Ar << NumKeys;
    for (int32 KeyIndex = 0; KeyIndex < NumKeys && !Ar.IsError(); ++KeyIndex) {
        FRigVMOperand Key;
        uint8 NumValues = 0;
        Ar << Key << NumValues;
        for (int32 ValueIndex = 0; ValueIndex < NumValues && !Ar.IsError(); ++ValueIndex) {
            FRigVMOperand Value;
            Ar << Value;
        }
    }
}

}

FArchive& operator<<(FArchive& Ar, FRigVMFunctionCompilationData& Data) {
    Ar << Data.ByteCode << Data.FunctionNames << Data.WorkPropertyDescriptions << Data.WorkPropertyPathDescriptions << Data.LiteralPropertyDescriptions
       << Data.LiteralPropertyPathDescriptions << Data.DebugPropertyDescriptions << Data.DebugPropertyPathDescriptions << Data.ExternalPropertyDescriptions
       << Data.ExternalPropertyPathDescriptions << Data.ExternalRegisterIndexToVariable << Data.Operands;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::RigVMCallables))
        Ar << Data.InterfaceOperands;
    Ar << Data.Hash;
    if (!HasDebugMap(Ar) || FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::DebugOperandMappingSimplified))
        return Ar;
    SerializeOperandToDebugRegisters(Ar);
    return Ar;
}

void FRigVMGraphFunctionArgument::Serialize(FArchive& Ar) {
    Ar << Name << DisplayName << CPPType << CPPTypeObject << bIsArray << Direction << DefaultValue << bIsConst << PathToTooltip;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::FunctionArgumentCanRepresentInputVariable))
        Ar << bIsInputVariable;
}

FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionIdentifier& Data) {
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::RemoveLibraryNodeReferenceFromFunctionIdentifier))
        Ar << Data.LibraryNodePath;
    else {
        FSoftObjectPath SoftPath;
        Ar << SoftPath;
        Data.LibraryNodePath = SoftPath.ToString();
    }
    return Ar << Data.HostObject;
}

FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionHeader& Data) {
    Ar << Data.LibraryPointer;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::AddVariantToFunctionIdentifier))
        Ar << Data.Variant;
    Ar << Data.Name << Data.NodeTitle << Data.NodeColor;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::VMRemoveTooltipFromFunctionHeader))
        Ar << Data.Description;
    else
        Ar << Data.Tooltip_DEPRECATED;
    Ar << Data.Category << Data.Keywords << Data.Arguments << Data.Dependencies << Data.ExternalVariables;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::FunctionHeaderStoresLayout))
        Ar << Data.Layout;
    return Ar;
}

FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionData& Data) {
    Ar << Data.Header << Data.CompilationData;
    if (!Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::RigVMSaveSerializedGraphInGraphFunctionData, EGame::GAME_UE5_2))
        return Ar;
    Ar << Data.SerializedCollapsedNode_DEPRECATED;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::RigVMSaveSerializedGraphInGraphFunctionDataAsByteArray))
        Ar << Data.CollapseNodeArchive;
    return Ar;
}

FArchive& operator<<(FArchive& Ar, FRigVMNodeLayout& Layout) {
    Ar << Layout.Categories;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::FunctionHeaderLayoutStoresPinIndexInCategory))
        Ar << Layout.PinIndexInCategory;
    return Ar << Layout.DisplayNames;
}

}
