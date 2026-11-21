#pragma once

#include "Engine/Plugins/Runtime/Metasound/Source/MetasoundFrontend/DocumentTemplates/MetasoundFrontendDocumentTemplate.h"
#include "Engine/Plugins/Runtime/Metasound/Source/MetasoundFrontend/MetasoundFrontendLiteral.h"
#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "MetasoundFrontendDocument.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EMetasoundFrontendClassType : uint8 {
    External,
    Graph,
    Input,
    Output,
    Literal,
    Variable,
    VariableDeferredAccessor,
    VariableAccessor,
    VariableMutator,
    Template,
    Invalid,
};

UENUM()
enum class EMetasoundFrontendClassAccessFlags : uint16 { None = 0, Deprecated = 1 << 0, Referenceable = 1 << 1 };

UENUM()
enum class EMetasoundFrontendVertexAccessType : int32 { Reference, Value, Unset };

UENUM()
enum class EMetasoundFrontendNodeStyleDisplayVisibility : uint8 { Visible, Hidden };

UENUM()
enum class EMetasoundFrontendStyleEdgeDisplay : uint8 { Default, Inherited, Hidden };

USTRUCT()
struct FMetasoundFrontendVersionNumber {
    GENERATED_BODY()

    UPROPERTY()
    int32 Major = 1;
    UPROPERTY()
    int32 Minor = 0;

    bool operator==(const FMetasoundFrontendVersionNumber& Other) const = default;
};

USTRUCT()
struct FMetasoundFrontendVersion {
    GENERATED_BODY()

    UPROPERTY()
    FName Name;
    UPROPERTY()
    FMetasoundFrontendVersionNumber Number;

    bool operator==(const FMetasoundFrontendVersion& Other) const = default;
    friend uint32 GetTypeHash(const FMetasoundFrontendVersion& Version) {
        return HashCombine(GetTypeHash(Version.Name), HashCombine(static_cast<uint32>(Version.Number.Major), static_cast<uint32>(Version.Number.Minor)));
    }
};

USTRUCT()
struct FMetasoundFrontendVertex {
    GENERATED_BODY()

    UPROPERTY()
    FName Name;
    UPROPERTY()
    FName TypeName;
    UPROPERTY()
    FGuid VertexID;
};

USTRUCT()
struct FMetasoundFrontendVertexHandle {
    GENERATED_BODY()

    UPROPERTY()
    FGuid NodeID;
    UPROPERTY()
    FGuid VertexID;
};

USTRUCT()
struct FMetasoundFrontendVertexLiteral {
    GENERATED_BODY()

    UPROPERTY()
    FGuid VertexID;
    UPROPERTY()
    FMetasoundFrontendLiteral Value;
};

USTRUCT()
struct FMetasoundFrontendVariable {
    GENERATED_BODY()

    UPROPERTY()
    FName Name;
    UPROPERTY()
    FText DisplayName;
    UPROPERTY()
    FText Description;
    UPROPERTY()
    FName TypeName;
    UPROPERTY()
    FMetasoundFrontendLiteral Literal;
    UPROPERTY()
    FGuid ID;
    UPROPERTY()
    FGuid VariableNodeID;
    UPROPERTY()
    FGuid MutatorNodeID;
    UPROPERTY()
    TArray<FGuid> AccessorNodeIDs;
    UPROPERTY()
    TArray<FGuid> DeferredAccessorNodeIDs;
};

USTRUCT()
struct FMetasoundFrontendNodeInterface {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMetasoundFrontendVertex> Inputs;
    UPROPERTY()
    TArray<FMetasoundFrontendVertex> Outputs;
    UPROPERTY()
    TArray<FMetasoundFrontendVertex> Environment;
};

USTRUCT()
struct FMetasoundFrontendNodeStyleDisplay {
    GENERATED_BODY()

    UPROPERTY()
    EMetasoundFrontendNodeStyleDisplayVisibility Visibility = EMetasoundFrontendNodeStyleDisplayVisibility::Visible;
    UPROPERTY()
    TMap<FGuid, FVector2D> Locations;
};

USTRUCT()
struct FMetasoundFrontendNodeStyle {
    GENERATED_BODY()

    UPROPERTY()
    FMetasoundFrontendNodeStyleDisplay Display;
    UPROPERTY()
    bool bMessageNodeUpdated = false;
    UPROPERTY()
    bool bIsPrivate = false;
};

USTRUCT()
struct FMetaSoundFrontendNodeConfiguration {
    GENERATED_BODY()
};

USTRUCT()
struct FMetasoundFrontendInterfaceStyle {
    GENERATED_BODY()
};

USTRUCT()
struct FMetasoundFrontendClassVertex : public FMetasoundFrontendVertex {
    GENERATED_BODY()

    UPROPERTY()
    FGuid NodeID;
    UPROPERTY()
    EMetasoundFrontendVertexAccessType AccessType = EMetasoundFrontendVertexAccessType::Reference;
};

USTRUCT()
struct FMetasoundFrontendClassInputDefault {
    GENERATED_BODY()

