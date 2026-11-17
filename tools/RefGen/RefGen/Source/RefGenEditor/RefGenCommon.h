#pragma once

#if __has_include("CoreMinimal.h")
#include "CoreMinimal.h"
#else
#include "UnrealEd.h" // <4.15
#endif
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 15
#include "Engine.h" // 4.15's first IWYU headers miss some transitive includes (PPF_*, TObjectIterator) under MSVC
#endif
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#if __has_include("AssetRegistry/AssetRegistryModule.h")
#include "AssetRegistry/AssetRegistryModule.h"
#else // 4.25 and older
#include "AssetRegistryModule.h"
#endif
#include "Factories/Factory.h"
#include "UObject/Package.h"
#if __has_include("UObject/SavePackage.h") // 4.26+
#include "UObject/SavePackage.h"
#endif

#define UE_AT_LEAST(Major, Minor) (ENGINE_MAJOR_VERSION > Major || (ENGINE_MAJOR_VERSION == Major && ENGINE_MINOR_VERSION >= Minor))
#include "RefGenSoftRefs.inl"
#include <initializer_list>
#if UE_AT_LEAST(4, 13)
#define RG_PA_BODIES(PA) (PA)->SkeletalBodySetups
#else
#define RG_PA_BODIES(PA) (PA)->BodySetup
#endif
namespace RefGen
{
	// TArray/TSet have no initializer_list support before 4.13; ue4_compat rewrites `X = { ... };` to this
	template <typename ContainerType, typename... ItemTypes>
	void AssignList(ContainerType& Container, ItemTypes&&... Items)
	{
		Container.Empty();
		int Unused[] = { 0, ((void)Container.Add(Items), 0)... };
		(void)Unused;
	}
}
#if !UE_AT_LEAST(4, 14)
using AGameModeBase = AGameMode; // AGameModeBase split out in 4.14
#endif

// ---- 4.14 and older (pre blend space rework / pre FText enum names) ----
#if UE_AT_LEAST(4, 15)
#define RG_BS_ADD_SAMPLE(BS, Anim, V) (BS)->AddSample(Anim, V)
#define RG_ENUM_DISPLAY_NAME(Str) FText::FromString(Str)
#define RG_REQUEST_TAG(Name) FGameplayTag::RequestGameplayTag(Name, false)
#else
#define RG_BS_ADD_SAMPLE(BS, Anim, V) (BS)->AddSample(FBlendSample(Anim, V, true))
#define RG_ENUM_DISPLAY_NAME(Str) FString(Str)
#define RG_REQUEST_TAG(Name) IGameplayTagsModule::RequestGameplayTag(Name, false)
#endif

