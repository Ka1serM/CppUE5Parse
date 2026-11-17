#include "RefGenCommon.h"
#include "RefTypes.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"
#include "Curves/CurveVector.h"
#include "Engine/DataTable.h"
#include "Engine/CurveTable.h"
#if __has_include("StructUtils/UserDefinedStruct.h")
#include "StructUtils/UserDefinedStruct.h"
#else
#include "Engine/UserDefinedStruct.h"
#endif
#include "UserDefinedStructure/UserDefinedStructEditorData.h"
#if UE_AT_LEAST(5, 8)
#define REF_DEVNOTES , FString()
#else
#define REF_DEVNOTES
#endif
#include "Engine/UserDefinedEnum.h"
#if UE_AT_LEAST(4, 16) // string tables are 4.16+
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#endif
#include "Kismet2/StructureEditorUtils.h"
#include "Kismet2/EnumEditorUtils.h"
#include "EdGraphSchema_K2.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundCue.h"
#include "EdGraph/EdGraphNode.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundNodeRandom.h"
#include "Sound/SoundNodeMixer.h"
#include "Sound/SoundNodeModulator.h"
#include "Sound/SoundNodeAttenuation.h"
#include "Sound/SoundNodeLooping.h"
#include "Sound/SoundNodeDelay.h"
#include "Factories/SoundCueFactoryNew.h"
#if RG_GAMEPLAY
#include "GameplayTagsManager.h"
#if !UE_AT_LEAST(4, 15)
#include "GameplayTagsModule.h"
#endif
#endif
#include "Misc/FileHelper.h"
#include "UObject/StructOnScope.h"

namespace
{
	using namespace RefGen;

	FEdGraphPinType Pin(const FRGPinName& Category, UObject* SubObject = nullptr, const FRGPinName& SubCategory = FRGPinName())
	{
		FEdGraphPinType Type;
		Type.PinCategory = Category;
		Type.PinSubCategory = SubCategory;
		Type.PinSubCategoryObject = SubObject;
		return Type;
	}

#if RG_GAMEPLAY
	FGameplayTag Tag(const TCHAR* Name)
	{
		return RG_REQUEST_TAG(FName(Name));
	}
#endif
}

