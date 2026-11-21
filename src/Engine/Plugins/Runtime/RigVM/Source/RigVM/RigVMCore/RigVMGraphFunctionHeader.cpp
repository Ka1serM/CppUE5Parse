#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMGraphFunctionHeader.h"

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionIdentifier& Identifier) {
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::RemoveLibraryNodeReferenceFromFunctionIdentifier))
        Ar << Identifier.LibraryNodePath;
    else {
        FSoftObjectPath Node;
        Ar << Node;
        Identifier.LibraryNodePath = Node.ToString();
    }
    return Ar << Identifier.HostObject;
}

FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionArgument& Argument) {
    Ar << Argument.Name << Argument.DisplayName << Argument.CPPType << Argument.CPPTypeObject << Argument.bIsArray << Argument.Direction;
    Ar << Argument.DefaultValue << Argument.bIsConst << Argument.PathToTooltip;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::FunctionArgumentCanRepresentInputVariable))
        Ar << Argument.bIsInputVariable;
    return Ar;
}

FArchive& operator<<(FArchive& Ar, FRigVMExternalVariableDef& Variable) {
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::GuidForVariables))
        Ar << Variable.Guid;
    return Ar << Variable.Name << Variable.BaseCPPType << Variable.CPPTypeObject << Variable.bIsArray << Variable.bIsPublic << Variable.bIsReadOnly << Variable.Size;
}

FArchive& operator<<(FArchive& Ar, FRigVMPinCategory& Category) {
    Ar << Category.Path << Category.Elements;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::FunctionHeaderLayoutStoresCategoryExpansion))
        Ar << Category.bExpandedByDefault;
    return Ar;
}

FArchive& operator<<(FArchive& Ar, FRigVMNodeLayout& Layout) {
    Ar << Layout.Categories;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::FunctionHeaderLayoutStoresPinIndexInCategory))
        Ar << Layout.PinIndexInCategory;
    return Ar << Layout.DisplayNames;
}

FArchive& operator<<(FArchive& Ar, FRigVMTag& Tag) {
    return Ar << Tag.Name << Tag.Label << Tag.ToolTip << Tag.Color << Tag.bShowInUserInterface << Tag.bMarksSubjectAsInvalid;
}

FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionHeader& Header) {
    Ar << Header.LibraryPointer;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::AddVariantToFunctionIdentifier))
        Ar << Header.Variant.Guid << Header.Variant.Tags;
    Ar << Header.Name << Header.NodeTitle << Header.NodeColor;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::VMRemoveTooltipFromFunctionHeader))
        Ar << Header.Description;
    else
        Ar << Header.Tooltip_DEPRECATED;
    Ar << Header.Category << Header.Keywords << Header.Arguments << Header.Dependencies << Header.ExternalVariables;
    if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::FunctionHeaderStoresLayout))
        Ar << Header.Layout;
    return Ar;
}

}