// ---- engine API differences (accessors that replaced public fields, setters added later) ----
#if UE_AT_LEAST(4, 27)
#define RG_SM_RENDERDATA(SM) (SM)->GetRenderData()
#define RG_SM_BODYSETUP(SM) (SM)->GetBodySetup()
#define RG_SM_SET_LIGHTMAPRES(SM, V) (SM)->SetLightMapResolution(V)
#define RG_SK_SKELETON(SK) (SK)->GetSkeleton()
#define RG_SK_PHYSICSASSET(SK) (SK)->GetPhysicsAsset()
#define RG_SK_MATERIALS(SK) (SK)->GetMaterials()
#define RG_SK_SET_MATERIALS(SK, V) (SK)->SetMaterials(V)
#else
#if UE_AT_LEAST(4, 15)
#define RG_SM_RENDERDATA(SM) (SM)->RenderData.Get()
#else
#define RG_SM_RENDERDATA(SM) (SM)->RenderData.GetOwnedPointer()
#endif
#define RG_SM_BODYSETUP(SM) (SM)->BodySetup
#define RG_SM_SET_LIGHTMAPRES(SM, V) ((SM)->LightMapResolution = (V))
#define RG_SK_SKELETON(SK) (SK)->Skeleton
#define RG_SK_PHYSICSASSET(SK) (SK)->PhysicsAsset
#define RG_SK_MATERIALS(SK) (SK)->Materials
#define RG_SK_SET_MATERIALS(SK, V) ((SK)->Materials = (V))
#endif
#if UE_AT_LEAST(4, 11)
#define RG_REFRESH_CACHE(Anim) (Anim)->RefreshCacheData()
#define RG_SET_FOV(Camera, Fov) (Camera)->SetFieldOfView(Fov)
#else // 4.10 and older: no RefreshCacheData (montage/sequence caches build on load) or SetFieldOfView
#define RG_REFRESH_CACHE(Anim) ((void)0)
#define RG_SET_FOV(Camera, Fov) ((Camera)->FieldOfView = (Fov))
#endif
// ---- 4.8 and older ----
#if UE_AT_LEAST(4, 9)
#define RG_PLAY_LENGTH(Anim) (Anim)->GetPlayLength()
#define RG_SET_WAVE(Player, Wave) (Player)->SetSoundWave(Wave)
#define RG_BS_VALIDATE(BS) (BS)->ValidateSampleData()
#else // public fields, no TBaseStructure, ValidateSampleData is protected
#define RG_PLAY_LENGTH(Anim) (Anim)->SequenceLength
#define RG_SET_WAVE(Player, Wave) ((Player)->SoundWave = (Wave))
#define RG_BS_VALIDATE(BS) (BS)->PostLoad() // ValidateSampleData is protected and not exported; PostLoad runs it
template <typename T> struct TBaseStructure;
inline UScriptStruct* RGCoreStruct(const TCHAR* Name) { return FindObjectChecked<UScriptStruct>(ANY_PACKAGE, Name); } // 4.7 nests them in UObject
template <> struct TBaseStructure<FVector> { static UScriptStruct* Get() { return RGCoreStruct(TEXT("Vector")); } };
template <> struct TBaseStructure<FTransform> { static UScriptStruct* Get() { return RGCoreStruct(TEXT("Transform")); } };
template <> struct TBaseStructure<FLinearColor> { static UScriptStruct* Get() { return RGCoreStruct(TEXT("LinearColor")); } };
#endif
#if UE_AT_LEAST(4, 8)
#define RG_SET_RENDER_TEXT(Comp, Str) (Comp)->SetText(FText::FromString(Str))
#define RG_SET_CHILD_ACTOR_CLASS(Comp, Class) (Comp)->SetChildActorClass(Class)
#define RG_SOUND_GRAPH_NODE(Node) (Node)->GetGraphNode()
#define RG_SET_EVENT_REFERENCE(Node, Name, Class) (Node)->EventReference.SetExternalMember(Name, Class)
#else // 4.7 and older: FString render text, public fields, event signature fields, NewObject without names
#define RG_SET_RENDER_TEXT(Comp, Str) (Comp)->SetText(FString(Str))
#define RG_SET_CHILD_ACTOR_CLASS(Comp, Class) ((Comp)->ChildActorClass = (Class))
#define RG_SOUND_GRAPH_NODE(Node) (Node)->GraphNode
#define RG_SET_EVENT_REFERENCE(Node, Name, Class) ((Node)->EventSignatureName = (Name), (Node)->EventSignatureClass = (Class))
template <class T> T* NewObject(UObject* Outer, UClass* Class, FName Name, EObjectFlags Flags = RF_NoFlags)
{
	return ConstructObject<T>(Class, Outer, Name, Flags);
}
template <class T> T* NewObject(UObject* Outer, FName Name, EObjectFlags Flags = RF_NoFlags)
{
	return ConstructObject<T>(T::StaticClass(), Outer, Name, Flags);
}
#endif
// ---- 4.3 ----
#if !UE_AT_LEAST(4, 4) // no linear sampler types yet: the plain ones sample linear textures too
#define SAMPLERTYPE_LinearColor SAMPLERTYPE_Color
#define SAMPLERTYPE_LinearGrayscale SAMPLERTYPE_Grayscale
#define RG_FBX_IMPORT_UI_CLASS() /* UFbxImportUI isn't exported on 4.3 */ \
	LoadClass<UObject>(nullptr, TEXT("/Script/UnrealEd.FbxImportUI"), nullptr, LOAD_None, nullptr)
