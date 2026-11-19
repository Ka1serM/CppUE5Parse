#include "RefGenCommon.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/SkeletalMesh.h"
#if __has_include("Engine/SkinnedAssetCommon.h")
#include "Engine/SkinnedAssetCommon.h" // FSkeletalMaterial (not pulled in transitively on 5.1-5.4)
#endif
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/TextureLightProfile.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendProfile.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsAssetUtils.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/Material.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/FbxAnimSequenceImportData.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Animation/AnimNotifies/AnimNotify_PlaySound.h"
#include "Animation/AnimNotifies/AnimNotify_PlayParticleEffect.h"
#include "Animation/AnimNotifies/AnimNotifyState_TimedParticleEffect.h"
#include "Animation/AnimNotifies/AnimNotifyState_Trail.h"
#if RG_HAS_COMPRESSION_SETTINGS
#include "Animation/AnimBoneCompressionSettings.h"
#include "Animation/AnimBoneCompressionCodec.h"
#else
#include "Animation/AnimCompress.h"
#endif
#include "Animation/AnimSequenceBase.h"

namespace
{
	using namespace RefGen;

	template <typename T>
	T* FirstOf(const TArray<UObject*>& Objects)
	{
		for (UObject* Object : Objects)
			if (T* Typed = Cast<T>(Object))
				return Typed;
		return nullptr;
	}

	template <typename T>
	T* FindIn(const FString& Path)
	{
		IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		TArray<FAssetData> Found;
		Registry.GetAssetsByPath(FName(*Path), Found, true);
		for (const FAssetData& Data : Found)
			if (T* Typed = Cast<T>(Data.GetAsset()))
				return Typed;
		return nullptr;
	}

	UFbxImportUI* BaseOptions()
	{
		UFbxImportUI* Options = RG_NEW_FBX_IMPORT_UI();
#if UE_AT_LEAST(4, 15)
		Options->bAutomatedImportShouldDetectType = false;
#endif
		Options->bImportMaterials = false;
		Options->bImportTextures = false;
		Options->bImportAnimations = false;
		Options->bCreatePhysicsAsset = false;
		return Options;
	}
}

