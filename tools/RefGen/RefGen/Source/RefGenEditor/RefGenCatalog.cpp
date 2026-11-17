#include "RefGenCatalog.h"
#include "RefTypes.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureCube.h"
#include "Engine/TextureRenderTargetCube.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Font.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "Materials/Material.h"
#if __has_include("MaterialDomain.h")
#include "MaterialDomain.h"
#endif
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Sound/SoundWave.h"
#include "Particles/ParticleSystem.h"
#include "Curves/CurveFloat.h"
#include "PhysicsEngine/BodySetup.h"
#include "Engine/DataTable.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectIterator.h"
#if __has_include("Engine/VolumeTexture.h") // 4.21+
#include "Engine/VolumeTexture.h"
#define RG_VOLUME_TEXTURE 1
#else
#define RG_VOLUME_TEXTURE 0
#endif
#if __has_include("Engine/Texture2DArray.h") // 4.24+
#include "Engine/Texture2DArray.h"
#define RG_TEXTURE_ARRAY UE_AT_LEAST(4, 25)
#else
#define RG_TEXTURE_ARRAY 0
#endif
#if __has_include("Engine/TextureCubeArray.h") // 5.0+
#include "Engine/TextureCubeArray.h"
#define RG_CUBE_ARRAY 1
#else
#define RG_CUBE_ARRAY 0
#endif
#if UE_AT_LEAST(4, 16)
#include "Engine/FontFace.h"
#endif
#if UE_AT_LEAST(4, 19)
#include "Materials/MaterialExpressionMaterialAttributeLayers.h"
#include "Materials/MaterialFunctionMaterialLayer.h"
#include "Materials/MaterialFunctionMaterialLayerBlend.h"
#endif
#if UE_AT_LEAST(4, 24)
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#endif
#if RG_WITH_UMGEDITOR && UE_AT_LEAST(4, 10)
#if !UE_AT_LEAST(4, 15)
#include "UMG.h" // pre-IWYU widget headers need their bases from the module header
#endif
#include "WidgetBlueprint.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#define RG_WIDGETS 1
#else
#define RG_WIDGETS 0
#endif
#if RG_WITH_PAPER2D && UE_AT_LEAST(4, 8)
#include "PaperSprite.h"
#include "PaperFlipbook.h"
#include "SpriteEditorOnlyTypes.h"
#define RG_PAPER2D 1
#else
#define RG_PAPER2D 0
#endif
#if RG_WITH_HAIRSTRANDSCORE && RG_WITH_HAIRSTRANDSEDITOR && UE_AT_LEAST(5, 0)
#include "GroomAsset.h"
#include "HairDescription.h"
#include "HairAttributes.h"
#include "HairStrandsImporter.h"
#include "GroomImportOptions.h"
#define RG_GROOM 1
#else
#define RG_GROOM 0
#endif
#if RG_WITH_NIAGARAEDITOR && UE_AT_LEAST(4, 26)
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEditorUtilities.h"
#define RG_NIAGARA 1
#else
#define RG_NIAGARA 0
#endif
#if RG_WITH_GEOMETRYCOLLECTIONENGINE && UE_AT_LEAST(5, 4)
#include "GeometryCollection/GeometryCollectionObject.h"
#include "GeometryCollection/GeometryCollectionEngineConversion.h"
#define RG_GEOMETRY_COLLECTION 1
#else
#define RG_GEOMETRY_COLLECTION 0
#endif

#if UE_AT_LEAST(5, 1)
#define RG_MAT_INPUT(M, Name) (M)->GetEditorOnlyData()->Name
#else
#define RG_MAT_INPUT(M, Name) (M)->Name
#endif

namespace
{
	using namespace RefGen;

	TArray<UObject*> GCandidates;
	TMap<FString, UObject*> GRemembered;

	const TCHAR* CatalogRoot = TEXT("/Game/Catalog");

	FString Folder(const TCHAR* Sub)
	{
		return FString(CatalogRoot) / Sub;
	}

	void Skip(const FString& What)
	{
		UE_LOG(LogRefGen, Display, TEXT("catalog: skipped %s (not in this engine)"), *What);
	}

	void Registered(UObject* Asset)
	{
		if (!Asset)
			return;
		FAssetRegistryModule::AssetCreated(Asset);
		Asset->MarkPackageDirty();
	}

	UPackage* NewPackage(const FString& PackageName)
	{
#if UE_AT_LEAST(4, 26)
		return CreatePackage(*PackageName);
#else
		return CreatePackage(nullptr, *PackageName);
#endif
	}

	// ---------------------------------------------------------------- generic assets

	struct FGenericAsset
	{
		const TCHAR* Class;   // class short name
		const TCHAR* Name;    // asset name
		const TCHAR* Sub;     // folder below /Game/Catalog
		const TCHAR* Factory; // preferred factory class (optional)
	};

	// Assets whose default state (plus the reference assets filled into their empty references) is valid.
	const FGenericAsset GGenericAssets[] = {
		// audio
		{ TEXT("SoundSubmix"), TEXT("SSM_Ref"), TEXT("Audio"), nullptr },
		{ TEXT("SoundConcurrency"), TEXT("SCON_Ref"), TEXT("Audio"), nullptr },
		{ TEXT("ReverbEffect"), TEXT("RE_Ref"), TEXT("Audio"), nullptr },
		{ TEXT("DialogueVoice"), TEXT("DV_Ref"), TEXT("Audio"), nullptr },
		{ TEXT("DialogueWave"), TEXT("DW_Ref"), TEXT("Audio"), nullptr },
		{ TEXT("SoundSourceBus"), TEXT("SSB_Ref"), TEXT("Audio"), nullptr },
		{ TEXT("AudioBus"), TEXT("AB_Ref"), TEXT("Audio"), nullptr },
		{ TEXT("SoundEffectSourcePresetChain"), TEXT("SEPC_Ref"), TEXT("Audio"), nullptr },
		{ TEXT("ForceFeedbackEffect"), TEXT("FFE_Ref"), TEXT("Audio"), nullptr },
		{ TEXT("HapticFeedbackEffect_Curve"), TEXT("HFE_Ref"), TEXT("Audio"), nullptr },
		{ TEXT("MetaSoundSource"), TEXT("MSS_Ref"), TEXT("Audio"), nullptr },
		{ TEXT("MetaSoundPatch"), TEXT("MSP_Ref"), TEXT("Audio"), nullptr },
		// rendering
		{ TEXT("SubsurfaceProfile"), TEXT("SP_Ref"), TEXT("Rendering"), nullptr },
		{ TEXT("SpecularProfile"), TEXT("SPP_Ref"), TEXT("Rendering"), nullptr },
		{ TEXT("RuntimeVirtualTexture"), TEXT("RVT_Ref"), TEXT("Rendering"), nullptr },
		{ TEXT("CurveLinearColorAtlas"), TEXT("CA_Ref"), TEXT("Rendering"), nullptr },
		{ TEXT("PhysicalMaterialMask"), TEXT("PMM_Ref"), TEXT("Rendering"), nullptr },
		{ TEXT("TextureRenderTarget2DArray"), TEXT("RTA_Ref"), TEXT("Rendering"), nullptr },
		{ TEXT("TextureRenderTargetVolume"), TEXT("RTV_Ref"), TEXT("Rendering"), nullptr },
		// media
		{ TEXT("MediaPlayer"), TEXT("MP_Ref"), TEXT("Media"), nullptr },
		{ TEXT("FileMediaSource"), TEXT("FMS_Ref"), TEXT("Media"), nullptr },
		{ TEXT("StreamMediaSource"), TEXT("SMS_Ref"), TEXT("Media"), nullptr },
		{ TEXT("MediaPlaylist"), TEXT("MPL_Ref"), TEXT("Media"), nullptr },
		{ TEXT("MediaTexture"), TEXT("MT_Ref"), TEXT("Media"), nullptr },
		{ TEXT("ImgMediaSource"), TEXT("IMS_Ref"), TEXT("Media"), nullptr },
		// input
		{ TEXT("InputAction"), TEXT("IA_Ref"), TEXT("Input"), nullptr },
		{ TEXT("InputMappingContext"), TEXT("IMC_Ref"), TEXT("Input"), nullptr },
		{ TEXT("TouchInterface"), TEXT("TI_Ref"), TEXT("Input"), nullptr },
		// AI
		{ TEXT("BlackboardData"), TEXT("BB_Ref"), TEXT("AI"), nullptr },
		{ TEXT("BehaviorTree"), TEXT("BT_Ref"), TEXT("AI"), nullptr },
		{ TEXT("EnvQuery"), TEXT("EQS_Ref"), TEXT("AI"), nullptr },
		{ TEXT("StateTree"), TEXT("STT_Ref"), TEXT("AI"), nullptr },
		{ TEXT("SmartObjectDefinition"), TEXT("SOD_Ref"), TEXT("AI"), nullptr },
		// animation
		{ TEXT("AimOffsetBlendSpace1D"), TEXT("AO1D_Ref"), TEXT("Animation"), nullptr },
		{ TEXT("MirrorDataTable"), TEXT("MDT_Ref"), TEXT("Animation"), nullptr },
		{ TEXT("AnimCurveCompressionSettings"), TEXT("ACC_Ref"), TEXT("Animation"), nullptr },
		{ TEXT("VariableFrameStrippingSettings"), TEXT("VFS_Ref"), TEXT("Animation"), nullptr },
		{ TEXT("CameraAnim"), TEXT("CAM_Ref"), TEXT("Animation"), nullptr },
		{ TEXT("ControlRigBlueprint"), TEXT("CR_Ref"), TEXT("Animation"), nullptr },
		{ TEXT("IKRigDefinition"), TEXT("IK_Ref"), TEXT("Animation"), nullptr },
		{ TEXT("IKRetargeter"), TEXT("RTG_Ref"), TEXT("Animation"), nullptr },
		// physics
		{ TEXT("ChaosPhysicalMaterial"), TEXT("CPM_Ref"), TEXT("Physics"), nullptr },
		{ TEXT("GeometryCollectionCache"), TEXT("GCC_Ref"), TEXT("Physics"), nullptr },
		// data
		{ TEXT("CompositeDataTable"), TEXT("CDT_Ref"), TEXT("Data"), nullptr },
		{ TEXT("CompositeCurveTable"), TEXT("CCT_Ref"), TEXT("Data"), nullptr },
		{ TEXT("PrimaryAssetLabel"), TEXT("PAL_Ref"), TEXT("Data"), nullptr },
		{ TEXT("DataLayerAsset"), TEXT("DL_Ref"), TEXT("Data"), nullptr },
		{ TEXT("LandscapeGrassType"), TEXT("LGT_Ref"), TEXT("World"), nullptr },
		{ TEXT("FoliageType_Actor"), TEXT("FTA_Ref"), TEXT("World"), nullptr },
		{ TEXT("ProceduralFoliageSpawner"), TEXT("PFS_Ref"), TEXT("World"), nullptr },
		{ TEXT("PCGGraph"), TEXT("PCG_Ref"), TEXT("World"), nullptr },
		// effects
		{ TEXT("NiagaraParameterCollection"), TEXT("NPC_Ref"), TEXT("Effects"), nullptr },
		{ TEXT("NiagaraEffectType"), TEXT("NET_Ref"), TEXT("Effects"), nullptr },
		// 2D / UI
		{ TEXT("PaperTileSet"), TEXT("TS_Ref"), TEXT("Paper2D"), nullptr },
		{ TEXT("PaperTileMap"), TEXT("TM_Ref"), TEXT("Paper2D"), nullptr },
		{ TEXT("SlateBrushAsset"), TEXT("SBA_Ref"), TEXT("UI"), nullptr },
	};