#define RG_NEW_FBX_IMPORT_UI() static_cast<UFbxImportUI*>(NewObject<UObject>(GetTransientPackage(), RG_FBX_IMPORT_UI_CLASS()))
#define RG_AS_FBX_IMPORT_UI(Object) ((Object) && (Object)->IsA(RG_FBX_IMPORT_UI_CLASS()) ? static_cast<UFbxImportUI*>(Object) : nullptr)
#else
#define RG_NEW_FBX_IMPORT_UI() NewObject<UFbxImportUI>()
#define RG_AS_FBX_IMPORT_UI(Object) Cast<UFbxImportUI>(Object)
#endif
// ---- 4.4 ----
#if UE_AT_LEAST(4, 5)
#define RG_SET_STREAMING_PACKAGE(Streaming, Name) (Streaming)->SetWorldAssetByPackageName(Name)
#define RG_SET_STREAMING_COLOR(Streaming, Color) ((Streaming)->LevelColor = (Color))
#define RG_RICH_CURVE(Curve) (Curve).GetRichCurve()
#else // streaming levels name their package directly and draw with an FColor
#define RG_SET_STREAMING_PACKAGE(Streaming, Name) ((Streaming)->PackageName = FName(Name))
#define RG_SET_STREAMING_COLOR(Streaming, Color) ((Streaming)->DrawColor = (Color).ToFColor(true))
#define RG_RICH_CURVE(Curve) (&(Curve).EditorCurveData) // GetRichCurve isn't exported
#endif
// ---- 4.5 ----
#if UE_AT_LEAST(4, 6)
#define RG_CHAR_MESH(C) (C)->GetMesh()
#define RG_CHAR_MOVEMENT(C) (C)->GetCharacterMovement()
#define RG_FIND_PROPERTY(Class, Name) (Class)->FindPropertyByName(Name)
#else // character components are public TSubobjectPtr members; no UStruct::FindPropertyByName
#define RG_CHAR_MESH(C) (C)->Mesh
#define RG_CHAR_MOVEMENT(C) (C)->CharacterMovement
#define RG_FIND_PROPERTY(Class, Name) FindField<UProperty>(Class, Name)
#endif
#if UE_AT_LEAST(4, 6)
#define RG_NOTIFY_LINK(Event, Asset, Time) (Event).Link(Asset, Time)
#define RG_NOTIFY_DURATION(Event, Asset, Time, Length) ((Event).SetDuration(Length), (Event).EndLink.Link(Asset, (Time) + (Length)))
#define RG_SET_PREVIEW_MESH(Anim, Mesh) (Anim)->SetPreviewMesh(Mesh)
#else // notifies are plain times, no per-animation preview mesh, no TFunctionRef
#define RG_NOTIFY_LINK(Event, Asset, Time) ((Event).DisplayTime = (Time))
#define RG_NOTIFY_DURATION(Event, Asset, Time, Length) ((Event).Duration = (Length))
#define RG_SET_PREVIEW_MESH(Anim, Mesh) ((void)0)
#include <functional>
template <typename FuncType> using TFunctionRef = std::function<FuncType>;
#endif
// ---- 4.6 ----
#if UE_AT_LEAST(4, 7)
#define RG_INIT_STRUCT(Struct, Data) (Struct)->InitializeStruct(Data)
#define RG_ADD_INSTANCE_COMPONENT(Actor, Component) (Actor)->AddInstanceComponent(Component)
#define RG_NEW_FBX_FACTORY() NewObject<UFbxFactory>()
#else // no hierarchical instanced meshes yet: same data on the plain instanced component
#define RG_INIT_STRUCT(Struct, Data) (Struct)->InitializeScriptStruct(Data)
#define RG_ADD_INSTANCE_COMPONENT(Actor, Component) ((void)0) // no instance component list: saved as owned subobjects
#define RG_NEW_FBX_FACTORY() /* not exported on 4.6 */ \
	static_cast<UFbxFactory*>(NewObject<UObject>(GetTransientPackage(), LoadClass<UObject>(nullptr, TEXT("/Script/UnrealEd.FbxFactory"), nullptr, LOAD_None, nullptr)))