void RefGen::MakeMeshes(FAssets& A, const FString& SourceDir)
{
	{
		UFbxImportUI* Options = BaseOptions();
		Options->MeshTypeToImport = FBXIT_StaticMesh;
		Options->bImportMesh = true;
		Options->bImportAsSkeletal = false;
#if UE_AT_LEAST(4, 15)
		Options->StaticMeshImportData->bCombineMeshes = true;
#else
		Options->bCombineMeshes = true;
#endif
		Options->StaticMeshImportData->bAutoGenerateCollision = false;
		Options->StaticMeshImportData->bGenerateLightmapUVs = true;
#if UE_AT_LEAST(4, 7) // 4.7+
		Options->StaticMeshImportData->VertexColorImportOption = EVertexColorImportOption::Replace;
#endif
		A.StaticMesh = FirstOf<UStaticMesh>(ImportFile(SourceDir / TEXT("SM_Ref.fbx"), TEXT("/Game/Meshes"), TEXT("SM_Ref"), Options));
	}
	if (UStaticMesh* SM = A.StaticMesh)
	{
		RG_SM_SET_MATERIAL(SM, 0, A.InstanceRed);
		RG_SM_SET_MATERIAL(SM, 1, A.InstanceBlue);
		UStaticMeshSocket* Socket = NewObject<UStaticMeshSocket>(SM);
		Socket->SocketName = TEXT("TopSocket");
		Socket->RelativeLocation = FVector(0, 0, 170);
		Socket->RelativeRotation = FRotator(0, 45, 0);
		RG_SM_ADD_SOCKET(SM, Socket);
		RG_SM_SET_NUM_LODS(SM, 3);
		for (int32 Lod = 1; Lod < 3; ++Lod)
		{
			FStaticMeshSourceModel& Model = RG_SM_SOURCE_MODEL(SM, Lod);
			Model.ReductionSettings.PercentTriangles = Lod == 1 ? 0.5f : 0.25f;
			RG_SCREEN_SIZE(Model) = Lod == 1 ? 0.5f : 0.2f;
		}
		SM->bAutoComputeLODScreenSize = false;
		RG_SM_SET_LIGHTMAPRES(SM, 64);
		if (RG_SM_BODYSETUP(SM))
			RG_SM_BODYSETUP(SM)->PhysMaterial = A.PhysMaterial;
		SM->Build(false);
		Touch(SM);

#if UE_AT_LEAST(5, 0)
		A.NaniteMesh = Cast<UStaticMesh>(Tools().DuplicateAsset(TEXT("SM_Ref_Nanite"), TEXT("/Game/Meshes"), SM));
		if (A.NaniteMesh)
		{
			A.NaniteMesh->SetNumSourceModels(1);
			A.NaniteMesh->NaniteSettings.bEnabled = true;
			A.NaniteMesh->Build(false);
			Touch(A.NaniteMesh);
		}
#endif
	}

	{
		UFbxImportUI* Options = BaseOptions();
		Options->MeshTypeToImport = FBXIT_SkeletalMesh;
		Options->bImportMesh = true;
		Options->bImportAsSkeletal = true;
		// 4.15's importer builds it with default params that fail on these small bones, then deletes it through
		// the editor UI (crash in a commandlet): there the physics asset comes from the fallback below instead
		Options->bCreatePhysicsAsset = UE_AT_LEAST(4, 16);
		Options->SkeletalMeshImportData->bImportMorphTargets = true;
#if UE_AT_LEAST(4, 22) // older skeletal imports always take the FBX vertex colors
#if UE_AT_LEAST(4, 7) // 4.7+
		Options->SkeletalMeshImportData->VertexColorImportOption = EVertexColorImportOption::Replace;
#endif
#endif
		Options->bImportAnimations = false;
		TArray<UObject*> Imported = ImportFile(SourceDir / TEXT("SK_Ref.fbx"), TEXT("/Game/Characters"), TEXT("SK_Ref"), Options);
		A.SkeletalMesh = FirstOf<USkeletalMesh>(Imported);
	}
	if (USkeletalMesh* SK = A.SkeletalMesh)
	{
		A.Skeleton = RG_SK_SKELETON(SK);
		A.PhysicsAsset = RG_SK_PHYSICSASSET(SK);
		if (!A.PhysicsAsset)
			A.PhysicsAsset = FindIn<UPhysicsAsset>(TEXT("/Game/Characters"));
		if (!A.Skeleton)
			Fail(TEXT("skeleton missing after import"));
		if (!A.PhysicsAsset)
		{
			A.PhysicsAsset = NewAsset<UPhysicsAsset>(TEXT("SK_Ref_PhysicsAsset"), TEXT("/Game/Characters"));
			FPhysAssetCreateParams Params;
#if !UE_AT_LEAST(4, 18) // plain struct without member defaults before 4.18
			Params.Initialize();
#endif
			Params.MinBoneSize = 0.01f;
#if UE_AT_LEAST(4, 16)
			Params.MinWeldSize = 0.001f;
#endif
			Params.GeomType = EFG_Sphyl;
			Params.RG_PHYS_CREATE_CONSTRAINTS = true;
			FText Error;
			#if UE_AT_LEAST(5, 4)
			const bool bPhysOk = FPhysicsAssetUtils::CreateFromSkeletalMesh(A.PhysicsAsset, SK, Params, Error, true, false); // no progress dialog
#elif UE_AT_LEAST(4, 14)
			const bool bPhysOk = FPhysicsAssetUtils::CreateFromSkeletalMesh(A.PhysicsAsset, SK, Params, Error, true);
#else // 4.13: no bSetToMesh
			const bool bPhysOk = FPhysicsAssetUtils::CreateFromSkeletalMesh(A.PhysicsAsset, SK, Params, Error);
			if (bPhysOk)
				RG_SK_PHYSICSASSET(SK) = A.PhysicsAsset;
#endif
			if (!bPhysOk || RG_PA_BODIES(A.PhysicsAsset).Num() == 0)
				Fail(FString::Printf(TEXT("physics asset creation: %s"), *Error.ToString()));
			Touch(A.PhysicsAsset);
		}
		TArray<FSkeletalMaterial> Materials = RG_SK_MATERIALS(SK);
		if (Materials.Num() >= 2)
		{
			Materials[0].MaterialInterface = A.InstanceRed;
			Materials[1].MaterialInterface = A.InstanceChild;
			RG_SK_SET_MATERIALS(SK, Materials);
		}
		USkeletalMeshSocket* MeshSocket = NewObject<USkeletalMeshSocket>(SK);
		MeshSocket->SocketName = TEXT("MeshOnlySocket");
		MeshSocket->BoneName = TEXT("bone_2");
		MeshSocket->RelativeLocation = FVector(10, 0, 0);
		SK->GetMeshOnlySocketList().Add(MeshSocket);
		Touch(SK);
	}
	if (USkeleton* Skel = A.Skeleton)
	{
		USkeletalMeshSocket* Socket = NewObject<USkeletalMeshSocket>(Skel);
		Socket->SocketName = TEXT("HandSocket");
		Socket->BoneName = TEXT("bone_3");
		Socket->RelativeLocation = FVector(0, 0, 25);
		Skel->Sockets.Add(Socket);
#if UE_AT_LEAST(4, 14) // virtual bones are 4.14+
		Skel->AddNewVirtualBone(TEXT("root"), TEXT("bone_3"));
#endif
#if UE_AT_LEAST(4, 11) // blend profiles are 4.11+
		if (UBlendProfile* Profile = Skel->CreateNewBlendProfile(TEXT("UpperBody")))
			Profile->SetBoneBlendScale(FName(TEXT("bone_2")), 0.5f, true, true);
#endif
		Skel->AddSlotGroupName(TEXT("UpperBodyGroup"));
#if UE_AT_LEAST(4, 6)
		Skel->RegisterSlotNode(TEXT("UpperBody"));
		Skel->SetSlotGroupName(TEXT("UpperBody"), TEXT("UpperBodyGroup"));
#else // 4.5: slot names and groups are separate lists
		Skel->AddSlotNodeName(TEXT("UpperBody"));
#endif
#if UE_AT_LEAST(5, 3)
		Skel->AddCurveMetaData(TEXT("RefCurve"));
#endif
		Touch(Skel);
	}

	{
		FString IesFile = SourceDir / TEXT("LP_Ref.ies");
#if !UE_AT_LEAST(4, 26)
		// older IES loaders assert on LF line endings (check(0) in GetLineContent): import a CRLF copy
		{
			FString Text;
			FFileHelper::LoadFileToString(Text, *IesFile);
			Text.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
			Text.ReplaceInline(TEXT("\n"), TEXT("\r\n"));
			IesFile = FPaths::ProjectSavedDir() / TEXT("RefGen") / TEXT("LP_Ref.ies");
			FFileHelper::SaveStringToFile(Text, *IesFile, FFileHelper::EEncodingOptions::ForceAnsi);
		}
#endif
		TArray<UObject*> Imported = ImportFile(IesFile, TEXT("/Game/Textures"), TEXT("LP_Ref"), nullptr);
		A.LightProfile = FirstOf<UTextureLightProfile>(Imported);
	}
}