void RefGen::MakeData(FAssets& A)
{
	A.CurveFloat = NewAsset<UCurveFloat>(TEXT("C_Float"), TEXT("/Game/Data"));
	A.CurveFloat->FloatCurve.AddKey(0.f, 0.f);
	const FKeyHandle Mid = A.CurveFloat->FloatCurve.AddKey(0.5f, 1.f);
	A.CurveFloat->FloatCurve.SetKeyInterpMode(Mid, RCIM_Cubic);
	A.CurveFloat->FloatCurve.AddKey(1.f, 0.2f);
#if UE_AT_LEAST(4, 9) // no curve extrapolation before 4.9
	A.CurveFloat->FloatCurve.PreInfinityExtrap = RCCE_Cycle;
#endif
	Touch(A.CurveFloat);

	A.CurveColor = NewAsset<UCurveLinearColor>(TEXT("C_Color"), TEXT("/Game/Data"));
	for (int32 Channel = 0; Channel < 4; ++Channel)
	{
		A.CurveColor->FloatCurves[Channel].AddKey(0.f, Channel == 0 ? 1.f : 0.f);
		const FKeyHandle Key = A.CurveColor->FloatCurves[Channel].AddKey(1.f, Channel == 2 ? 1.f : 0.5f);
		A.CurveColor->FloatCurves[Channel].SetKeyInterpMode(Key, RCIM_Constant);
	}
	Touch(A.CurveColor);

	A.CurveVector = NewAsset<UCurveVector>(TEXT("C_Vector"), TEXT("/Game/Data"));
	for (int32 Channel = 0; Channel < 3; ++Channel)
	{
		A.CurveVector->FloatCurves[Channel].AddKey(0.f, float(Channel));
		A.CurveVector->FloatCurves[Channel].AddKey(2.f, float(Channel) * -3.f);
	}
	Touch(A.CurveVector);

#if UE_AT_LEAST(4, 26)
	UPackage* EnumPackage = CreatePackage(TEXT("/Game/Data/E_RefState"));
	UPackage* StructPackage = CreatePackage(TEXT("/Game/Data/S_RefStruct"));
#else
	UPackage* EnumPackage = CreatePackage(nullptr, TEXT("/Game/Data/E_RefState"));
	UPackage* StructPackage = CreatePackage(nullptr, TEXT("/Game/Data/S_RefStruct"));
#endif
	A.Enum = Cast<UUserDefinedEnum>(FEnumEditorUtils::CreateUserDefinedEnum(EnumPackage, TEXT("E_RefState"), RF_Public | RF_Standalone | RF_Transactional));
	if (A.Enum)
	{
		while (A.Enum->NumEnums() - 1 < 3)
			FEnumEditorUtils::AddNewEnumeratorForUserDefinedEnum(A.Enum);
		FEnumEditorUtils::SetEnumeratorDisplayName(A.Enum, 0, RG_ENUM_DISPLAY_NAME(TEXT("Idle")));
		FEnumEditorUtils::SetEnumeratorDisplayName(A.Enum, 1, RG_ENUM_DISPLAY_NAME(TEXT("Running")));
		FEnumEditorUtils::SetEnumeratorDisplayName(A.Enum, 2, RG_ENUM_DISPLAY_NAME(TEXT("Dead")));
		FAssetRegistryModule::AssetCreated(A.Enum);
		Touch(A.Enum);
	}
	else
		Fail(TEXT("user defined enum"));

	A.Struct = FStructureEditorUtils::CreateUserDefinedStruct(StructPackage, TEXT("S_RefStruct"), RF_Public | RF_Standalone | RF_Transactional);
	if (A.Struct)
	{
		FStructureEditorUtils::AddVariable(A.Struct, Pin(UEdGraphSchema_K2::PC_Int));
#if UE_AT_LEAST(5, 0)
		FStructureEditorUtils::AddVariable(A.Struct, Pin(UEdGraphSchema_K2::PC_Real, nullptr, UEdGraphSchema_K2::PC_Float));
#else
		FStructureEditorUtils::AddVariable(A.Struct, Pin(UEdGraphSchema_K2::PC_Float));
#endif
		FStructureEditorUtils::AddVariable(A.Struct, Pin(UEdGraphSchema_K2::PC_String));
		FStructureEditorUtils::AddVariable(A.Struct, Pin(UEdGraphSchema_K2::PC_Struct, TBaseStructure<FVector>::Get()));
		FStructureEditorUtils::AddVariable(A.Struct, Pin(UEdGraphSchema_K2::PC_Byte, A.Enum));
		FStructureEditorUtils::AddVariable(A.Struct, Pin(UEdGraphSchema_K2::PC_SoftObject, UStaticMesh::StaticClass()));
		FEdGraphPinType ArrayPin = Pin(UEdGraphSchema_K2::PC_Name);
		RG_SET_CONTAINER(ArrayPin, EPinContainerType::Array);
		FStructureEditorUtils::AddVariable(A.Struct, ArrayPin);
		const TCHAR* Names[] = { TEXT("Count"), TEXT("Scale"), TEXT("Label"), TEXT("Offset"), TEXT("State"), TEXT("Mesh"), TEXT("Names") };
		TArray<FStructVariableDescription>& Vars = FStructureEditorUtils::GetVarDesc(A.Struct);
		const int32 Existing = Vars.Num();
		for (int32 Index = 0; Index < Existing && Index < UE_ARRAY_COUNT(Names) + 1; ++Index)
		{
			const int32 NameIndex = Index - (Existing - UE_ARRAY_COUNT(Names));
			if (NameIndex >= 0)
				FStructureEditorUtils::RenameVariable(A.Struct, Vars[Index].VarGuid, Names[NameIndex]);
		}
		if (Existing > UE_ARRAY_COUNT(Names))
			FStructureEditorUtils::RemoveVariable(A.Struct, Vars[0].VarGuid);
		FAssetRegistryModule::AssetCreated(A.Struct);
		Touch(A.Struct);
	}
	else
		Fail(TEXT("user defined struct"));

	A.Table = NewAsset<UDataTable>(TEXT("DT_Weapons"), TEXT("/Game/Data"));
	A.Table->RowStruct = FRefTableRow::StaticStruct();
	for (int32 Index = 0; Index < 4; ++Index)
	{
		FRefTableRow Row;
		Row.DisplayName = FText::FromString(FString::Printf(TEXT("Weapon %d"), Index));
		Row.Damage = 10 * (Index + 1);
		Row.Range = 1000.f + Index * 250.f;
		Row.Kind = static_cast<ERefNativeEnum>(Index % 3);
		Row.Asset = A.StaticMesh;
#if RG_GAMEPLAY
		Row.Tag = Tag(TEXT("Ref.Damage.Fire"));
#endif
#if UE_AT_LEAST(4, 16)
		A.Table->AddRow(FName(*FString::Printf(TEXT("Weapon_%d"), Index)), Row);
#else // no AddRow yet: allocate and copy the row into RowMap the way the JSON importer does
		uint8* RowData = (uint8*)FMemory::Malloc(A.Table->RowStruct->GetStructureSize());
		RG_INIT_STRUCT(A.Table->RowStruct, RowData);
		A.Table->RowStruct->CopyScriptStruct(RowData, &Row);
		A.Table->RowMap.Add(FName(*FString::Printf(TEXT("Weapon_%d"), Index)), RowData);
#endif
	}
	Touch(A.Table);

	if (A.Struct)
	{
		A.StructTable = NewAsset<UDataTable>(TEXT("DT_UserStruct"), TEXT("/Game/Data"));
		A.StructTable->RowStruct = A.Struct;
		auto EnumName = [&A](int32 Index) { return A.Enum ? A.Enum->RG_ENUM_NAME_BY_INDEX(FMath::Min(Index, A.Enum->NumEnums() - 2)) : FString(); };
		FString Json = TEXT("[{\"Name\":\"RowA\",\"Count\":3,\"Scale\":1.5,\"Label\":\"Alpha\",\"Offset\":{\"X\":1,\"Y\":2,\"Z\":3},\"State\":\"@E1@\",\"Mesh\":\"/Game/Meshes/SM_Ref.SM_Ref\",\"Names\":[\"a\",\"b\"]},")
			TEXT("{\"Name\":\"RowB\",\"Count\":7,\"Scale\":0.25,\"Label\":\"Beta\",\"Offset\":{\"X\":-1,\"Y\":0,\"Z\":9},\"State\":\"@E2@\",\"Mesh\":\"\",\"Names\":[]}]");
		Json.ReplaceInline(TEXT("@E1@"), *EnumName(1));
		Json.ReplaceInline(TEXT("@E2@"), *EnumName(2));
		const TArray<FString> Problems = A.StructTable->CreateTableFromJSONString(Json);
		for (const FString& Problem : Problems)
			UE_LOG(LogRefGen, Warning, TEXT("DT_UserStruct: %s"), *Problem);
		Touch(A.StructTable);
	}

	A.CurveTable = NewAsset<UCurveTable>(TEXT("CT_Scaling"), TEXT("/Game/Data"));
	A.CurveTable->CreateTableFromCSVString(TEXT("Name,1,5,10,20\nDamage,10,25,60,140\nHealth,100,150,220,400\n"), RCIM_Cubic);
	Touch(A.CurveTable);

#if UE_AT_LEAST(4, 16)
	A.StringTable = NewAsset<UStringTable>(TEXT("ST_Ref"), TEXT("/Game/Data"));
	A.StringTable->GetMutableStringTable()->SetNamespace(TEXT("RefGen"));
	A.StringTable->GetMutableStringTable()->SetSourceString(TEXT("Greeting"), TEXT("Hello reference") REF_DEVNOTES);
	A.StringTable->GetMutableStringTable()->SetSourceString(TEXT("Farewell"), TEXT("Goodbye") REF_DEVNOTES);
	A.StringTable->GetMutableStringTable()->SetMetaData(TEXT("Greeting"), TEXT("Comment"), TEXT("Shown on spawn"));
	Touch(A.StringTable);
#endif

	A.DataAsset = NewAsset<URefDataAsset>(TEXT("DA_Ref"), TEXT("/Game/Data"));
	{
		URefDataAsset* D = A.DataAsset;
		D->bFlag = true;
		D->Byte = 200;
		D->Int8 = -100;
		D->Int16 = -30000;
		D->UInt16 = 60000;
		D->Int32 = -123456789;
		D->UInt32 = 4000000000u;
		D->Int64 = -1234567890123ll;
		D->UInt64 = 12345678901234567ull;
		D->Float = 3.14159f;
		D->Double = 2.718281828459045;
		D->String = TEXT("Reference string with unicode éè中");
		D->Name = TEXT("RefName_42");
		D->Text = FText::FromString(TEXT("Localized text"));
		D->Enum = ERefNativeEnum::Third;
		D->Flags = 5;
		D->Vector = FVector(1, -2, 3.5);
		D->Vector2D = FVector2D(0.5, -0.25);
		D->Vector4 = FVector4(1, 2, 3, 4);
		D->Rotator = FRotator(10, 20, 30);
		D->Quat = FQuat(FRotator(45, 0, 90));
		D->Transform = FTransform(FRotator(0, 90, 0), FVector(100, 200, 300), FVector(1, 2, 3));
		D->LinearColor = FLinearColor(0.1f, 0.2f, 0.3f, 0.4f);
		D->Color = FColor(10, 20, 30, 40);
		D->Box = FBox(FVector(-1, -2, -3), FVector(4, 5, 6));
		D->Guid = FGuid(0x11111111, 0x22222222, 0x33333333, 0x44444444);
		D->DateTime = FDateTime(2026, 11, 11, 12, 30, 0);
		D->Timespan = FTimespan(1, 2, 3, 4);
		D->SoftPath = FSoftObjectPath(A.Albedo);
		D->SoftClassPath = FSoftClassPath(TEXT("/Script/Engine.StaticMeshActor"));
		D->HardObject = A.StaticMesh;
		D->SoftObject = A.SkeletalMesh;
		D->ActorClass = AActor::StaticClass();
#if UE_AT_LEAST(4, 12)
		D->SoftActorClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Script/Engine.PointLight")));
#else // TAssetSubclassOf has no path constructor; same stored path
		D->SoftActorClass = APointLight::StaticClass();
#endif
#if RG_GAMEPLAY
		D->Tag = Tag(TEXT("Ref.Damage.Fire"));
		D->Tags.AddTag(Tag(TEXT("Ref.Damage.Ice")));
		D->Tags.AddTag(Tag(TEXT("Ref.State.Stunned")));
#endif
		D->Inner.Count = 7;
		D->Inner.Label = TEXT("Inner");
		D->Inner.Weights = { 0.1f, 0.2f, 0.7f };
		D->IntArray = { 1, 2, 3, 5, 8, 13 };
		D->StringArray = { TEXT("a"), TEXT(""), TEXT("long string value") };
		D->StructArray.Add(D->Inner);
		RefGen::AddDefaulted(D->StructArray);
		D->ObjectArray = { A.Albedo, A.Normal, nullptr };
		D->NameSet = { TEXT("One"), TEXT("Two"), TEXT("Three") };
		D->NameToInt.Add(TEXT("Alpha"), 1);
		D->NameToInt.Add(TEXT("Beta"), 2);
		D->StringToStruct.Add(TEXT("first"), D->Inner);
		D->IntToSoft.Add(7, A.StaticMesh);
		D->RowHandle.DataTable = A.Table;
		D->RowHandle.RowName = TEXT("Weapon_2");
		RG_RICH_CURVE(D->RuntimeCurve)->AddKey(0.f, 1.f);
		RG_RICH_CURVE(D->RuntimeCurve)->AddKey(1.f, 3.f);
		D->RuntimeCurve.ExternalCurve = A.CurveFloat;
		URefInstancedObject* Sub = NewObject<URefInstancedObject>(D, TEXT("Instanced"));
		Sub->Value = 9.5f;
		Sub->Note = TEXT("instanced subobject");
		D->InstancedSubobject = Sub;
		D->StaticArray[0] = 4;
		D->StaticArray[2] = 6;
		Touch(D);
	}
}