using UHierarchicalInstancedStaticMeshComponent = UInstancedStaticMeshComponent;
template <typename T> using TInlineComponentArray = TArray<T>;
#endif
// GameplayTags/Abilities: 4.7 and older installed builds can't run UHT on a project depending on GameplayTags
// (AIModule includes its headers but is parsed first). ue4_compat.py strips the reflected members there.
#define RG_GAMEPLAY UE_AT_LEAST(4, 8)
// Level Sequences: the 4.10 LevelSequence module is experimental with a different API, and 4.9 has none
#define RG_LEVEL_SEQUENCES UE_AT_LEAST(4, 11)
#if UE_AT_LEAST(5, 0)
#define RG_SET_SEQUENCE(Node, Seq) (Node).SetSequence(Seq)
#define RG_SET_BLENDSPACE(Node, BS) (Node).SetBlendSpace(BS)
#else
#define RG_SET_SEQUENCE(Node, Seq) ((Node).Sequence = (Seq))
#define RG_SET_BLENDSPACE(Node, BS) ((Node).BlendSpace = (BS))
#endif
#if UE_AT_LEAST(5, 1)
#define RG_SET_SEGMENT_ANIM(Segment, Anim) (Segment).SetAnimReference((Anim), true)
#else
#define RG_SET_SEGMENT_ANIM(Segment, Anim) ((Segment).AnimReference = (Anim), (Segment).AnimStartTime = 0.f, (Segment).AnimEndTime = RG_PLAY_LENGTH(Anim))
#endif
// 4.24 and older: UProperty instead of FProperty (FField), ARRAY_COUNT, protected sound class, timeline names
#if !UE_AT_LEAST(4, 25)
#include "UObject/UnrealType.h"
using FProperty = UProperty;
using FStructProperty = UStructProperty;
using FObjectProperty = UObjectProperty;
using FArrayProperty = UArrayProperty;
using FMulticastDelegateProperty = UMulticastDelegateProperty;
template <typename T> T* FindFProperty(const UStruct* Owner, FName Name) { return FindField<T>(Owner, Name); }
#endif
#ifndef UE_ARRAY_COUNT
#define UE_ARRAY_COUNT ARRAY_COUNT
#endif
// Cut-offs below were checked against each release's headers.
#if UE_AT_LEAST(4, 26)
#define RG_SET_SOUNDCLASS(Sound, Class) ((Sound)->SoundClassObject = (Class))
#else // protected in some releases: go through reflection
#define RG_SET_SOUNDCLASS(Sound, Class) FindField<UObjectProperty>(USoundBase::StaticClass(), TEXT("SoundClassObject"))->SetObjectPropertyValue_InContainer(Sound, Class)
#endif
#if UE_AT_LEAST(4, 22)
#define RG_SET_TRACK_NAME(Track, Name, Timeline) (Track).SetTrackName(Name, Timeline)
#define RG_TIMELINE_VARIABLE(Timeline) (Timeline)->GetVariableName()
#else
#define RG_SET_TRACK_NAME(Track, Name, Timeline) ((Track).TrackName = (Name))
#define RG_TIMELINE_VARIABLE(Timeline) FName(*UTimelineTemplate::TimelineTemplateNameToVariableName((Timeline)->GetFName()))
#endif
#if UE_AT_LEAST(4, 23)
#define RG_SM_ADD_SOCKET(Mesh, Socket) (Mesh)->AddSocket(Socket)
#define RG_SM_SOURCE_MODEL(Mesh, Lod) (Mesh)->GetSourceModel(Lod)
#else
#define RG_SM_ADD_SOCKET(Mesh, Socket) (Mesh)->Sockets.Add(Socket)
#define RG_SM_SOURCE_MODEL(Mesh, Lod) (Mesh)->SourceModels[Lod]
#endif
#if UE_AT_LEAST(4, 20)
#define RG_SM_SET_MATERIAL(Mesh, Index, Material) (Mesh)->SetMaterial(Index, Material)
#define RG_SM_SET_NUM_LODS(Mesh, Num) (Mesh)->SetNumSourceModels(Num)
#define RG_SCREEN_SIZE(Model) (Model).ScreenSize.Default
#elif UE_AT_LEAST(4, 14) // 4.19: public arrays, per-platform screen size came in 4.20
#define RG_SM_SET_MATERIAL(Mesh, Index, Material) \
	((Mesh)->StaticMaterials.IsValidIndex(Index) ? (void)((Mesh)->StaticMaterials[Index].MaterialInterface = (Material)) : (void)0)