void RefGen::MakeAnimation(FAssets& A, const FString& SourceDir)
{
	if (!A.Skeleton)
		return;
	auto ImportAnim = [&](const FString& File, const FString& Name) -> UAnimSequence*
	{
		UFbxImportUI* Options = BaseOptions();
		Options->MeshTypeToImport = FBXIT_Animation;
		Options->OriginalImportType = FBXIT_SkeletalMesh;
		Options->bImportMesh = false;
		Options->bImportAsSkeletal = true;
		Options->bImportAnimations = true;
		Options->Skeleton = A.Skeleton;
		Options->AnimSequenceImportData->AnimationLength = FBXALIT_ExportedTime;
		Options->AnimSequenceImportData->bImportCustomAttribute = true;
		const FString Folder = FString(TEXT("/Game/Animations/Import_")) + Name;
#if UE_AT_LEAST(4, 4)
		UAnimSequence* Seq = FirstOf<UAnimSequence>(ImportFile(SourceDir / File, Folder, Name, Options));
#else // 4.3's FBX factory can't import an animation on its own; Persona's entry point can
		UPackage* AnimPackage = CreatePackage(nullptr, *(Folder / Name));
		UAnimSequence* Seq = UEditorEngine::ImportFbxAnimation(A.Skeleton, AnimPackage, Options->AnimSequenceImportData,
			*FPaths::ConvertRelativePathToFull(SourceDir / File), *Name, false);
		if (Seq)
		{
			FAssetRegistryModule::AssetCreated(Seq);
			Seq->MarkPackageDirty();
		}
#endif
		if (!Seq)
			Seq = FindIn<UAnimSequence>(Folder);
		if (!Seq)
		{
			Fail(FString::Printf(TEXT("anim %s"), *Name));
			return nullptr;
		}
#if UE_AT_LEAST(4, 19)
		TArray<FAssetRenameData> Renames;
		Renames.Emplace(Seq, TEXT("/Game/Animations"), Name);
		Tools().RenameAssets(Renames);
#else // 4.18's rename manager always builds a Slate checkout dialog: move the object by hand
		{
			UPackage* OldPackage = Seq->GetOutermost();
			const FString OldPath = Seq->GetPathName();
			UPackage* NewPackage = CreatePackage(nullptr, *(FString(TEXT("/Game/Animations/")) + Name));
			NewPackage->FullyLoad();
			Seq->Rename(*Name, NewPackage, REN_NonTransactional | REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
			NewPackage->MarkPackageDirty();
			OldPackage->SetDirtyFlag(false);
			FAssetRegistryModule::AssetRenamed(Seq, OldPath);
		}
#endif
		return Seq;
	};
	A.Bend = ImportAnim(TEXT("SK_Ref.fbx"), TEXT("A_Bend"));
	A.Twist = ImportAnim(TEXT("A_Twist.fbx"), TEXT("A_Twist"));
	if (!A.Bend || !A.Twist)
		return;
	RG_SET_PREVIEW_MESH(A.Bend, A.SkeletalMesh);
	RG_SET_PREVIEW_MESH(A.Twist, A.SkeletalMesh);

#if UE_AT_LEAST(4, 11) // sync markers are 4.11+
	FAnimSyncMarker Left;
	Left.MarkerName = TEXT("LeftFoot");
	Left.Time = 0.25f;
	A.Bend->AuthoredSyncMarkers.Add(Left);
	FAnimSyncMarker Right;
	Right.MarkerName = TEXT("RightFoot");
	Right.Time = 0.9f;
	A.Bend->AuthoredSyncMarkers.Add(Right);
#endif
#if UE_AT_LEAST(4, 26) // private before; older engines rebuild it from AuthoredSyncMarkers in Serialize
	A.Bend->RefreshSyncMarkerDataFromAuthored();
#endif

#if UE_AT_LEAST(5, 2)
	{
		IAnimationDataController& Controller = A.Bend->GetController();
		Controller.OpenBracket(FText::FromString(TEXT("RefCurve")), false);
#if UE_AT_LEAST(5, 3)
		const FAnimationCurveIdentifier CurveId(TEXT("RefCurve"), ERawCurveTrackTypes::RCT_Float);
#else // 5.2: identifiers are skeleton SmartNames
		const FAnimationCurveIdentifier CurveId = UAnimationCurveIdentifierExtensions::GetCurveIdentifier(A.Skeleton, TEXT("RefCurve"), ERawCurveTrackTypes::RCT_Float);
#endif
		Controller.AddCurve(CurveId, 0x04, false);
		TArray<FRichCurveKey> Keys;
		Keys.Add(FRichCurveKey(0.f, 0.f));
		Keys.Add(FRichCurveKey(0.75f, 1.f));
		Keys.Add(FRichCurveKey(1.5f, 0.f));
		Controller.SetCurveKeys(CurveId, Keys, false);
		Controller.CloseBracket(false);
	}
#endif

	auto AddNotify = [](UAnimSequenceBase* Seq, UClass* NotifyClass, UClass* StateClass, float Time, float Duration, FName Name, int32 Track)
	{
#if WITH_EDITORONLY_DATA
		while (Seq->AnimNotifyTracks.Num() <= Track)
		{
			FAnimNotifyTrack& NewTrack = RefGen::AddDefaulted(Seq->AnimNotifyTracks);
			NewTrack.TrackName = *FString::FromInt(Seq->AnimNotifyTracks.Num());
			NewTrack.TrackColor = FLinearColor::White;
		}
#endif
		FAnimNotifyEvent& Event = RefGen::AddDefaulted(Seq->Notifies);
		Event.NotifyName = Name;
		RG_NOTIFY_LINK(Event, Seq, Time);
		Event.TriggerTimeOffset = GetTriggerTimeOffsetForType(Seq->CalculateOffsetForNotify(Time));
		Event.TrackIndex = Track;
		if (NotifyClass)
			Event.Notify = NewObject<UAnimNotify>(Seq, NotifyClass, NAME_None, RF_Transactional);
		if (StateClass)
		{
			Event.NotifyStateClass = NewObject<UAnimNotifyState>(Seq, StateClass, NAME_None, RF_Transactional);
			RG_NOTIFY_DURATION(Event, Seq, Time, Duration);
		}
		return &Event;
	};
	AddNotify(A.Bend, nullptr, nullptr, 0.1f, 0.f, TEXT("RefNamedNotify"), 0);
	AddNotify(A.Bend, LoadClass<UAnimNotify>(nullptr, TEXT("/Script/Engine.AnimNotify_PlaySound")), nullptr, 0.5f, 0.f, NAME_None, 1);
#if UE_AT_LEAST(4, 11)
	UClass* PlayParticle = UAnimNotify_PlayParticleEffect::StaticClass();
#else // no header for it in the 4.10-and-older monolithic includes
	UClass* PlayParticle = LoadClass<UAnimNotify>(nullptr, TEXT("/Script/Engine.AnimNotify_PlayParticleEffect"));
#endif
	AddNotify(A.Bend, PlayParticle, nullptr, 0.6f, 0.f, NAME_None, 1);
	AddNotify(A.Bend, nullptr, UAnimNotifyState_TimedParticleEffect::StaticClass(), 0.2f, 0.5f, NAME_None, 2);
	AddNotify(A.Bend, nullptr, UAnimNotifyState_Trail::StaticClass(), 0.8f, 0.4f, NAME_None, 2);
	RG_REFRESH_CACHE(A.Bend);
	Touch(A.Bend);

#if RG_HAS_COMPRESSION_SETTINGS
	// ACL ships with the engine from 5.3; older engines just don't get the ACL compression asset
	UAnimBoneCompressionSettings* Acl = nullptr;
	if (UClass* AclCodec = LoadClass<UAnimBoneCompressionCodec>(nullptr, TEXT("/Script/ACLPlugin.AnimBoneCompressionCodec_ACL"), nullptr, LOAD_Quiet | LOAD_NoWarn))
	{
		Acl = NewAsset<UAnimBoneCompressionSettings>(TEXT("ABC_ACL"), TEXT("/Game/Animations/Compression"));
		Acl->Codecs.Add(NewObject<UAnimBoneCompressionCodec>(Acl, AclCodec));
		Touch(Acl);
	}
	else
	{
#if UE_AT_LEAST(5, 3)
		Fail(TEXT("ACL codec class missing"));
#else
		UE_LOG(LogRefGen, Display, TEXT("no ACLPlugin in this engine, skipping ABC_ACL"));
#endif
	}
	UAnimBoneCompressionSettings* Bitwise = NewAsset<UAnimBoneCompressionSettings>(TEXT("ABC_Bitwise"), TEXT("/Game/Animations/Compression"));
	Bitwise->Codecs.Add(NewObject<UAnimBoneCompressionCodec>(Bitwise, LoadClass<UAnimBoneCompressionCodec>(nullptr, TEXT("/Script/Engine.AnimCompress_BitwiseCompressOnly"))));
	Touch(Bitwise);
	UAnimBoneCompressionSettings* Linear = NewAsset<UAnimBoneCompressionSettings>(TEXT("ABC_RemoveLinear"), TEXT("/Game/Animations/Compression"));
	Linear->Codecs.Add(NewObject<UAnimBoneCompressionCodec>(Linear, LoadClass<UAnimBoneCompressionCodec>(nullptr, TEXT("/Script/Engine.AnimCompress_RemoveLinearKeys"))));
	Touch(Linear);
	UAnimBoneCompressionSettings* PerTrack = NewAsset<UAnimBoneCompressionSettings>(TEXT("ABC_PerTrack"), TEXT("/Game/Animations/Compression"));
	PerTrack->Codecs.Add(NewObject<UAnimBoneCompressionCodec>(PerTrack, LoadClass<UAnimBoneCompressionCodec>(nullptr, TEXT("/Script/Engine.AnimCompress_PerTrackCompression"))));
	Touch(PerTrack);

	auto SetCompression = [](UAnimSequence* Seq, UAnimBoneCompressionSettings* Settings) { Seq->BoneCompressionSettings = Settings; };
#else
	// 4.24 and older have no shared compression settings assets: each sequence owns an instanced UAnimCompress scheme
	UE_LOG(LogRefGen, Display, TEXT("no AnimBoneCompressionSettings in this engine, using per-sequence compression schemes"));
	UClass* Acl = nullptr;
	UClass* Bitwise = LoadClass<UAnimCompress>(nullptr, TEXT("/Script/Engine.AnimCompress_BitwiseCompressOnly"));
	UClass* Linear = LoadClass<UAnimCompress>(nullptr, TEXT("/Script/Engine.AnimCompress_RemoveLinearKeys"));
	UClass* PerTrack = LoadClass<UAnimCompress>(nullptr, TEXT("/Script/Engine.AnimCompress_PerTrackCompression"));
	auto SetCompression = [](UAnimSequence* Seq, UClass* Scheme) { Seq->CompressionScheme = NewObject<UAnimCompress>(Seq, Scheme); };
#endif
	SetCompression(A.Bend, Acl ? Acl : PerTrack);
	SetCompression(A.Twist, Bitwise);

	auto Duplicate = [&](UAnimSequence* Source, const FString& Name) -> UAnimSequence*
	{
#if UE_AT_LEAST(5, 5)
		UAnimSequence* Copy = Cast<UAnimSequence>(Tools().DuplicateAsset(Name, TEXT("/Game/Animations"), Source));
#else
		// 5.4 and older (the 5.2-5.4 launcher builds too): duplicating an AnimSequence corrupts its AnimDataModel stream (FDuplicateDataReader
		// overread -> crash), so re-import the same FBX under the new name instead
		UAnimSequence* Copy = ImportAnim(Source == A.Bend ? TEXT("SK_Ref.fbx") : TEXT("A_Twist.fbx"), Name);
		if (Copy)
			RG_SET_PREVIEW_MESH(Copy, A.SkeletalMesh);
#endif
		if (!Copy)
			Fail(FString::Printf(TEXT("duplicate %s"), *Name));
		return Copy;
	};
	A.BendAdditive = Duplicate(A.Bend, TEXT("A_Bend_Additive"));
	if (A.BendAdditive)
	{
		A.BendAdditive->AdditiveAnimType = AAT_LocalSpaceBase;
		A.BendAdditive->RefPoseType = ABPT_RefPose;
		SetCompression(A.BendAdditive, Linear);
		Touch(A.BendAdditive);
	}
	A.AimCenter = Duplicate(A.Twist, TEXT("A_Aim_Center"));
	A.AimLeft = Duplicate(A.Twist, TEXT("A_Aim_Left"));
	A.AimRight = Duplicate(A.Bend, TEXT("A_Aim_Right"));
	for (UAnimSequence* Aim : { A.AimCenter, A.AimLeft, A.AimRight })
	{
		if (!Aim)
			continue;
		Aim->AdditiveAnimType = AAT_RotationOffsetMeshSpace;
		Aim->RefPoseType = ABPT_AnimFrame;
		Aim->RefPoseSeq = A.Twist;
		Aim->RefFrameIndex = 0;
		SetCompression(Aim, PerTrack);
		Touch(Aim);
	}
	Touch(A.Twist);
}