void RefGen::MakeAudio(FAssets& A, const FString& SourceDir)
{
	for (UObject* Object : ImportFile(SourceDir / TEXT("SW_Tone.wav"), TEXT("/Game/Audio"), TEXT("SW_Tone"), nullptr))
		if (USoundWave* Wave = Cast<USoundWave>(Object))
			A.Wave = Wave;
	if (!A.Wave)
		return;

	A.SoundClass = NewAsset<USoundClass>(TEXT("SCL_Ref"), TEXT("/Game/Audio"));
	A.SoundClass->Properties.Volume = 0.8f;
	Touch(A.SoundClass);
	USoundMix* Mix = NewAsset<USoundMix>(TEXT("SMX_Ref"), TEXT("/Game/Audio"));
	FSoundClassAdjuster Adjuster;
	Adjuster.SoundClassObject = A.SoundClass;
	Adjuster.VolumeAdjuster = 0.5f;
	Mix->SoundClassEffects.Add(Adjuster);
	Touch(Mix);
	A.Attenuation = NewAsset<USoundAttenuation>(TEXT("ATT_Ref"), TEXT("/Game/Audio"));
	A.Attenuation->Attenuation.FalloffDistance = 2500.f;
	A.Attenuation->Attenuation.bSpatialize = true;
	Touch(A.Attenuation);
	RG_SET_SOUNDCLASS(A.Wave, A.SoundClass);
	A.Wave->AttenuationSettings = A.Attenuation;
	Touch(A.Wave);

	auto* Factory = NewObject<USoundCueFactoryNew>();
	A.Cue = Create<USoundCue>(TEXT("SC_Ref"), TEXT("/Game/Audio"), Factory);
	if (!A.Cue)
		return;
	USoundCue* Cue = A.Cue;
	auto* PlayerA = Cue->ConstructSoundNode<USoundNodeWavePlayer>();
	RG_SET_WAVE(PlayerA, A.Wave);
	auto* PlayerB = Cue->ConstructSoundNode<USoundNodeWavePlayer>();
	RG_SET_WAVE(PlayerB, A.Wave);
	PlayerB->bLooping = true;
	auto* Random = Cue->ConstructSoundNode<USoundNodeRandom>();
	Random->ChildNodes = { PlayerA, PlayerB };
	Random->Weights = { 0.7f, 0.3f };
	auto* Delay = Cue->ConstructSoundNode<USoundNodeDelay>();
	Delay->DelayMin = 0.1f;
	Delay->DelayMax = 0.3f;
	Delay->ChildNodes = { Random };
	auto* Modulator = Cue->ConstructSoundNode<USoundNodeModulator>();
	Modulator->PitchMin = 0.9f;
	Modulator->PitchMax = 1.1f;
	Modulator->ChildNodes = { Delay };
	auto* Attenuation = Cue->ConstructSoundNode<USoundNodeAttenuation>();
	Attenuation->AttenuationSettings = A.Attenuation;
	Attenuation->ChildNodes = { Modulator };
	auto* Mixer = Cue->ConstructSoundNode<USoundNodeMixer>();
	auto* PlayerC = Cue->ConstructSoundNode<USoundNodeWavePlayer>();
	RG_SET_WAVE(PlayerC, A.Wave);
	Mixer->ChildNodes = { Attenuation, PlayerC };
	Mixer->InputVolume = { 1.f, 0.5f };
	Cue->FirstNode = Mixer;
	RG_SET_SOUNDCLASS(Cue, A.SoundClass);
#if WITH_EDITOR
	for (USoundNode* Node : Cue->AllNodes)
		if (Node && RG_SOUND_GRAPH_NODE(Node))
			RG_SOUND_GRAPH_NODE(Node)->ReconstructNode();
#endif
	Cue->LinkGraphNodesFromSoundNodes();
	Touch(Cue);
}