#define RG_SM_SET_NUM_LODS(Mesh, Num) (Mesh)->SourceModels.SetNum(Num)
#define RG_SCREEN_SIZE(Model) (Model).ScreenSize
#else // 4.13: plain material array (FStaticMaterial is 4.14+)
#define RG_SM_SET_MATERIAL(Mesh, Index, Material) \
	((Mesh)->Materials.IsValidIndex(Index) ? (void)((Mesh)->Materials[Index] = (Material)) : (void)0)
#define RG_SM_SET_NUM_LODS(Mesh, Num) (Mesh)->SourceModels.SetNum(Num)
#define RG_SCREEN_SIZE(Model) (Model).ScreenSize
#endif
#if UE_AT_LEAST(4, 19)
#define RG_MPARAM(Name) FMaterialParameterInfo(Name)
#define RG_SWITCH_NAME(Switch) (Switch).ParameterInfo.Name
#else // 4.18: material parameters are addressed by plain name
#define RG_MPARAM(Name) FName(Name)
#define RG_SWITCH_NAME(Switch) (Switch).ParameterName
#endif
#if UE_AT_LEAST(4, 19)
#define RG_PHYS_CREATE_CONSTRAINTS bCreateConstraints
#else
#define RG_PHYS_CREATE_CONSTRAINTS bCreateJoints
#endif
#if UE_AT_LEAST(4, 19)
using FRGPinName = FName;
#define RG_PIN_STR(Name) (Name).ToString()
#else // 4.18: pin names and categories are strings
using FRGPinName = FString;
#define RG_PIN_STR(Name) (Name)
#endif

#if UE_AT_LEAST(4, 16)
#define RG_ENUM_NAME_BY_INDEX GetNameStringByIndex
#else
#define RG_ENUM_NAME_BY_INDEX GetEnumName // short name, same as GetNameStringByIndex
#endif
#if UE_AT_LEAST(4, 17)
#define RG_SET_CONTAINER(PinType, Container) ((PinType).ContainerType = (Container))
#define RG_RT_SET_RGBA16F(RT) ((RT)->RenderTargetFormat = RTF_RGBA16f)
#define RG_SET_MATERIAL_FUNCTION(Call, Function) (Call)->SetMaterialFunction(Function)
#define RG_IMPORT_DATA(Data) (Data)
#else // 4.16: container flags on the pin type, bHDR render targets, 3-argument SetMaterialFunction
enum class EPinContainerType : uint8 { None, Array, Set, Map };
#if UE_AT_LEAST(4, 14)
#define RG_SET_CONTAINER(PinType, Container) \
	((PinType).bIsArray = (Container) == EPinContainerType::Array, (PinType).bIsSet = (Container) == EPinContainerType::Set, \
	 (PinType).bIsMap = (Container) == EPinContainerType::Map)