    UPROPERTY()
    FMetasoundFrontendLiteral Literal;
    UPROPERTY()
    FGuid PageID;
};

USTRUCT()
struct FMetasoundFrontendClassInput : public FMetasoundFrontendClassVertex {
    GENERATED_BODY()

    UPROPERTY()
    FMetasoundFrontendLiteral DefaultLiteral;
    UPROPERTY()
    TArray<FMetasoundFrontendClassInputDefault> Defaults;
};

USTRUCT()
struct FMetasoundFrontendClassVariable : public FMetasoundFrontendClassVertex {
    GENERATED_BODY()

    UPROPERTY()
    FMetasoundFrontendLiteral DefaultLiteral;
};

USTRUCT()
struct FMetasoundFrontendClassOutput : public FMetasoundFrontendClassVertex {
    GENERATED_BODY()
};

USTRUCT()
struct FMetasoundFrontendClassEnvironmentVariable {
    GENERATED_BODY()

    UPROPERTY()
    FName Name;
    UPROPERTY()
    FName TypeName;
    UPROPERTY()
    bool bIsRequired = true;
};

USTRUCT()
struct FMetasoundFrontendClassInterface {
    GENERATED_BODY()

    UPROPERTY()
    FMetasoundFrontendInterfaceStyle InputStyle;
    UPROPERTY()
    FMetasoundFrontendInterfaceStyle OutputStyle;
    UPROPERTY()
    TArray<FMetasoundFrontendClassInput> Inputs;
    UPROPERTY()
    TArray<FMetasoundFrontendClassOutput> Outputs;
    UPROPERTY()
    TArray<FMetasoundFrontendClassEnvironmentVariable> Environment;
    UPROPERTY()
    FGuid ChangeID;
};

USTRUCT()
struct FMetasoundFrontendNode {
    GENERATED_BODY()

    UPROPERTY()
    FGuid ID;
    UPROPERTY()
    FGuid ClassID;
    UPROPERTY()
    FName Name;
    UPROPERTY()
    FMetasoundFrontendNodeInterface Interface;
    UPROPERTY()
    TArray<FMetasoundFrontendVertexLiteral> InputLiterals;
    UPROPERTY()
    FMetasoundFrontendNodeStyle Style;
    UPROPERTY()
    TInstancedStruct<FMetaSoundFrontendNodeConfiguration> Configuration;
    UPROPERTY()
    TInstancedStruct<FMetasoundFrontendClassInterface> ClassInterfaceOverride;
};

USTRUCT()
struct FMetasoundFrontendEdge {
    GENERATED_BODY()

    UPROPERTY()
    FGuid FromNodeID;
    UPROPERTY()
    FGuid FromVertexID;
    UPROPERTY()
    FGuid ToNodeID;
    UPROPERTY()
    FGuid ToVertexID;
};

USTRUCT()
struct FMetasoundFrontendEdgeStyleLiteralColorPair {
    GENERATED_BODY()

    UPROPERTY()
    FMetasoundFrontendLiteral Value;
    UPROPERTY()
    FLinearColor Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
};

USTRUCT()
struct FMetasoundFrontendEdgeStyle {
    GENERATED_BODY()

    UPROPERTY()
    FGuid NodeID;
    UPROPERTY()
    FName OutputName;
    UPROPERTY()
    TArray<FMetasoundFrontendEdgeStyleLiteralColorPair> LiteralColorPairs;
};

//! The 5.0 edge style, which 5.1 replaced with FMetasoundFrontendEdgeStyle.
USTRUCT()
struct FMetasoundFrontendStyleEdge {
    GENERATED_BODY()

    UPROPERTY()
    EMetasoundFrontendStyleEdgeDisplay Display = EMetasoundFrontendStyleEdgeDisplay::Default;
};

USTRUCT()
struct FMetasoundFrontendStyleEdgeClass {
    GENERATED_BODY()

    UPROPERTY()
    FName TypeName;
    UPROPERTY()
    FMetasoundFrontendStyleEdge Style;
};

USTRUCT()
struct FMetasoundFrontendGraphStyle {
    GENERATED_BODY()

    UPROPERTY()
    bool bIsGraphEditable = true;
    UPROPERTY()
    TArray<FMetasoundFrontendEdgeStyle> EdgeStyles;
};

USTRUCT()
struct FMetasoundFrontendGraph {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMetasoundFrontendNode> Nodes;
    UPROPERTY()
    TArray<FMetasoundFrontendEdge> Edges;
    UPROPERTY()
    TArray<FMetasoundFrontendVariable> Variables;
    UPROPERTY()
    FMetasoundFrontendGraphStyle Style;
    UPROPERTY()
    FGuid PageID;
};

USTRUCT()
struct FMetasoundFrontendClassName {
    GENERATED_BODY()

    UPROPERTY()
    FName Namespace;
    UPROPERTY()
    FName Name;
    UPROPERTY()
    FName Variant;
};

USTRUCT()
struct FMetasoundFrontendClassMetadata {
    GENERATED_BODY()