	UObject* MakeGeneric(const FGenericAsset& Entry, FAssets& A)
	{
		const FString ClassName = Entry.Class;
		UClass* Class = ClassNamed(Entry.Class);
		if (!Class || Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
		{
			Skip(Entry.Class);
			return nullptr;
		}
#if !UE_AT_LEAST(4, 20)
		if (ClassName.StartsWith(TEXT("Niagara")))
		{
			Skip(Entry.Class);
			return nullptr;
		}
#endif
#if !UE_AT_LEAST(4, 5)
		// behavior trees were experimental: their factories crash without the editor's AI settings
		// (and Paper2D's tile maps, whose factory crashes the same way)
		if (ClassName == TEXT("BlackboardData") || ClassName == TEXT("BehaviorTree") || ClassName == TEXT("EnvQuery") || ClassName == TEXT("PaperTileMap"))
		{
			Skip(Entry.Class);
			return nullptr;
		}
#endif
		UE_LOG(LogRefGen, Display, TEXT("catalog: creating %s"), Entry.Class);
		UObject* Asset = nullptr;
		if (UFactory* Factory = FactoryFor(Class, Entry.Factory))
		{
			if (ClassName == TEXT("StateTree")) // the factory asks for a schema
				TrySetPropertyText(Factory, TEXT("StateTreeSchemaClass"), TEXT("/Script/GameplayStateTree.StateTreeComponentSchema"));
			Asset = Tools().CreateAsset(Entry.Name, Folder(Entry.Sub), Class, Factory);
		}
		if (!Asset && ClassName == TEXT("StateTree"))
		{
			Skip(TEXT("StateTree without a schema"));
			return nullptr;
		}
		if (!Asset) // no factory, or one that needs its dialog (skeleton / row struct picker): a plain object, set up below
			Asset = NewAsset(Entry.Name, Folder(Entry.Sub), Class);
		if (ClassName == TEXT("MirrorDataTable"))
		{
			TrySetPropertyText(Asset, TEXT("RowStruct"), TEXT("/Script/Engine.MirrorTableRow"));
			if (A.Skeleton)
				TrySetPropertyText(Asset, TEXT("Skeleton"), *static_cast<UObject*>(static_cast<void*>(A.Skeleton))->GetPathName());
		}
#if UE_AT_LEAST(4, 21)
		else if (ClassName == TEXT("CompositeDataTable") && A.Table && A.Table->GetRowStruct())
		{
			TrySetPropertyText(Asset, TEXT("RowStruct"), *A.Table->GetRowStruct()->GetPathName());
			TrySetPropertyText(Asset, TEXT("ParentTables"), *FString::Printf(TEXT("(\"%s\")"), *A.Table->GetPathName()));
		}
#endif
		if (!Asset)
		{
			Fail(FString::Printf(TEXT("catalog asset %s (%s)"), Entry.Name, Entry.Class));
			return nullptr;
		}
		AssignAssets(Asset);
		UE_LOG(LogRefGen, Display, TEXT("catalog: %s"), Entry.Name);
		// no PostEditChange in general: some (sound effect chains, ...) crash in it without an editor UI. Subsurface
		// profiles need it to register with the renderer before a material references them (4.14 crashes otherwise).
		if (ClassName == TEXT("SubsurfaceProfile"))
			Touch(Asset);
		else
			Asset->MarkPackageDirty();
		Remember(Entry.Name, Asset);
		return Asset;
	}

	// ---------------------------------------------------------------- blueprints

	UBlueprint* MakeBlueprint(const TCHAR* Name, const TCHAR* Sub, UClass* Parent, UClass* BlueprintClass = nullptr, UClass* GeneratedClass = nullptr)
	{
		if (!Parent)
			return nullptr;
		UE_LOG(LogRefGen, Display, TEXT("catalog: blueprint %s (%s)"), Name, *Parent->GetName());
		UPackage* Package = NewPackage(Folder(Sub) / Name);
		UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(Parent, Package, FName(Name), BPTYPE_Normal,
			BlueprintClass ? BlueprintClass : UBlueprint::StaticClass(), GeneratedClass ? GeneratedClass : UBlueprintGeneratedClass::StaticClass());
		if (!Blueprint)
		{
			Fail(FString::Printf(TEXT("blueprint %s"), Name));
			return nullptr;
		}
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		Registered(Blueprint);
		Remember(Name, Blueprint);
		return Blueprint;
	}

	void CatBlueprints()
	{
		struct FEntry
		{
			const TCHAR* Name;
			const TCHAR* Parents[3]; // first one this engine has
		};
		const FEntry Entries[] = {
			{ TEXT("BP_RefComponent"), { TEXT("ActorComponent"), nullptr, nullptr } },
			{ TEXT("BP_RefSceneComponent"), { TEXT("SceneComponent"), nullptr, nullptr } },
			{ TEXT("BP_RefPawn"), { TEXT("DefaultPawn"), TEXT("Pawn"), nullptr } },
			{ TEXT("BP_RefHUD"), { TEXT("HUD"), nullptr, nullptr } },
			{ TEXT("BP_RefPlayerState"), { TEXT("PlayerState"), nullptr, nullptr } },
			{ TEXT("BP_RefGameState"), { TEXT("GameStateBase"), TEXT("GameState"), nullptr } },
			{ TEXT("BP_RefGameInstance"), { TEXT("GameInstance"), nullptr, nullptr } },
			{ TEXT("BP_RefSaveGame"), { TEXT("SaveGame"), nullptr, nullptr } },
			{ TEXT("BP_RefObject"), { TEXT("Object"), nullptr, nullptr } },
			{ TEXT("BP_RefDamageType"), { TEXT("DamageType"), nullptr, nullptr } },
			{ TEXT("BP_RefCameraManager"), { TEXT("PlayerCameraManager"), nullptr, nullptr } },
			{ TEXT("BP_RefCameraShake"), { TEXT("LegacyCameraShake"), TEXT("MatineeCameraShake"), TEXT("CameraShake") } },
			{ TEXT("BP_RefAnimNotifyState"), { TEXT("AnimNotifyState"), nullptr, nullptr } },
			{ TEXT("BP_RefAIController"), { TEXT("AIController"), nullptr, nullptr } },
			{ TEXT("BP_RefBTTask"), { TEXT("BTTask_BlueprintBase"), nullptr, nullptr } },
			{ TEXT("BP_RefWheeledVehicle"), { TEXT("WheeledVehiclePawn"), TEXT("WheeledVehicle"), nullptr } },
		};
		for (const FEntry& Entry : Entries)
		{
			UClass* Parent = nullptr;
			for (const TCHAR* Name : Entry.Parents)
				if (Name && !Parent)
					Parent = ClassNamed(Name);
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 14
			// 4.14's WheeledVehicle CDO crashes the blueprint compile (vehicle movement half-moved to PhysXVehicles)
			if (FCString::Strcmp(Entry.Name, TEXT("BP_RefWheeledVehicle")) == 0)
				Parent = nullptr;
#endif
			if (!Parent)
			{
				Skip(Entry.Name);
				continue;
			}
			MakeBlueprint(Entry.Name, TEXT("Blueprints"), Parent);
		}
	}

	// ---------------------------------------------------------------- textures

	template <typename T>
	T* RawTexture(const FString& Name, int32 SizeX, int32 SizeY, int32 Slices, ETextureSourceFormat Format, int32 Seed)
	{
		const int32 Bpp = Format == TSF_RGBA16F ? 8 : Format == TSF_G8 ? 1 : 4;
		T* Texture = NewAsset<T>(Name, Folder(TEXT("Textures")));
		TArray<uint8> Data;
		Data.SetNumZeroed(SizeX * SizeY * Slices * Bpp);
		for (int32 Slice = 0; Slice < Slices; ++Slice)
			for (int32 Y = 0; Y < SizeY; ++Y)
				for (int32 X = 0; X < SizeX; ++X)
				{
					uint8* P = &Data[((Slice * SizeY + Y) * SizeX + X) * Bpp];
					const bool Check = (((X * 8) / FMath::Max(SizeX, 1)) + ((Y * 8) / FMath::Max(SizeY, 1)) + Slice + Seed) % 2 == 0;
					if (Format == TSF_RGBA16F)
					{
						FFloat16* H = reinterpret_cast<FFloat16*>(P);
						H[0] = FFloat16(X * 4.f / SizeX);
						H[1] = FFloat16(Y * 4.f / SizeY);
						H[2] = FFloat16(Check ? 2.f : 0.25f);
						H[3] = FFloat16(1.f);
					}
					else if (Format == TSF_G8)
						P[0] = uint8((X ^ Y ^ (Slice * 31)) & 0xFF);
					else
					{
						P[0] = uint8(Check ? 200 : 30);
						P[1] = uint8((X * 255) / FMath::Max(SizeX - 1, 1));
						P[2] = uint8((Y * 255) / FMath::Max(SizeY - 1, 1));
						P[3] = uint8(255 - Slice * 40);
					}
				}
		Texture->Source.Init(SizeX, SizeY, Slices, 1, Format, Data.GetData());
		return Texture;
	}

	void CatTextures(FAssets& A)
	{
		// one texture per compression setting: covers every pixel format the engine cooks to on PC
		for (const FEnumEntry& Entry : EnumValues(TEXT("/Script/Engine.TextureCompressionSettings")))
		{
			const bool bFloat = Entry.Name.Contains(TEXT("HDR")) || Entry.Name.Contains(TEXT("Float")) || Entry.Name.Contains(TEXT("Reflection"));
			UTexture2D* T = RawTexture<UTexture2D>(TEXT("T_TC_") + Entry.Name, 64, 64, 1, bFloat ? TSF_RGBA16F : TSF_BGRA8, int32(Entry.Value));
			T->CompressionSettings = TextureCompressionSettings(Entry.Value);
			T->SRGB = !bFloat && (Entry.Name == TEXT("Default") || Entry.Name == TEXT("EditorIcon") || Entry.Name.StartsWith(TEXT("BC7")));
			Touch(T);
		}

		UTexture2D* NoMips = RawTexture<UTexture2D>(TEXT("T_NoMips"), 128, 128, 1, TSF_BGRA8, 1);
		NoMips->MipGenSettings = TMGS_NoMipmaps;
		Touch(NoMips);

		UTexture2D* NonPow2 = RawTexture<UTexture2D>(TEXT("T_NonPow2"), 100, 60, 1, TSF_BGRA8, 2);
		NonPow2->MipGenSettings = TMGS_NoMipmaps;
		NonPow2->CompressionSettings = TC_EditorIcon;
		Touch(NonPow2);

		UTexture2D* Large = RawTexture<UTexture2D>(TEXT("T_Large"), 2048, 2048, 1, TSF_BGRA8, 3); // streamed mips (bulk data)
		Touch(Large);
		Remember(TEXT("T_Large"), Large);

		UTexture2D* NeverStream = RawTexture<UTexture2D>(TEXT("T_NeverStream"), 512, 512, 1, TSF_BGRA8, 4);
		NeverStream->NeverStream = true;
		Touch(NeverStream);

		UTexture2D* Gray = RawTexture<UTexture2D>(TEXT("T_G8"), 64, 64, 1, TSF_G8, 5);
		Gray->SRGB = false;
		Gray->CompressionSettings = TC_Grayscale;
		Touch(Gray);

#if UE_AT_LEAST(4, 23)
		UTexture2D* Virtual = RawTexture<UTexture2D>(TEXT("T_Virtual"), 1024, 1024, 1, TSF_BGRA8, 6);
		Virtual->VirtualTextureStreaming = true;
		Touch(Virtual);
		Remember(TEXT("T_Virtual"), Virtual);

		// UDIM: a virtual texture with 3 blocks (1001, 1002, 1011)
		{
			UTexture2D* Udim = NewAsset<UTexture2D>(TEXT("T_Udim"), Folder(TEXT("Textures")));
			const int32 Size = 256;
			const int32 Tiles[3][2] = { { 0, 0 }, { 1, 0 }, { 0, 1 } };
			TArray<FTextureSourceBlock> Blocks;
			TArray<TArray<uint8>> Pixels;
			TArray<const uint8*> DataPerBlock;
			Pixels.SetNum(3);
			for (int32 Tile = 0; Tile < 3; ++Tile)
			{
				FTextureSourceBlock Block;
				Block.BlockX = Tiles[Tile][0];
				Block.BlockY = Tiles[Tile][1];
				Block.SizeX = Size;
				Block.SizeY = Size;
				Block.NumSlices = 1;
				Block.NumMips = 1;
				Blocks.Add(Block);
				Pixels[Tile].SetNumZeroed(Size * Size * 4);
				for (int32 Index = 0; Index < Size * Size; ++Index)
				{
					Pixels[Tile][Index * 4 + 0] = uint8(Tile * 100);
					Pixels[Tile][Index * 4 + 1] = uint8(Index % Size);
					Pixels[Tile][Index * 4 + 2] = uint8(Index / Size);
					Pixels[Tile][Index * 4 + 3] = 255;
				}
			}
			for (int32 Tile = 0; Tile < 3; ++Tile)
				DataPerBlock.Add(Pixels[Tile].GetData());
			const ETextureSourceFormat Format = TSF_BGRA8;
			Udim->Source.InitBlocked(&Format, Blocks.GetData(), 1, Blocks.Num(), DataPerBlock.GetData());
			Udim->VirtualTextureStreaming = true;
			Touch(Udim);
			Remember(TEXT("T_Udim"), Udim);
		}
#endif
#if RG_TEXTURE_ARRAY
		{
			UTexture2DArray* Array = RawTexture<UTexture2DArray>(TEXT("T_Array"), 64, 64, 4, TSF_BGRA8, 7);
			Touch(Array);
			Remember(TEXT("T_Array"), Array);
		}
#endif
#if RG_VOLUME_TEXTURE
		{
			UVolumeTexture* Volume = RawTexture<UVolumeTexture>(TEXT("T_Volume"), 32, 32, 32, TSF_BGRA8, 8);
			Touch(Volume);
			Remember(TEXT("T_Volume"), Volume);
		}
#endif
#if RG_CUBE_ARRAY
		{
			UTextureCubeArray* CubeArray = RawTexture<UTextureCubeArray>(TEXT("T_CubeArray"), 32, 32, 12, TSF_BGRA8, 9);
			Touch(CubeArray);
		}
#endif
		{
			UTextureCube* HdrCube = RawTexture<UTextureCube>(TEXT("T_CubeHdr"), 32, 32, 6, TSF_RGBA16F, 10);
			HdrCube->CompressionSettings = TC_HDR;
			HdrCube->SRGB = false;
			Touch(HdrCube);
		}
		{
			UTextureRenderTargetCube* RTCube = NewAsset<UTextureRenderTargetCube>(TEXT("RTC_Ref"), Folder(TEXT("Textures")));
			RTCube->Init(64, PF_FloatRGBA);
			RTCube->ClearColor = FLinearColor(0.3f, 0.2f, 0.1f, 1.f);
			Touch(RTCube);
		}
	}

	// ---------------------------------------------------------------- materials

	UMaterialExpression* Expr(UObject* Owner, UClass* Class, int32 X, int32 Y)
	{
		if (!Class)
			return nullptr;
		UMaterialExpression* E = NewObject<UMaterialExpression>(Owner, Class, NAME_None, RF_Transactional);
		E->MaterialExpressionEditorX = X;
		E->MaterialExpressionEditorY = Y;
		if (UMaterial* M = Cast<UMaterial>(Owner))
		{
			E->Material = M;
#if UE_AT_LEAST(5, 1)
			M->GetExpressionCollection().AddExpression(E);
#else
			M->Expressions.Add(E);
#endif
		}
		else if (UMaterialFunction* F = Cast<UMaterialFunction>(Owner))
		{
#if UE_AT_LEAST(5, 1)
			F->GetExpressionCollection().AddExpression(E);
#else
			F->FunctionExpressions.Add(E);
#endif
		}
		return E;
	}

	UMaterialExpression* Expr(UObject* Owner, const TCHAR* ClassName, int32 X, int32 Y)
	{
		return Expr(Owner, ClassNamed(ClassName), X, Y);
	}

	template <typename T>
	T* ExprT(UObject* Owner, const TCHAR* ClassName, int32 X, int32 Y)
	{
		return static_cast<T*>(Expr(Owner, ClassName, X, Y));
	}

	UMaterialExpressionVectorParameter* Color(UMaterial* M, const TCHAR* Name, const FLinearColor& Value, int32 Y)
	{
		auto* P = ExprT<UMaterialExpressionVectorParameter>(M, TEXT("MaterialExpressionVectorParameter"), -500, Y);
		P->ParameterName = Name;
		P->DefaultValue = Value;
		return P;
	}

	UMaterialExpressionScalarParameter* Scalar(UMaterial* M, const TCHAR* Name, float Value, int32 Y)
	{
		auto* P = ExprT<UMaterialExpressionScalarParameter>(M, TEXT("MaterialExpressionScalarParameter"), -500, Y);
		P->ParameterName = Name;
		P->DefaultValue = Value;
		return P;
	}

	UMaterialExpressionTextureSample* Sample(UMaterial* M, UTexture* Texture, int32 Y, const TCHAR* ClassName = TEXT("MaterialExpressionTextureSample"))
	{
		auto* S = ExprT<UMaterialExpressionTextureSample>(M, ClassName, -700, Y);
		if (S)
		{
			S->Texture = Texture;
			S->AutoSetSampleType();
		}
		return S;
	}

	UMaterial* NewMaterial(const FString& Name, const TCHAR* Sub = TEXT("Materials"))
	{
		UE_LOG(LogRefGen, Display, TEXT("catalog: material %s"), *Name);
		UMaterial* M = Create<UMaterial>(Name, Folder(Sub), NewObject<UMaterialFactoryNew>());
		if (M)
			Remember(*Name, M);
		return M;
	}

	void Finish(UMaterial* M)
	{
		if (!M)
			return;
#if !UE_AT_LEAST(5, 0) && UE_AT_LEAST(4, 15)
		TGuardValue<bool> ClientGuard(GIsClient, true);
#endif
#if !UE_AT_LEAST(4, 15) // as RefGenMaterials.cpp: 4.14 flushes shader maps before allocating resources on a fresh material
		{
#if UE_AT_LEAST(4, 9)
			TGuardValue<bool> AllowRendering(PRIVATE_GAllowCommandletRendering, true);
#else
			TGuardValue<bool> AllowRendering(PRIVATE_GIsRunningCommandlet, false);
#endif
			M->ForceRecompileForRendering();
		}
#endif
		M->PreEditChange(nullptr);
		M->PostEditChange();
		M->MarkPackageDirty();
	}

	UMaterialInstanceConstant* NewInstance(const FString& Name, UMaterialInterface* Parent)
	{
		auto* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
		Factory->InitialParent = Parent;
		return Create<UMaterialInstanceConstant>(Name, Folder(TEXT("Materials")), Factory);
	}

	// ---------------------------------------------------------------- landscape

	// an input of a struct in an array property (Layers[1].LayerInput, ...); null when the engine has no such field
	FExpressionInput* ArrayStructInput(UObject* Owner, const TCHAR* ArrayName, int32 Index, const TCHAR* Field)
	{
		FArrayProperty* Array = RG_CAST_FIELD(FArrayProperty, RG_FIND_PROPERTY(Owner->GetClass(), ArrayName));
		FStructProperty* Inner = Array ? RG_CAST_FIELD(FStructProperty, Array->Inner) : nullptr;
		if (!Inner)
			return nullptr;
		FScriptArrayHelper Helper(Array, Array->ContainerPtrToValuePtr<void>(Owner));
		FStructProperty* Input = RG_CAST_FIELD(FStructProperty, RG_FIND_PROPERTY(Inner->Struct, Field));
		if (!Input || Index >= Helper.Num())
			return nullptr;
		return Input->ContainerPtrToValuePtr<FExpressionInput>(Helper.GetRawPtr(Index));
	}

	// the paint layer infos (LI_Grass, LI_Rock) and M_Landscape: a weight-blended layer per paint layer, layer UVs and
	// the grass output that scatters LGT_Ref on the grass layer. RefGenMaps.cpp paints the landscape with them.
	void MakeLandscapeMaterial(FAssets& A)
	{
		UClass* LayerInfoClass = ClassNamed(TEXT("LandscapeLayerInfoObject"));
		UClass* BlendClass = ClassNamed(TEXT("MaterialExpressionLandscapeLayerBlend"));
		if (!LayerInfoClass || !BlendClass)
		{
			Skip(TEXT("landscape layers"));
			return;
		}
		const TCHAR* Layers[] = { TEXT("Grass"), TEXT("Rock") };
		for (const TCHAR* Layer : Layers)
		{
			const FString Name = FString::Printf(TEXT("LI_%s"), Layer);
			UObject* Info = NewAsset(*Name, Folder(TEXT("World")), LayerInfoClass);
			if (!Info)
				continue;
			TrySetPropertyText(Info, TEXT("LayerName"), Layer);
			TrySetPropertyText(Info, TEXT("LayerUsageDebugColor"), FCString::Strcmp(Layer, TEXT("Rock")) == 0 ? TEXT("(R=0.5,G=0.5,B=0.5,A=1)") : TEXT("(R=0.1,G=0.8,B=0.1,A=1)"));
			if (A.PhysMaterial && FCString::Strcmp(Layer, TEXT("Rock")) == 0)
				TrySetPropertyText(Info, TEXT("PhysMaterial"), *static_cast<UObject*>(static_cast<void*>(A.PhysMaterial))->GetPathName());
			Info->MarkPackageDirty();
			Remember(*Name, Info);
		}

		UMaterial* M = NewMaterial(TEXT("M_Landscape"));
		if (!M)
			return;
		UMaterialExpression* Blend = Expr(M, BlendClass, -300, 0);
		TrySetPropertyText(Blend, TEXT("Layers"), TEXT("((LayerName=\"Grass\",BlendType=LB_WeightBlend,PreviewWeight=1.0),(LayerName=\"Rock\",BlendType=LB_WeightBlend))"));
		if (FExpressionInput* Grass = ArrayStructInput(Blend, TEXT("Layers"), 0, TEXT("LayerInput")))
			Grass->Expression = Color(M, TEXT("GrassColor"), FLinearColor(0.1f, 0.4f, 0.05f), 0);
		if (FExpressionInput* Rock = ArrayStructInput(Blend, TEXT("Layers"), 1, TEXT("LayerInput")))
		{
			UMaterialExpressionTextureSample* Albedo = Sample(M, A.Albedo, 200);
			if (UMaterialExpression* Coords = Expr(M, TEXT("MaterialExpressionLandscapeLayerCoords"), -900, 200))
			{
				TrySetPropertyText(Coords, TEXT("MappingScale"), TEXT("8.0"));
				if (Albedo)
					Albedo->Coordinates.Expression = Coords;
			}
			Rock->Expression = Albedo;
		}
		RG_MAT_INPUT(M, BaseColor).Expression = Blend;
		RG_MAT_INPUT(M, Roughness).Expression = Scalar(M, TEXT("Roughness"), 0.8f, 400);
		UObject* GrassType = Recall(TEXT("LGT_Ref"));
		UMaterialExpression* Output = GrassType ? Expr(M, TEXT("MaterialExpressionLandscapeGrassOutput"), 0, 600) : nullptr;
		if (Output)
		{
			TrySetPropertyText(Output, TEXT("GrassTypes"), *FString::Printf(TEXT("((Name=\"Grass\",GrassType=\"%s\"))"), *GrassType->GetPathName()));
			if (UMaterialExpression* Weight = Expr(M, TEXT("MaterialExpressionLandscapeLayerSample"), -300, 600))
			{
				TrySetPropertyText(Weight, TEXT("ParameterName"), TEXT("Grass"));
				if (FExpressionInput* Input = ArrayStructInput(Output, TEXT("GrassTypes"), 0, TEXT("Input")))
					Input->Expression = Weight;
			}
		}
		Finish(M);
	}

	void CatMaterials(FAssets& A)
	{
		UObject* Subsurface = Recall(TEXT("SP_Ref"));

		// every shading model this engine has
		for (const FEnumEntry& Entry : EnumValues(TEXT("/Script/Engine.EMaterialShadingModel")))
		{
			if (Entry.Name == TEXT("Unlit") || Entry.Name == TEXT("DefaultLit") || Entry.Name == TEXT("NUM") || Entry.Name.Contains(TEXT("Strata")) || Entry.Name.Contains(TEXT("Substrate")))
				continue;
			const bool bFromExpression = Entry.Name == TEXT("FromMaterialExpression");
			UMaterial* M = NewMaterial(TEXT("M_SM_") + Entry.Name);
			if (!M)
				continue;
			RG_MAT_INPUT(M, BaseColor).Expression = Color(M, TEXT("Color"), FLinearColor(0.8f, 0.5f, 0.3f), 0);
			RG_MAT_INPUT(M, Roughness).Expression = Scalar(M, TEXT("Roughness"), 0.4f, 200);
			if (bFromExpression)
			{
#if UE_AT_LEAST(4, 24)
				if (UMaterialExpression* Model = Expr(M, TEXT("MaterialExpressionShadingModel"), -300, 400))
				{
					TrySetPropertyText(Model, TEXT("ShadingModel"), TEXT("MSM_ClearCoat"));
					RG_MAT_INPUT(M, ShadingModelFromMaterialExpression).Expression = Model;
				}
#endif
			}
			else
				M->SetShadingModel(EMaterialShadingModel(Entry.Value));
			if (Entry.Name == TEXT("SubsurfaceProfile") && Subsurface)
				TrySetPropertyText(M, TEXT("SubsurfaceProfile"), *Subsurface->GetPathName());
			if (Entry.Name == TEXT("ThinTranslucent"))
			{
				M->BlendMode = BLEND_Translucent;
				TrySetPropertyText(M, TEXT("TranslucencyLightingMode"), TEXT("TLM_SurfacePerPixelLighting"));
				if (UMaterialExpression* Out = Expr(M, TEXT("MaterialExpressionThinTranslucentMaterialOutput"), 0, 400))
				{
					AssignAssets(Out);
					FStructProperty* Input = RG_CAST_FIELD(FStructProperty, RG_FIND_PROPERTY(Out->GetClass(), TEXT("TransmittanceColor")));
					if (Input)
						Input->ContainerPtrToValuePtr<FExpressionInput>(Out)->Expression = Color(M, TEXT("Transmittance"), FLinearColor(0.5f, 0.8f, 1.f), 600);
				}
			}
			if (Entry.Name == TEXT("Hair"))
				M->bUsedWithSkeletalMesh = true;
			Finish(M);
		}

		// every blend mode not covered by M_Opaque / M_Masked / M_Translucent
		for (const FEnumEntry& Entry : EnumValues(TEXT("/Script/Engine.EBlendMode")))
		{
			if (Entry.Name == TEXT("Opaque") || Entry.Name == TEXT("Masked") || Entry.Name == TEXT("Translucent") || Entry.Name.Contains(TEXT("Transmittance")))
				continue;
			UMaterial* M = NewMaterial(TEXT("M_Blend_") + Entry.Name);
			if (!M)
				continue;
			M->BlendMode = EBlendMode(Entry.Value);
			M->SetShadingModel(MSM_Unlit);
			RG_MAT_INPUT(M, EmissiveColor).Expression = Color(M, TEXT("Color"), FLinearColor(0.2f, 0.6f, 1.f), 0);
			RG_MAT_INPUT(M, Opacity).Expression = Scalar(M, TEXT("Opacity"), 0.5f, 200);
			Finish(M);
		}

		// every domain not covered by M_Opaque / M_Decal / M_LightFunction / M_PostProcess
		for (const FEnumEntry& Entry : EnumValues(TEXT("/Script/Engine.EMaterialDomain")))
		{
			if (Entry.Name == TEXT("Surface") || Entry.Name == TEXT("DeferredDecal") || Entry.Name == TEXT("LightFunction") || Entry.Name == TEXT("PostProcess"))
				continue;
			UMaterial* M = NewMaterial(TEXT("M_Domain_") + Entry.Name);
			if (!M)
				continue;
			M->MaterialDomain = EMaterialDomain(Entry.Value);
			if (Entry.Name == TEXT("Volume"))
			{
				M->BlendMode = BLEND_Additive;
				RG_MAT_INPUT(M, BaseColor).Expression = Color(M, TEXT("Albedo"), FLinearColor(0.7f, 0.7f, 0.8f), 0);
				RG_MAT_INPUT(M, SubsurfaceColor).Expression = Color(M, TEXT("Extinction"), FLinearColor(0.1f, 0.1f, 0.1f), 200);
			}
			else if (Entry.Name == TEXT("UI"))
			{
				M->BlendMode = BLEND_Translucent;
				RG_MAT_INPUT(M, EmissiveColor).Expression = Sample(M, A.Albedo, 0);
				RG_MAT_INPUT(M, Opacity).Expression = Scalar(M, TEXT("Opacity"), 0.8f, 300);
			}
			else
			{
				RG_MAT_INPUT(M, BaseColor).Expression = Sample(M, A.Albedo, 0);
				RG_MAT_INPUT(M, Roughness).Expression = Scalar(M, TEXT("Roughness"), 0.5f, 300);
			}
			Finish(M);
		}

		// world position offset, pixel depth offset, two sided, dithered LOD
		if (UMaterial* M = NewMaterial(TEXT("M_WorldPositionOffset")))
		{
			RG_MAT_INPUT(M, BaseColor).Expression = Color(M, TEXT("Color"), FLinearColor(0.3f, 0.9f, 0.3f), 0);
			auto* Time = ExprT<UMaterialExpressionTime>(M, TEXT("MaterialExpressionTime"), -900, 300);
			auto* Sine = ExprT<UMaterialExpressionSine>(M, TEXT("MaterialExpressionSine"), -700, 300);
			Sine->Input.Expression = Time;
			auto* Normal = Expr(M, TEXT("MaterialExpressionVertexNormalWS"), -700, 450);
			auto* Mul = ExprT<UMaterialExpressionMultiply>(M, TEXT("MaterialExpressionMultiply"), -400, 350);
			Mul->A.Expression = Sine;
			Mul->B.Expression = Normal;
			RG_MAT_INPUT(M, WorldPositionOffset).Expression = Mul;
#if UE_AT_LEAST(4, 8)
			RG_MAT_INPUT(M, PixelDepthOffset).Expression = Scalar(M, TEXT("DepthOffset"), 2.f, 600);
#endif
			M->TwoSided = true;
#if UE_AT_LEAST(4, 9)
			M->DitheredLODTransition = true;
#endif
			M->bUsedWithInstancedStaticMeshes = true;
#if UE_AT_LEAST(4, 14)
			M->bUsedWithSplineMeshes = true;
#endif
			Finish(M);
		}

#if UE_AT_LEAST(4, 10) && !UE_AT_LEAST(5, 0) // tessellation was removed in 5.0
		if (UMaterial* M = NewMaterial(TEXT("M_Tessellation")))
		{
			RG_MAT_INPUT(M, BaseColor).Expression = Sample(M, A.Albedo, 0);
			M->D3D11TessellationMode = MTM_PNTriangles;
			auto* Normal = Expr(M, TEXT("MaterialExpressionVertexNormalWS"), -700, 450);
			auto* Mul = ExprT<UMaterialExpressionMultiply>(M, TEXT("MaterialExpressionMultiply"), -400, 350);
			Mul->A.Expression = Normal;
			Mul->B.Expression = Scalar(M, TEXT("Displacement"), 4.f, 600);
			RG_MAT_INPUT(M, WorldDisplacement).Expression = Mul;
			RG_MAT_INPUT(M, TessellationMultiplier).Expression = Scalar(M, TEXT("Tessellation"), 2.f, 700);
			Finish(M);
		}
#endif

#if UE_AT_LEAST(4, 23)
		if (UTexture* Virtual = Recall<UTexture>(TEXT("T_Virtual")))
			if (UMaterial* M = NewMaterial(TEXT("M_VirtualTexture")))
			{
				auto* S = Sample(M, Virtual, 0);
				UTexture* Udim = Recall<UTexture>(TEXT("T_Udim"));
				auto* S2 = Udim ? Sample(M, Udim, 300) : nullptr;
				if (S2)
				{
					auto* Mul = ExprT<UMaterialExpressionMultiply>(M, TEXT("MaterialExpressionMultiply"), -300, 100);
					Mul->A.Expression = S;
					Mul->B.Expression = S2;
					RG_MAT_INPUT(M, BaseColor).Expression = Mul;
				}
				else
					RG_MAT_INPUT(M, BaseColor).Expression = S;
				Finish(M);
			}
		if (UObject* Rvt = Recall(TEXT("RVT_Ref")))
		{
			if (UMaterial* M = NewMaterial(TEXT("M_RVT_Write")))
			{
				RG_MAT_INPUT(M, BaseColor).Expression = Sample(M, A.Albedo, 0);
				if (UMaterialExpression* Out = Expr(M, TEXT("MaterialExpressionRuntimeVirtualTextureOutput"), 0, 300))
				{
					FStructProperty* Input = RG_CAST_FIELD(FStructProperty, RG_FIND_PROPERTY(Out->GetClass(), TEXT("BaseColor")));
					if (Input)
						Input->ContainerPtrToValuePtr<FExpressionInput>(Out)->Expression = Color(M, TEXT("RvtColor"), FLinearColor(0.4f, 0.3f, 0.2f), 300);
				}
				Finish(M);
			}
			if (UMaterial* M = NewMaterial(TEXT("M_RVT_Sample")))
			{
				if (UMaterialExpression* S = Expr(M, TEXT("MaterialExpressionRuntimeVirtualTextureSample"), -500, 0))
				{
					AssignAssets(S);
					RG_MAT_INPUT(M, BaseColor).Expression = S;
				}
				Finish(M);
			}
		}
#endif
		if (UTexture* Array = Recall<UTexture>(TEXT("T_Array")))
			if (UMaterial* M = NewMaterial(TEXT("M_TextureArray")))
			{
				UMaterialExpression* S = Expr(M, TEXT("MaterialExpressionTextureSampleParameter2DArray"), -700, 0);
				if (S)
				{
					TrySetPropertyText(S, TEXT("ParameterName"), TEXT("ArrayTexture"));
					static_cast<UMaterialExpressionTextureSample*>(S)->Texture = Array;
					RG_MAT_INPUT(M, BaseColor).Expression = S;
				}
				Finish(M);
			}
		if (UTexture* Volume = Recall<UTexture>(TEXT("T_Volume")))
			if (UMaterial* M = NewMaterial(TEXT("M_VolumeTexture")))
			{
				UMaterialExpression* S = Expr(M, TEXT("MaterialExpressionTextureSampleParameterVolume"), -700, 0);
				if (S)
				{
					TrySetPropertyText(S, TEXT("ParameterName"), TEXT("VolumeTexture"));
					static_cast<UMaterialExpressionTextureSample*>(S)->Texture = Volume;
					static_cast<UMaterialExpressionTextureSample*>(S)->Coordinates.Expression = Expr(M, TEXT("MaterialExpressionWorldPosition"), -1000, 0);
					RG_MAT_INPUT(M, BaseColor).Expression = S;
				}
				Finish(M);
			}

#if UE_AT_LEAST(4, 19)
		// material layers: two layers and a blend on a MaterialAttributeLayers node, plus an instance
		{
			UClass* LayerClass = ClassNamed(TEXT("MaterialFunctionMaterialLayer"));
			UClass* BlendClass = ClassNamed(TEXT("MaterialFunctionMaterialLayerBlend"));
			UMaterialFunctionInterface* LayerA = LayerClass ? Cast<UMaterialFunctionInterface>(Tools().CreateAsset(TEXT("ML_Base"), Folder(TEXT("Materials")), LayerClass, FactoryFor(LayerClass))) : nullptr;
			UMaterialFunctionInterface* LayerB = LayerClass ? Cast<UMaterialFunctionInterface>(Tools().CreateAsset(TEXT("ML_Top"), Folder(TEXT("Materials")), LayerClass, FactoryFor(LayerClass))) : nullptr;
			UMaterialFunctionInterface* Blend = BlendClass ? Cast<UMaterialFunctionInterface>(Tools().CreateAsset(TEXT("MLB_Ref"), Folder(TEXT("Materials")), BlendClass, FactoryFor(BlendClass))) : nullptr;
			if (LayerA && LayerB && Blend)
			{
				Touch(LayerA);
				Touch(LayerB);
				Touch(Blend);
				if (UMaterial* M = NewMaterial(TEXT("M_Layered")))
				{
					M->bUseMaterialAttributes = true;
					auto* Layers = ExprT<UMaterialExpressionMaterialAttributeLayers>(M, TEXT("MaterialExpressionMaterialAttributeLayers"), -400, 0);
					FMaterialLayersFunctions& L = Layers->DefaultLayers;
#if UE_AT_LEAST(5, 0)
					if (L.Layers.Num() == 0)
						L.AddDefaultBackgroundLayer();
#endif
					if (L.Layers.Num() > 0)
					{
						L.Layers[0] = LayerA;
						L.AppendBlendedLayer();
						L.Layers.Last() = LayerB;
						L.Blends.Last() = Blend;
					}
					RG_MAT_INPUT(M, MaterialAttributes).Expression = Layers;
					Finish(M);
					if (UMaterialInstanceConstant* MI = NewInstance(TEXT("MI_Layered"), M))
						Touch(MI);
				}
			}
			else
				Skip(TEXT("material layers"));
		}
#endif

#if UE_AT_LEAST(5, 0)
		// Substrate (Strata before 5.3): a slab BSDF on the front material
		if (UClass* SlabClass = ClassNamed(TEXT("MaterialExpressionSubstrateSlabBSDF")) ? ClassNamed(TEXT("MaterialExpressionSubstrateSlabBSDF")) : ClassNamed(TEXT("MaterialExpressionStrataSlabBSDF")))
			if (UMaterial* M = NewMaterial(TEXT("M_Substrate")))
			{
				UMaterialExpression* Slab = Expr(M, SlabClass, -400, 0);
				FStructProperty* Diffuse = RG_CAST_FIELD(FStructProperty, RG_FIND_PROPERTY(SlabClass, TEXT("DiffuseAlbedo")));
				if (Diffuse)
					Diffuse->ContainerPtrToValuePtr<FExpressionInput>(Slab)->Expression = Color(M, TEXT("Albedo"), FLinearColor(0.6f, 0.2f, 0.2f), 300);
				RG_MAT_INPUT(M, FrontMaterial).Expression = Slab;
				Finish(M);
			}
#endif
	}

	// ---------------------------------------------------------------- meshes

	void CatMeshes(FAssets& A)
	{
#if UE_AT_LEAST(4, 24)
		// a procedural grid: > 65535 vertices (32 bit indices), 4 UV channels, vertex colors, 2 sections
		{
			UStaticMesh* Grid = NewAsset<UStaticMesh>(TEXT("SM_Grid"), Folder(TEXT("Meshes")));
#if UE_AT_LEAST(4, 27)
			Grid->GetStaticMaterials().Add(FStaticMaterial(A.InstanceRed, TEXT("Red")));
			Grid->GetStaticMaterials().Add(FStaticMaterial(A.InstanceBlue, TEXT("Blue")));
#else
			Grid->StaticMaterials.Add(FStaticMaterial(A.InstanceRed, TEXT("Red")));
			Grid->StaticMaterials.Add(FStaticMaterial(A.InstanceBlue, TEXT("Blue")));
#endif
			RG_SM_SET_NUM_LODS(Grid, 1);
			FMeshDescription* Desc = Grid->CreateMeshDescription(0);
			FStaticMeshAttributes Attributes(*Desc);
			Attributes.Register();
#if UE_AT_LEAST(5, 0)
			typedef FVector3f FRGVec;
			typedef FVector2f FRGVec2;
			typedef FVector4f FRGVec4;
#else
			typedef FVector FRGVec;
			typedef FVector2D FRGVec2;
			typedef FVector4 FRGVec4;
#endif
			auto Positions = Attributes.GetVertexPositions();
			auto Normals = Attributes.GetVertexInstanceNormals();
			auto UVs = Attributes.GetVertexInstanceUVs();
			auto Colors = Attributes.GetVertexInstanceColors();
#if UE_AT_LEAST(5, 0)
			UVs.SetNumChannels(4);
#else
			UVs.SetNumIndices(4);
#endif
			const int32 N = 301;
			const FPolygonGroupID GroupA = Desc->CreatePolygonGroup();
			const FPolygonGroupID GroupB = Desc->CreatePolygonGroup();
			Attributes.GetPolygonGroupMaterialSlotNames()[GroupA] = TEXT("Red");
			Attributes.GetPolygonGroupMaterialSlotNames()[GroupB] = TEXT("Blue");
			TArray<FVertexInstanceID> Instances;
			Instances.SetNum(N * N);
			for (int32 Y = 0; Y < N; ++Y)
				for (int32 X = 0; X < N; ++X)
				{
					const FVertexID V = Desc->CreateVertex();
					Positions[V] = FRGVec(X * 10.f, Y * 10.f, 20.f * FMath::Sin(X * 0.05f) * FMath::Cos(Y * 0.05f));
					const FVertexInstanceID I = Desc->CreateVertexInstance(V);
					Normals[I] = FRGVec(0, 0, 1);
					for (int32 Channel = 0; Channel < 4; ++Channel)
						UVs.Set(I, Channel, FRGVec2(X / float(N - 1) * (Channel + 1), Y / float(N - 1) * (Channel + 1)));
					Colors[I] = FRGVec4(X / float(N - 1), Y / float(N - 1), 0.5f, 1.f);
					Instances[Y * N + X] = I;
				}
			for (int32 Y = 0; Y + 1 < N; ++Y)
				for (int32 X = 0; X + 1 < N; ++X)
				{
					const FPolygonGroupID Group = X < N / 2 ? GroupA : GroupB;
					const FVertexInstanceID I0 = Instances[Y * N + X], I1 = Instances[Y * N + X + 1], I2 = Instances[(Y + 1) * N + X], I3 = Instances[(Y + 1) * N + X + 1];
					TArray<FVertexInstanceID> T0;
					T0.Add(I0);
					T0.Add(I2);
					T0.Add(I1);
					TArray<FVertexInstanceID> T1;
					T1.Add(I1);
					T1.Add(I2);
					T1.Add(I3);
#if UE_AT_LEAST(4, 26)
					Desc->CreateTriangle(Group, T0);
					Desc->CreateTriangle(Group, T1);
#else
					Desc->CreatePolygon(Group, T0);
					Desc->CreatePolygon(Group, T1);
#endif
				}
			RG_SM_SOURCE_MODEL(Grid, 0).BuildSettings.bRecomputeNormals = false;
			RG_SM_SOURCE_MODEL(Grid, 0).BuildSettings.bRecomputeTangents = true;
			RG_SM_SOURCE_MODEL(Grid, 0).BuildSettings.bGenerateLightmapUVs = false;
			RG_SM_SOURCE_MODEL(Grid, 0).BuildSettings.bUseFullPrecisionUVs = true;
			Grid->CommitMeshDescription(0);
			Grid->Build(false);
			Touch(Grid);
			Remember(TEXT("SM_Grid"), Grid);
		}
#endif

		// simple collision of every shape, complex-as-simple off
		if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
		{
			UPackage* Package = NewPackage(Folder(TEXT("Meshes")) / TEXT("SM_Collision"));
			UStaticMesh* Shapes = DuplicateObject<UStaticMesh>(Cube, Package, TEXT("SM_Collision"));
			Shapes->SetFlags(RF_Public | RF_Standalone);
			if (UBodySetup* Body = RG_SM_BODYSETUP(Shapes))
			{
				Body->AggGeom.EmptyElements();
				FKBoxElem Box(50.f, 60.f, 70.f);
				Body->AggGeom.BoxElems.Add(Box);
				FKSphereElem Sphere(40.f);
				Sphere.Center = FVector(0, 0, 80);
				Body->AggGeom.SphereElems.Add(Sphere);
				FKSphylElem Capsule(20.f, 60.f);
				Capsule.Center = FVector(80, 0, 0);
				Body->AggGeom.SphylElems.Add(Capsule);
#if UE_AT_LEAST(4, 21)
				FKTaperedCapsuleElem Tapered(20.f, 10.f, 60.f);
				Tapered.Center = FVector(-80, 0, 0);
				Body->AggGeom.TaperedCapsuleElems.Add(Tapered);
#endif
				Body->CollisionTraceFlag = CTF_UseSimpleAsComplex;
			}
			Registered(Shapes);
			Touch(Shapes);
		}

#if UE_AT_LEAST(5, 5)
		// a Nanite skeletal mesh
		if (A.SkeletalMesh)
		{
			UPackage* Package = NewPackage(TEXT("/Game/Characters/SK_Ref_Nanite"));
			USkeletalMesh* Nanite = DuplicateObject<USkeletalMesh>(A.SkeletalMesh, Package, TEXT("SK_Ref_Nanite"));
			Nanite->SetFlags(RF_Public | RF_Standalone);
			TrySetPropertyText(Nanite, TEXT("NaniteSettings"), TEXT("(bEnabled=True)"));
			Registered(Nanite);
			Touch(Nanite);
		}
#endif

#if RG_GEOMETRY_COLLECTION
		if (A.StaticMesh)
			if (UGeometryCollection* Collection = NewAsset<UGeometryCollection>(TEXT("GC_Ref"), Folder(TEXT("Physics"))))
			{
				TArray<UMaterialInterface*> Materials;
				Materials.Add(A.InstanceRed);
				Materials.Add(A.InstanceBlue);
				FGeometryCollectionEngineConversion::AppendStaticMesh(A.StaticMesh, Materials, FTransform::Identity, Collection);
				Collection->InitializeMaterials();
				Collection->RebuildRenderData();
				Touch(Collection);
				Remember(TEXT("GC_Ref"), Collection);
			}
#endif

#if RG_GROOM
		// a procedural groom: 64 strands of 8 points
		{
			FHairDescription Hair;
			auto Positions = Hair.VertexAttributes().GetAttributesRef<FVector3f>(HairAttribute::Vertex::Position);
			auto Counts = Hair.StrandAttributes().GetAttributesRef<int>(HairAttribute::Strand::VertexCount);
			for (int32 Strand = 0; Strand < 64; ++Strand)
			{
				const FStrandID S = Hair.AddStrand();
				Counts[S] = 8;
				for (int32 Point = 0; Point < 8; ++Point)
				{
					const FVertexID V = Hair.AddVertex();
					Positions[V] = FVector3f((Strand % 8) * 1.f, (Strand / 8) * 1.f, Point * 2.f + 0.3f * FMath::Sin(Strand + Point * 0.7f));
				}
			}
			UGroomImportOptions* Options = NewObject<UGroomImportOptions>();
			// one interpolation setting per hair group (the description has no group ids: one group)
			if (FArrayProperty* Settings = RG_CAST_FIELD(FArrayProperty, RG_FIND_PROPERTY(UGroomImportOptions::StaticClass(), TEXT("InterpolationSettings"))))
			{
				FScriptArrayHelper Helper(Settings, Settings->ContainerPtrToValuePtr<void>(Options));
				if (Helper.Num() == 0)
					Helper.AddValues(1);
			}
			UPackage* Package = NewPackage(Folder(TEXT("Meshes")) / TEXT("GR_Ref"));
			FHairImportContext Context(Options, Package, UGroomAsset::StaticClass(), TEXT("GR_Ref"), RF_Public | RF_Standalone);
			UGroomAsset* Groom = FHairStrandsImporter::ImportHair(Context, Hair);
			if (Groom)
			{
				Registered(Groom);
				Touch(Groom);
				Remember(TEXT("GR_Ref"), Groom);
				AddCandidate(Groom);
			}
			else
				Fail(TEXT("groom import"));
		}
#endif
	}

	// ---------------------------------------------------------------- audio

	void CatAudio(FAssets& A, const FString& SourceDir)
	{
		const TCHAR* Files[] = { TEXT("SW_Stereo"), TEXT("SW_Surround"), TEXT("SW_Long") };
		for (const TCHAR* File : Files)
		{
#if !UE_AT_LEAST(4, 25)
			if (FCString::Strcmp(File, TEXT("SW_Surround")) == 0)
			{
				Skip(TEXT("6 channel wave import")); // older importers take only mono or stereo files
				continue;
			}
#endif
			USoundWave* Wave = nullptr;
			for (UObject* O : ImportFile(SourceDir / FString(File) + TEXT(".wav"), Folder(TEXT("Audio")), File, nullptr))
				if (!Wave)
					Wave = Cast<USoundWave>(O);
			if (!Wave && FCString::Strcmp(File, TEXT("SW_Surround")) == 0)
				Skip(TEXT("6 channel wave import")); // older importers take multichannel sound only as one file per channel
			else if (!Wave)
				Fail(FString::Printf(TEXT("import %s.wav"), File));
			else
				Remember(File, Wave);
		}
		USoundWave* Long = Recall<USoundWave>(TEXT("SW_Long"));
		if (!Long)
			return;
		auto Variant = [&](const FString& Name, const TCHAR* Property, const TCHAR* Value)
		{
			if (!RG_FIND_PROPERTY(USoundWave::StaticClass(), Property))
			{
				Skip(Name);
				return;
			}
			UPackage* Package = NewPackage(Folder(TEXT("Audio")) / Name);
			USoundWave* Wave = DuplicateObject<USoundWave>(Long, Package, *Name);
			Wave->SetFlags(RF_Public | RF_Standalone);
			TrySetPropertyText(Wave, Property, Value);
			Registered(Wave);
			Touch(Wave);
		};
		// 5.x codecs; RADAudio is 5.4+ (an unknown value fails the text import and the wave keeps its default)
		const TCHAR* Codecs[] = { TEXT("PCM"), TEXT("ADPCM"), TEXT("BinkAudio"), TEXT("Opus"), TEXT("RADAudio"), TEXT("PlatformSpecific") };
		const TArray<FEnumEntry> Known = EnumValues(TEXT("/Script/Engine.ESoundAssetCompressionType"));
		for (const TCHAR* Codec : Codecs)
		{
			if (!Known.ContainsByPredicate([&](const FEnumEntry& Item) { return Item.Name == Codec; }))
				Skip(FString(TEXT("SW_Codec_")) + Codec);
			else
				Variant(FString(TEXT("SW_Codec_")) + Codec, TEXT("SoundAssetCompressionType"), Codec);
		}
		// streaming: stream caching (4.25+) and the older streaming flag
		Variant(TEXT("SW_LoadOnDemand"), TEXT("LoadingBehavior"), TEXT("LoadOnDemand"));
		Variant(TEXT("SW_RetainOnLoad"), TEXT("LoadingBehavior"), TEXT("RetainOnLoad"));
		Variant(TEXT("SW_ForceInline"), TEXT("LoadingBehavior"), TEXT("ForceInline"));
		Variant(TEXT("SW_Streaming"), TEXT("bStreaming"), TEXT("True"));
	}

	// ---------------------------------------------------------------- fonts, widgets, 2D

	void MakeFonts(FAssets& A)
	{
#if UE_AT_LEAST(4, 16)
		// a font face holding the TTF inline (the import factory flushes Slate's font cache: no Slate in commandlets)
		const FString Ttf = FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf");
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *Ttf))
		{
			Fail(TEXT("read Roboto-Regular.ttf"));
			return;
		}
		UFontFace* Face = NewAsset<UFontFace>(TEXT("FF_Roboto"), Folder(TEXT("UI")));
		Face->SourceFilename = Ttf;
		Face->LoadingPolicy = EFontLoadingPolicy::Inline;
		Face->FontFaceData->SetData(MoveTemp(Bytes));
		Face->MarkPackageDirty();
		Remember(TEXT("FF_Roboto"), Face);
		UFont* Font = NewAsset<UFont>(TEXT("F_Runtime"), Folder(TEXT("UI")));
		Font->FontCacheType = EFontCacheType::Runtime;
		FTypefaceEntry Entry;
		Entry.Name = TEXT("Regular");
		Entry.Font = FFontData(Face);
		Font->CompositeFont.DefaultTypeface.Fonts.Add(Entry);
		Font->MarkPackageDirty();
		AddCandidate(Font);
#endif
		// an offline (pre-rasterized) font: texture pages
		if (UFactory* Offline = FactoryFor(UFont::StaticClass(), TEXT("TrueTypeFontFactory")))
		{
			UObject* OfflineFont = Tools().CreateAsset(TEXT("F_Offline"), Folder(TEXT("UI")), UFont::StaticClass(), Offline);
			if (OfflineFont)
				OfflineFont->MarkPackageDirty();
			else
				Fail(TEXT("offline font"));
		}
	}

	void MakeWidgets(FAssets& A)
	{
#if RG_WIDGETS
		UPackage* Package = NewPackage(Folder(TEXT("UI")) / TEXT("WBP_Ref"));
		UWidgetBlueprint* Widget = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(UUserWidget::StaticClass(), Package, TEXT("WBP_Ref"),
			BPTYPE_Normal, UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
		if (!Widget)
		{
			Fail(TEXT("widget blueprint"));
			return;
		}
		UWidgetTree* Tree = Widget->WidgetTree;
		UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		Tree->RootWidget = Canvas;
		UVerticalBox* Box = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Box"));
		Canvas->AddChild(Box);
		UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon"));
		Image->SetBrushFromTexture(A.Albedo);
		Box->AddChild(Image);
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
		Text->SetText(FText::FromString(TEXT("RefGen Widget")));
		Box->AddChild(Text);
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button"));
		Box->AddChild(Button);
		UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("Progress"));
		Bar->SetPercent(0.42f);
		Box->AddChild(Bar);
		FKismetEditorUtilities::CompileBlueprint(Widget);
		Registered(Widget);
		Remember(TEXT("WBP_Ref"), Widget);
#else
		Skip(TEXT("widget blueprints"));
#endif
	}

	void MakePaper2D(FAssets& A)
	{
#if RG_PAPER2D
		UPaperSprite* Sprite = NewAsset<UPaperSprite>(TEXT("SPR_Ref"), Folder(TEXT("Paper2D")));
		FSpriteAssetInitParameters Params;
		Params.SetTextureAndFill(A.SubUV);
		Sprite->InitializeSprite(Params);
		Touch(Sprite);
		Remember(TEXT("SPR_Ref"), Sprite);
		AddCandidate(Sprite);
		UPaperFlipbook* Flipbook = NewAsset<UPaperFlipbook>(TEXT("FB_Ref"), Folder(TEXT("Paper2D")));
		{
			FScopedFlipbookMutator Mutator(Flipbook);
			Mutator.FramesPerSecond = 12.f;
			for (int32 Frame = 0; Frame < 3; ++Frame)
			{
				FPaperFlipbookKeyFrame Key;
				Key.Sprite = Sprite;
				Key.FrameRun = Frame + 1;
				Mutator.KeyFrames.Add(Key);
			}
		}
		Touch(Flipbook);
		Remember(TEXT("FB_Ref"), Flipbook);
		AddCandidate(Flipbook);
#else
		Skip(TEXT("Paper2D sprites"));
#endif
	}

	void MakeNiagaraAssets(FAssets& A);
	void MakeNiagara(FAssets& A)
	{
#if UE_AT_LEAST(4, 20)
		MakeNiagaraAssets(A);
#else
		Skip(TEXT("Niagara")); // experimental: its script shaders fail to compile in the cook
#endif
	}

	void MakeNiagaraAssets(FAssets& A)
	{
		UClass* SystemClass = ClassNamed(TEXT("NiagaraSystem"));
		UClass* EmitterClass = ClassNamed(TEXT("NiagaraEmitter"));
		if (!SystemClass || !EmitterClass)
		{
			Skip(TEXT("Niagara"));
			return;
		}
		UObject* Emitter = Tools().CreateAsset(TEXT("NE_Ref"), Folder(TEXT("Effects")), EmitterClass, FactoryFor(EmitterClass));
		UObject* System = Tools().CreateAsset(TEXT("NS_Ref"), Folder(TEXT("Effects")), SystemClass, FactoryFor(SystemClass));
		if (!Emitter || !System)
		{
			Fail(TEXT("niagara system / emitter"));
			return;
		}
#if RG_NIAGARA
#if UE_AT_LEAST(5, 1)
		FNiagaraEditorUtilities::AddEmitterToSystem(*CastChecked<UNiagaraSystem>(System), *CastChecked<UNiagaraEmitter>(Emitter), CastChecked<UNiagaraEmitter>(Emitter)->GetExposedVersion().VersionGuid);
#else
		FNiagaraEditorUtilities::AddEmitterToSystem(*CastChecked<UNiagaraSystem>(System), *CastChecked<UNiagaraEmitter>(Emitter));
#endif
		CastChecked<UNiagaraSystem>(System)->RequestCompile(false);
		CastChecked<UNiagaraSystem>(System)->WaitForCompilationComplete();
#endif
		Touch(Emitter);
		Touch(System);
		Remember(TEXT("NS_Ref"), System);
		AddCandidate(System);
	}

	void MakeVectorField(const FString& SourceDir)
	{
		UObject* Field = nullptr;
		for (UObject* O : ImportFile(SourceDir / TEXT("VF_Ref.fga"), Folder(TEXT("Effects")), TEXT("VF_Ref"), nullptr))
			if (!Field)
				Field = O;
		if (!Field)
			Fail(TEXT("import VF_Ref.fga"));
		else
		{
			Remember(TEXT("VF_Ref"), Field);
			AddCandidate(Field);
		}
	}
}