#else // 4.13: arrays only (Blueprint sets/maps and FEdGraphTerminalType are 4.14+)
#define RG_SET_CONTAINER(PinType, Container) ((PinType).bIsArray = (Container) == EPinContainerType::Array)
struct FEdGraphTerminalType { FString TerminalCategory; FString TerminalSubCategory; };
#endif
#define RG_RT_SET_RGBA16F(RT) ((RT)->bHDR = true) // PF_FloatRGBA, same as RTF_RGBA16f
#define RG_SET_MATERIAL_FUNCTION(Call, Function) (Call)->SetMaterialFunction(nullptr, nullptr, Function)
#define RG_IMPORT_DATA(Data) (*(Data))
#endif

namespace RefGen
{
	// TArray::AddDefaulted_GetRef only exists from 4.19 on, AddDefaulted from 4.8 on
	template <typename T, typename Allocator>
	T& AddDefaulted(TArray<T, Allocator>& Array)
	{
#if UE_AT_LEAST(4, 8)
		return Array[Array.AddDefaulted()];
#else
		return Array[Array.Add(T())];
#endif
	}
}
#define RG_HAS_COMPRESSION_SETTINGS __has_include("Animation/AnimBoneCompressionSettings.h") // 4.25+

// UE4 blend spaces (1D, 2D, aim offsets) share UBlendSpaceBase; UE5 derives them all from UBlendSpace
#if UE_AT_LEAST(5, 0)
class UBlendSpace;
using UBlendSpaceAny = UBlendSpace;
#else
class UBlendSpaceBase;
using UBlendSpaceAny = UBlendSpaceBase;
#endif

class UAnimBlueprint;
class UAnimComposite;
class UAnimMontage;
class UAnimSequence;
class UBlendSpace;
class UBlueprint;
class UCurveFloat;
class UCurveLinearColor;
class UCurveTable;
class UCurveVector;
class UDataTable;
class ULevelSequence;
class UMaterial;
class UMaterialFunction;
class UMaterialInstanceConstant;
class UMaterialParameterCollection;
class UParticleSystem;
class UPhysicalMaterial;
class UPhysicsAsset;
class UPoseAsset;
class URefDataAsset;
class USkeletalMesh;
class USkeleton;
class USoundAttenuation;
class USoundClass;
class USoundCue;
class USoundWave;
class UStaticMesh;
class UStringTable;
class UTexture2D;
class UTextureCube;
class UTextureLightProfile;
class UTextureRenderTarget2D;
class UUserDefinedEnum;
class UUserDefinedStruct;
class UAnimBoneCompressionSettings;

DECLARE_LOG_CATEGORY_EXTERN(LogRefGen, Log, All);

namespace RefGen
{
	struct FAssets;

	IAssetTools& Tools();
	UObject* Create(const FString& Name, const FString& Path, UClass* Class, UFactory* Factory);
	template <typename T>
	T* Create(const FString& Name, const FString& Path, UFactory* Factory)
	{
		return Cast<T>(Create(Name, Path, T::StaticClass(), Factory));
	}
	UObject* NewAsset(const FString& Name, const FString& Path, UClass* Class);
	template <typename T>
	T* NewAsset(const FString& Name, const FString& Path)
	{
		return Cast<T>(NewAsset(Name, Path, T::StaticClass()));
	}
	void Touch(UObject* Asset);
	TArray<UObject*> ImportFile(const FString& File, const FString& Path, const FString& Name, UObject* Options);
	bool SetPropertyText(UObject* Object, const TCHAR* PropertyName, const TCHAR* Value);
	void Fail(const FString& Message);
	int32 Failures();

