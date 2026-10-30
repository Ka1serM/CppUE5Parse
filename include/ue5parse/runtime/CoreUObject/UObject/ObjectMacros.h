#pragma once

#include "ue5parse/runtime/Core/Containers/Array.h"

namespace ue5parse::runtime {

// The flag values are UE's, so that cooked flags can be used as they are.

enum EObjectFlags : uint32 {
    RF_NoFlags = 0x00000000,
    RF_Public = 0x00000001,
    RF_Standalone = 0x00000002,
    RF_MarkAsNative = 0x00000004,
    RF_Transactional = 0x00000008,
    RF_ClassDefaultObject = 0x00000010,
    RF_ArchetypeObject = 0x00000020,
    RF_Transient = 0x00000040,
    RF_MarkAsRootSet = 0x00000080,
    RF_TagGarbageTemp = 0x00000100,
    RF_NeedInitialization = 0x00000200,
    RF_NeedLoad = 0x00000400,
    RF_KeepForCooker = 0x00000800,
    RF_NeedPostLoad = 0x00001000,
    RF_NeedPostLoadSubobjects = 0x00002000,
    RF_NewerVersionExists = 0x00004000,
    RF_BeginDestroyed = 0x00008000,
    RF_FinishDestroyed = 0x00010000,
    RF_BeingRegenerated = 0x00020000,
    RF_DefaultSubObject = 0x00040000,
    RF_WasLoaded = 0x00080000,
    RF_TextExportTransient = 0x00100000,
    RF_LoadCompleted = 0x00200000,
    RF_InheritableComponentTemplate = 0x00400000,
    RF_DuplicateTransient = 0x00800000,
    RF_StrongRefOnFrame = 0x01000000,
    RF_NonPIEDuplicateTransient = 0x02000000,
    RF_WillBeLoaded = 0x08000000,
    RF_HasExternalPackage = 0x10000000,
    // UE keeps garbage as an internal flag; the runtime keeps it here.
    RF_Garbage = 0x40000000,
};
constexpr inline EObjectFlags operator|(EObjectFlags A, EObjectFlags B) { return EObjectFlags(uint32(A) | uint32(B)); }
constexpr inline EObjectFlags operator&(EObjectFlags A, EObjectFlags B) { return EObjectFlags(uint32(A) & uint32(B)); }
constexpr inline EObjectFlags operator~(EObjectFlags A) { return EObjectFlags(~uint32(A)); }

// Flags of the objects that load from packages and their templates.
constexpr EObjectFlags RF_PropagateToSubObjects = EObjectFlags(RF_Public | RF_ArchetypeObject | RF_Transactional | RF_Transient);

enum EClassFlags : uint32 {
    CLASS_None = 0x00000000u,
    CLASS_Abstract = 0x00000001u,
    CLASS_DefaultConfig = 0x00000002u,
    CLASS_Config = 0x00000004u,
    CLASS_Transient = 0x00000008u,
    CLASS_Optional = 0x00000010u,
    CLASS_MatchedSerializers = 0x00000020u,
    CLASS_ProjectUserConfig = 0x00000040u,
    CLASS_Native = 0x00000080u,
    CLASS_NotPlaceable = 0x00000200u,
    CLASS_PerObjectConfig = 0x00000400u,
    CLASS_ReplicationDataIsSetUp = 0x00000800u,
    CLASS_EditInlineNew = 0x00001000u,
    CLASS_CollapseCategories = 0x00002000u,
    CLASS_Interface = 0x00004000u,
    CLASS_Const = 0x00010000u,
    CLASS_NeedsDeferredDependencyLoading = 0x00020000u,
    CLASS_CompiledFromBlueprint = 0x00040000u,
    CLASS_MinimalAPI = 0x00080000u,
    CLASS_RequiredAPI = 0x00100000u,
    CLASS_DefaultToInstanced = 0x00200000u,
    CLASS_TokenStreamAssembled = 0x00400000u,
    CLASS_HasInstancedReference = 0x00800000u,
    CLASS_Hidden = 0x01000000u,
    CLASS_Deprecated = 0x02000000u,
    CLASS_HideDropDown = 0x04000000u,
    CLASS_GlobalUserConfig = 0x08000000u,
    CLASS_Intrinsic = 0x10000000u,
    CLASS_Constructed = 0x20000000u,
    CLASS_ConfigDoNotCheckDefaults = 0x40000000u,
    CLASS_NewerVersionExists = 0x80000000u,
};

enum EClassCastFlags : uint64 {
    CASTCLASS_None = 0x0000000000000000,
    CASTCLASS_UField = 0x0000000000000001,
    CASTCLASS_FInt8Property = 0x0000000000000002,
    CASTCLASS_UEnum = 0x0000000000000004,
    CASTCLASS_UStruct = 0x0000000000000008,
    CASTCLASS_UScriptStruct = 0x0000000000000010,
    CASTCLASS_UClass = 0x0000000000000020,
    CASTCLASS_FByteProperty = 0x0000000000000040,
    CASTCLASS_FIntProperty = 0x0000000000000080,
    CASTCLASS_FFloatProperty = 0x0000000000000100,
    CASTCLASS_FUInt64Property = 0x0000000000000200,
    CASTCLASS_FClassProperty = 0x0000000000000400,
    CASTCLASS_FUInt32Property = 0x0000000000000800,
    CASTCLASS_FInterfaceProperty = 0x0000000000001000,
    CASTCLASS_FNameProperty = 0x0000000000002000,
    CASTCLASS_FStrProperty = 0x0000000000004000,
    CASTCLASS_FProperty = 0x0000000000008000,
    CASTCLASS_FObjectProperty = 0x0000000000010000,
    CASTCLASS_FBoolProperty = 0x0000000000020000,
    CASTCLASS_FUInt16Property = 0x0000000000040000,
    CASTCLASS_UFunction = 0x0000000000080000,
    CASTCLASS_FStructProperty = 0x0000000000100000,
    CASTCLASS_FArrayProperty = 0x0000000000200000,
    CASTCLASS_FInt64Property = 0x0000000000400000,
    CASTCLASS_FDelegateProperty = 0x0000000000800000,
    CASTCLASS_FNumericProperty = 0x0000000001000000,
    CASTCLASS_FMulticastDelegateProperty = 0x0000000002000000,
    CASTCLASS_FObjectPropertyBase = 0x0000000004000000,
    CASTCLASS_FWeakObjectProperty = 0x0000000008000000,
    CASTCLASS_FLazyObjectProperty = 0x0000000010000000,
    CASTCLASS_FSoftObjectProperty = 0x0000000020000000,
    CASTCLASS_FTextProperty = 0x0000000040000000,
    CASTCLASS_FInt16Property = 0x0000000080000000,
    CASTCLASS_FDoubleProperty = 0x0000000100000000,
    CASTCLASS_FSoftClassProperty = 0x0000000200000000,
    CASTCLASS_UPackage = 0x0000000400000000,
    CASTCLASS_ULevel = 0x0000000800000000,
    CASTCLASS_AActor = 0x0000001000000000,
    CASTCLASS_APlayerController = 0x0000002000000000,
    CASTCLASS_APawn = 0x0000004000000000,
    CASTCLASS_USceneComponent = 0x0000008000000000,
    CASTCLASS_UPrimitiveComponent = 0x0000010000000000,
    CASTCLASS_USkinnedMeshComponent = 0x0000020000000000,
    CASTCLASS_USkeletalMeshComponent = 0x0000040000000000,
    CASTCLASS_UBlueprint = 0x0000080000000000,
    CASTCLASS_UDelegateFunction = 0x0000100000000000,
    CASTCLASS_UStaticMeshComponent = 0x0000200000000000,
    CASTCLASS_FMapProperty = 0x0000400000000000,
    CASTCLASS_FSetProperty = 0x0000800000000000,
    CASTCLASS_FEnumProperty = 0x0001000000000000,
    CASTCLASS_USparseDelegateFunction = 0x0002000000000000,
    CASTCLASS_FMulticastInlineDelegateProperty = 0x0004000000000000,
    CASTCLASS_FMulticastSparseDelegateProperty = 0x0008000000000000,
    CASTCLASS_FFieldPathProperty = 0x0010000000000000,
    CASTCLASS_FObjectPtrProperty = 0x0020000000000000,
    CASTCLASS_FClassPtrProperty = 0x0040000000000000,
    CASTCLASS_FLargeWorldCoordinatesRealProperty = 0x0080000000000000,
    CASTCLASS_FOptionalProperty = 0x0100000000000000,
};

enum EPropertyFlags : uint64 {
    CPF_None = 0,
    CPF_Edit = 0x0000000000000001,
    CPF_ConstParm = 0x0000000000000002,
    CPF_BlueprintVisible = 0x0000000000000004,
    CPF_ExportObject = 0x0000000000000008,
    CPF_BlueprintReadOnly = 0x0000000000000010,
    CPF_Net = 0x0000000000000020,
    CPF_EditFixedSize = 0x0000000000000040,
    CPF_Parm = 0x0000000000000080,
    CPF_OutParm = 0x0000000000000100,
    CPF_ZeroConstructor = 0x0000000000000200,
    CPF_ReturnParm = 0x0000000000000400,
    CPF_DisableEditOnTemplate = 0x0000000000000800,
    CPF_NonNullable = 0x0000000000001000,
    CPF_Transient = 0x0000000000002000,
    CPF_Config = 0x0000000000004000,
    CPF_RequiredParm = 0x0000000000008000,
    CPF_DisableEditOnInstance = 0x0000000000010000,
    CPF_EditConst = 0x0000000000020000,
    CPF_GlobalConfig = 0x0000000000040000,
    CPF_InstancedReference = 0x0000000000080000,
    CPF_DuplicateTransient = 0x0000000000200000,
    CPF_SaveGame = 0x0000000001000000,
    CPF_NoClear = 0x0000000002000000,
    CPF_ReferenceParm = 0x0000000008000000,
    CPF_BlueprintAssignable = 0x0000000010000000,
    CPF_Deprecated = 0x0000000020000000,
    CPF_IsPlainOldData = 0x0000000040000000,
    CPF_RepSkip = 0x0000000080000000,
    CPF_RepNotify = 0x0000000100000000,
    CPF_Interp = 0x0000000200000000,
    CPF_NonTransactional = 0x0000000400000000,
    CPF_EditorOnly = 0x0000000800000000,
    CPF_NoDestructor = 0x0000001000000000,
    CPF_AutoWeak = 0x0000004000000000,
    CPF_ContainsInstancedReference = 0x0000008000000000,
    CPF_AssetRegistrySearchable = 0x0000010000000000,
    CPF_SimpleDisplay = 0x0000020000000000,
    CPF_AdvancedDisplay = 0x0000040000000000,
    CPF_Protected = 0x0000080000000000,
    CPF_BlueprintCallable = 0x0000100000000000,
    CPF_BlueprintAuthorityOnly = 0x0000200000000000,
    CPF_TextExportTransient = 0x0000400000000000,
    CPF_NonPIEDuplicateTransient = 0x0000800000000000,
    CPF_ExposeOnSpawn = 0x0001000000000000,
    CPF_PersistentInstance = 0x0002000000000000,
    CPF_UObjectWrapper = 0x0004000000000000,
    CPF_HasGetValueTypeHash = 0x0008000000000000,
    CPF_NativeAccessSpecifierPublic = 0x0010000000000000,
    CPF_NativeAccessSpecifierProtected = 0x0020000000000000,
    CPF_NativeAccessSpecifierPrivate = 0x0040000000000000,
    CPF_SkipSerialization = 0x0080000000000000,
};
constexpr inline EPropertyFlags operator|(EPropertyFlags A, EPropertyFlags B) { return EPropertyFlags(uint64(A) | uint64(B)); }
constexpr inline EPropertyFlags operator&(EPropertyFlags A, EPropertyFlags B) { return EPropertyFlags(uint64(A) & uint64(B)); }
constexpr inline EPropertyFlags operator~(EPropertyFlags A) { return EPropertyFlags(~uint64(A)); }

constexpr EPropertyFlags CPF_ComputedFlags =
    EPropertyFlags(CPF_IsPlainOldData | CPF_NoDestructor | CPF_ZeroConstructor | CPF_HasGetValueTypeHash);

enum EStructFlags : uint32 {
    STRUCT_NoFlags = 0x00000000,
    STRUCT_Native = 0x00000001,
    STRUCT_IdenticalNative = 0x00000002,
    STRUCT_HasInstancedReference = 0x00000004,
    STRUCT_NoExport = 0x00000008,
    STRUCT_Atomic = 0x00000010,
    STRUCT_Immutable = 0x00000020,
    STRUCT_AddStructReferencedObjects = 0x00000040,
    STRUCT_RequiredAPI = 0x00000200,
    STRUCT_NetSerializeNative = 0x00000400,
    STRUCT_SerializeNative = 0x00000800,
    STRUCT_CopyNative = 0x00001000,
    STRUCT_IsPlainOldData = 0x00002000,
    STRUCT_NoDestructor = 0x00004000,
    STRUCT_ZeroConstructor = 0x00008000,
    STRUCT_ExportTextItemNative = 0x00010000,
    STRUCT_ImportTextItemNative = 0x00020000,
    STRUCT_PostSerializeNative = 0x00040000,
    STRUCT_SerializeFromMismatchedTag = 0x00080000,
    STRUCT_NetDeltaSerializeNative = 0x00100000,
    STRUCT_PostScriptConstruct = 0x00200000,
    STRUCT_NetSharedSerialization = 0x00400000,
};

}

// The reflection markers, as UE's: they expand to nothing, tools/UnrealHeaderTool reads them. GENERATED_BODY() expands
// to the macro of its line in the header's Foo.generated.h (the header's last include), which sets CURRENT_FILE_ID.
#ifndef UCLASS
#define UCLASS(...)
#define USTRUCT(...)
#define UENUM(...)
#define UINTERFACE(...)
#define UPROPERTY(...)
#define UFUNCTION(...)
#define UDELEGATE(...)
#define UPARAM(...)
#define UMETA(...)

#define BODY_MACRO_COMBINE_INNER(A, B, C, D) A##B##C##D
#define BODY_MACRO_COMBINE(A, B, C, D) BODY_MACRO_COMBINE_INNER(A, B, C, D)
#define GENERATED_BODY(...) BODY_MACRO_COMBINE(CURRENT_FILE_ID, _, __LINE__, _GENERATED_BODY);
#define GENERATED_USTRUCT_BODY(...) GENERATED_BODY()
#endif