// ---------------------------------------------------------------- shared helpers

UClass* RefGen::ClassNamed(const TCHAR* Name)
{
	for (TObjectIterator<UClass> It; It; ++It)
		if (It->GetName() == Name && !It->HasAnyClassFlags(CLASS_NewerVersionExists) && It->GetOutermost()->GetName().StartsWith(TEXT("/Script/")))
			return *It;
	return nullptr;
}

UFactory* RefGen::FactoryFor(UClass* Class, const TCHAR* FactoryName)
{
	UClass* Found = FactoryName ? ClassNamed(FactoryName) : nullptr;
	if (!Found)
		for (TObjectIterator<UClass> It; It && !Found; ++It)
		{
			if (!It->IsChildOf(UFactory::StaticClass()) || It->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
				continue;
			UFactory* Default = It->GetDefaultObject<UFactory>();
			if (Default->GetSupportedClass() == Class && Default->CanCreateNew())
				Found = *It;
		}
	if (!Found)
		return nullptr;
	UFactory* Factory = NewObject<UFactory>(GetTransientPackage(), Found);
	AssignAssets(Factory);
	return Factory;
}

const TArray<UObject*>& RefGen::Candidates()
{
	return GCandidates;
}

void RefGen::AddCandidate(UObject* Asset)
{
	if (Asset)
		GCandidates.AddUnique(Asset);
}

void RefGen::AssignAssets(UObject* Object)
{
	if (!Object)
		return;
	for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
	{
		FObjectProperty* Property = RG_CAST_FIELD(FObjectProperty, *It);
		if (!Property || Property->ArrayDim != 1 || !Property->HasAnyPropertyFlags(CPF_Edit)
			|| Property->HasAnyPropertyFlags(CPF_EditConst | CPF_Deprecated | CPF_Transient | CPF_InstancedReference)
			|| !Property->PropertyClass || Property->PropertyClass->IsChildOf(UClass::StaticClass()))
			continue;
		if (Property->GetObjectPropertyValue_InContainer(Object))
			continue;
		for (UObject* Candidate : GCandidates)
			if (Candidate && Candidate->IsA(Property->PropertyClass) && Candidate != Object)
			{
				Property->SetObjectPropertyValue_InContainer(Object, Candidate);
				break;
			}
	}
}

bool RefGen::TrySetPropertyText(UObject* Object, const TCHAR* PropertyName, const TCHAR* Value)
{
	if (!Object || !RG_FIND_PROPERTY(Object->GetClass(), PropertyName))
		return false;
	return SetPropertyText(Object, PropertyName, Value);
}

TArray<RefGen::FEnumEntry> RefGen::EnumValues(const TCHAR* EnumPath)
{
	TArray<FEnumEntry> Out;
	UEnum* Enum = FindObject<UEnum>(nullptr, EnumPath);
	if (!Enum)
		return Out;
	for (int32 Index = 0; Index < Enum->NumEnums(); ++Index)
	{
		FString Name = Enum->RG_ENUM_NAME_BY_INDEX(Index);
		if (Name.EndsWith(TEXT("_MAX")) || Name.EndsWith(TEXT("MAX")))
			continue;
		int32 Underscore;
		if (Name.FindChar(TEXT('_'), Underscore) && Name.Left(Underscore).ToUpper() == Name.Left(Underscore))
			Name = Name.Mid(Underscore + 1); // drop the TC_ / MSM_ / BLEND_ / MD_ prefix
#if UE_AT_LEAST(4, 16)
		if (Enum->HasMetaData(TEXT("Hidden"), Index))
			continue;
#endif
		FEnumEntry Entry;
		Entry.Name = Name;
		Entry.Value = RG_ENUM_VALUE(Enum, Index);
		Out.Add(Entry);
	}
	return Out;
}

void RefGen::Remember(const TCHAR* Name, UObject* Asset)
{
	if (Asset)
		GRemembered.Add(Name, Asset);
}

UObject* RefGen::Recall(const TCHAR* Name)
{
	UObject** Found = GRemembered.Find(Name);
	return Found ? *Found : nullptr;
}

void RefGen::MakeCatalog(FAssets& A, const FString& SourceDir)
{
	// most specific first: AssignAssets takes the first candidate of a fitting class
	// (the void* hop avoids including every type's header; all of them have UObject as their primary base)
#define RG_OBJ(P) static_cast<UObject*>(static_cast<void*>(P))
	UObject* Ordered[] = { RG_OBJ(A.StaticMesh), RG_OBJ(A.SkeletalMesh), RG_OBJ(A.Skeleton), RG_OBJ(A.PhysicsAsset), RG_OBJ(A.Bend),
		RG_OBJ(A.AnimBlueprint), RG_OBJ(A.BlendSpace2D), RG_OBJ(A.Opaque), RG_OBJ(A.InstanceRed), RG_OBJ(A.Albedo), RG_OBJ(A.Cube),
		RG_OBJ(A.RenderTarget), RG_OBJ(A.LightProfile), RG_OBJ(A.Cue), RG_OBJ(A.Wave), RG_OBJ(A.Attenuation), RG_OBJ(A.SoundClass),
		RG_OBJ(A.Particles), RG_OBJ(A.CurveFloat), RG_OBJ(A.CurveColor), RG_OBJ(A.CurveVector), RG_OBJ(A.Table), RG_OBJ(A.CurveTable),
		RG_OBJ(A.StringTable), RG_OBJ(A.Struct), RG_OBJ(A.Enum), RG_OBJ(A.Sequence), RG_OBJ(A.PhysMaterial), RG_OBJ(A.Collection),
		RG_OBJ(A.Function), RG_OBJ(A.DataAsset) };
#undef RG_OBJ
	for (UObject* Object : Ordered)
		AddCandidate(Object);

	CatTextures(A);
	for (const FGenericAsset& Entry : GGenericAssets)
		if (UObject* Asset = MakeGeneric(Entry, A))
			AddCandidate(Asset);
	CatBlueprints();
	CatMaterials(A);
	CatMeshes(A);
	CatAudio(A, SourceDir);
	MakeFonts(A);
	MakeWidgets(A);
	MakePaper2D(A);
	MakeNiagara(A);
	MakeVectorField(SourceDir);
	// foliage type: needs the mesh, so it is made here and not generically
	if (UClass* FoliageClass = ClassNamed(TEXT("FoliageType_InstancedStaticMesh")))
	{
		UObject* Foliage = NewAsset(TEXT("FT_Ref"), Folder(TEXT("World")), FoliageClass);
		TrySetPropertyText(Foliage, TEXT("Mesh"), *A.StaticMesh->GetPathName());
		Touch(Foliage);
		Remember(TEXT("FT_Ref"), Foliage);
	}
	if (UObject* Grass = Recall(TEXT("LGT_Ref")))
	{
		TrySetPropertyText(Grass, TEXT("GrassVarieties"), *FString::Printf(TEXT("((GrassMesh=\"%s\",GrassDensity=(Default=40.0),StartCullDistance=(Default=2000),EndCullDistance=(Default=4000)))"), *A.StaticMesh->GetPathName()));
		Touch(Grass);
	}
	MakeLandscapeMaterial(A);
}