	void MakeTextures(FAssets& A);
	void MakeMaterials(FAssets& A);
	void MakeMeshes(FAssets& A, const FString& SourceDir);
	void MakeAnimation(FAssets& A, const FString& SourceDir);
	void MakeAnimBlueprint(FAssets& A);
	void MakeData(FAssets& A);
	void MakeAudio(FAssets& A, const FString& SourceDir);
	void MakeParticles(FAssets& A);
	void MakeBlueprints(FAssets& A);
	void MakeSequence(FAssets& A);
	void MakeMaps(FAssets& A);
}

struct RefGen::FAssets
{
	UTexture2D* Albedo = nullptr;
	UTexture2D* Normal = nullptr;
	UTexture2D* Mask = nullptr;
	UTexture2D* Hdr = nullptr;
	UTexture2D* SubUV = nullptr;
	UTextureCube* Cube = nullptr;
	UTextureLightProfile* LightProfile = nullptr;
	UTextureRenderTarget2D* RenderTarget = nullptr;
	UMaterialParameterCollection* Collection = nullptr;
	UMaterialFunction* Function = nullptr;
	UMaterial* Opaque = nullptr;
	UMaterial* Masked = nullptr;
	UMaterial* Translucent = nullptr;
	UMaterial* Unlit = nullptr;
	UMaterial* Decal = nullptr;
	UMaterial* PostProcess = nullptr;
	UMaterial* LightFunction = nullptr;
	UMaterial* Particle = nullptr;
	UMaterialInstanceConstant* InstanceRed = nullptr;
	UMaterialInstanceConstant* InstanceBlue = nullptr;
	UMaterialInstanceConstant* InstanceChild = nullptr;
	UPhysicalMaterial* PhysMaterial = nullptr;
	UStaticMesh* StaticMesh = nullptr;
	UStaticMesh* NaniteMesh = nullptr;
	USkeletalMesh* SkeletalMesh = nullptr;
	USkeleton* Skeleton = nullptr;
	UPhysicsAsset* PhysicsAsset = nullptr;
	UAnimSequence* Bend = nullptr;
	UAnimSequence* Twist = nullptr;
	UAnimSequence* BendAdditive = nullptr;
	UAnimSequence* AimCenter = nullptr;
	UAnimSequence* AimLeft = nullptr;
	UAnimSequence* AimRight = nullptr;
	UAnimMontage* Montage = nullptr;
	UAnimComposite* Composite = nullptr;
	UBlendSpaceAny* BlendSpace2D = nullptr;
	UBlendSpaceAny* BlendSpace1D = nullptr;
	UBlendSpaceAny* AimOffset = nullptr;
	UPoseAsset* PoseAsset = nullptr;
	UAnimBlueprint* AnimBlueprint = nullptr;
	UAnimBlueprint* AnimBlueprintChild = nullptr;
	UCurveFloat* CurveFloat = nullptr;
	UCurveLinearColor* CurveColor = nullptr;
	UCurveVector* CurveVector = nullptr;
	UUserDefinedStruct* Struct = nullptr;
	UUserDefinedEnum* Enum = nullptr;
	UDataTable* Table = nullptr;
	UDataTable* StructTable = nullptr;
	UCurveTable* CurveTable = nullptr;
	UStringTable* StringTable = nullptr;
	URefDataAsset* DataAsset = nullptr;
	USoundWave* Wave = nullptr;
	USoundCue* Cue = nullptr;
	USoundAttenuation* Attenuation = nullptr;
	USoundClass* SoundClass = nullptr;
	UParticleSystem* Particles = nullptr;
	UBlueprint* Interface = nullptr;
	UBlueprint* Actor = nullptr;
	UBlueprint* ActorChild = nullptr;
	UBlueprint* Simple = nullptr;
	UBlueprint* Character = nullptr;
	UBlueprint* GameMode = nullptr;
	UBlueprint* Controller = nullptr;
	UBlueprint* Ability = nullptr;
	UBlueprint* Effect = nullptr;
	UBlueprint* FunctionLibrary = nullptr;
	UBlueprint* MacroLibrary = nullptr;
	UBlueprint* Notify = nullptr;
	ULevelSequence* Sequence = nullptr;
};