    UPROPERTY()
    FMetasoundFrontendClassName ClassName;
    UPROPERTY()
    FMetasoundFrontendVersionNumber Version;
    UPROPERTY()
    EMetasoundFrontendClassType Type = EMetasoundFrontendClassType::Invalid;
    UPROPERTY()
    bool bIsDeprecated = false;
    UPROPERTY()
    uint16 AccessFlags = static_cast<uint16>(EMetasoundFrontendClassAccessFlags::Referenceable);
    UPROPERTY()
    bool bAutoUpdateManagesInterface = false;
    UPROPERTY()
    FGuid ChangeID;
};

USTRUCT()
struct FMetasoundFrontendClassStyleDisplay {
    GENERATED_BODY()

    UPROPERTY()
    FName ImageName;
    UPROPERTY()
    bool bShowName = true;
    UPROPERTY()
    bool bShowInputNames = true;
    UPROPERTY()
    bool bShowOutputNames = true;
};

USTRUCT()
struct FMetasoundFrontendClassStyle {
    GENERATED_BODY()

    UPROPERTY()
    FMetasoundFrontendClassStyleDisplay Display;
};

USTRUCT()
struct FMetasoundFrontendClass {
    GENERATED_BODY()

    UPROPERTY()
    FGuid ID;
    UPROPERTY()
    FMetasoundFrontendClassMetadata Metadata;
    UPROPERTY()
    FMetasoundFrontendClassInterface Interface;
    UPROPERTY()
    FMetasoundFrontendClassStyle Style;
};

USTRUCT()
struct FMetasoundFrontendGraphClassPresetOptions {
    GENERATED_BODY()

    UPROPERTY()
    bool bIsPreset = false;
    UPROPERTY()
    TSet<FName> InputsInheritingDefault;
};

USTRUCT()
struct FMetasoundFrontendGraphClass : public FMetasoundFrontendClass {
    GENERATED_BODY()

    UPROPERTY()
    FMetasoundFrontendGraph Graph;
    UPROPERTY()
    TArray<FMetasoundFrontendGraph> PagedGraphs;
    UPROPERTY()
    FMetasoundFrontendGraphClassPresetOptions PresetOptions;
};

USTRUCT()
struct FMetasoundFrontendInterfaceUClassOptions {
    GENERATED_BODY()

    UPROPERTY()
    FTopLevelAssetPath ClassPath;
    UPROPERTY()
    bool bIsModifiable = true;
    UPROPERTY()
    bool bIsDefault = false;
};

USTRUCT()
struct FMetasoundFrontendInterfaceMetadata {
    GENERATED_BODY()

    UPROPERTY()
    FMetasoundFrontendVersion Version;
    UPROPERTY()
    TArray<FMetasoundFrontendInterfaceUClassOptions> UClassOptions;
};

USTRUCT()
struct FMetasoundFrontendInterface : public FMetasoundFrontendClassInterface {
    GENERATED_BODY()

    UPROPERTY()
    FMetasoundFrontendVersion Version;
    UPROPERTY()
    TArray<FMetasoundFrontendInterfaceUClassOptions> UClassOptions;
    UPROPERTY()
    FMetasoundFrontendInterfaceMetadata Metadata;
};

USTRUCT()
struct FMetasoundFrontendInterfaceVertexBinding {
    GENERATED_BODY()

    UPROPERTY()
    FName OutputName;
    UPROPERTY()
    FName InputName;
};

USTRUCT()
struct FMetasoundFrontendInterfaceBinding {
    GENERATED_BODY()

    UPROPERTY()
    FMetasoundFrontendVersion OutputInterfaceVersion;
    UPROPERTY()
    FMetasoundFrontendVersion InputInterfaceVersion;
    UPROPERTY()
    int32 BindingPriority = 0;
    UPROPERTY()
    TArray<FMetasoundFrontendInterfaceVertexBinding> VertexBindings;
};

USTRUCT()
struct FMetasoundFrontendDocumentMetadata {
    GENERATED_BODY()

    UPROPERTY()
    FMetasoundFrontendVersion Version;
};

USTRUCT()
struct FMetasoundFrontendDocument {
    GENERATED_BODY()

    UPROPERTY()
    FMetasoundFrontendDocumentMetadata Metadata;
    UPROPERTY()
    TSet<FMetasoundFrontendVersion> Interfaces;
    UPROPERTY()
    FMetasoundFrontendGraphClass RootGraph;
    UPROPERTY()
    TArray<FMetasoundFrontendGraphClass> Subgraphs;
    UPROPERTY()
    TArray<FMetasoundFrontendClass> Dependencies;
    UPROPERTY()
    FMetasoundFrontendVersion ArchetypeVersion;
    UPROPERTY()
    TArray<FMetasoundFrontendVersion> InterfaceVersions;
    UPROPERTY()
    uint32 IdCounter = 1;
    UPROPERTY()
    TInstancedStruct<FMetaSoundFrontendDocumentTemplate> Template;
};

}
