#include "RefGenCommon.h"
#if UE_AT_LEAST(4, 20)
#include "AssetImportTask.h"
#else
#include "AutomatedAssetImportData.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "HAL/FileManager.h"
#endif
#include "UObject/UObjectGlobals.h"
#if UE_AT_LEAST(4, 15)
#include "Misc/Paths.h"
#endif

namespace
{
	int32 GFailures = 0;
}

IAssetTools& RefGen::Tools()
{
	return FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
}

void RefGen::Fail(const FString& Message)
{
	++GFailures;
	UE_LOG(LogRefGen, Error, TEXT("REFGEN FAIL: %s"), *Message);
}

int32 RefGen::Failures()
{
	return GFailures;
}

UObject* RefGen::Create(const FString& Name, const FString& Path, UClass* Class, UFactory* Factory)
{
	UObject* Asset = Tools().CreateAsset(Name, Path, Class, Factory);
	if (!Asset)
		Fail(FString::Printf(TEXT("create %s/%s (%s)"), *Path, *Name, *Class->GetName()));
	return Asset;
}

UObject* RefGen::NewAsset(const FString& Name, const FString& Path, UClass* Class)
{
	const FString PackageName = Path / Name;
#if UE_AT_LEAST(4, 26)
	UPackage* Package = CreatePackage(*PackageName);
#else
	UPackage* Package = CreatePackage(nullptr, *PackageName);
#endif
	UObject* Asset = NewObject<UObject>(Package, Class, FName(*Name), RF_Public | RF_Standalone | RF_Transactional);
	FAssetRegistryModule::AssetCreated(Asset);
	Package->MarkPackageDirty();
	return Asset;
}

void RefGen::Touch(UObject* Asset)
{
	if (!Asset)
		return;
	Asset->PostEditChange();
	Asset->MarkPackageDirty();
}

#if !UE_AT_LEAST(4, 7)
#include "EngineAnalytics.h"
#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProvider.h"
namespace
{
	// 4.6's FBX importer records analytics through FEngineAnalytics::GetProvider() without checking IsAvailable(), and
	// commandlets never initialize analytics (the check() kills the import). Both statics are private and the init is
	// not exported, so read their addresses out of the exported GetProvider() and install a no-op provider meanwhile.
	class FNullAnalytics : public IAnalyticsProvider
	{
	public:
		virtual bool StartSession(const TArray<FAnalyticsEventAttribute>&) override { return true; }
		virtual void EndSession() override {}
		virtual FString GetSessionID() const override { return FString(); }
		virtual bool SetSessionID(const FString&) override { return true; }
		virtual void FlushEvents() override {}
		virtual void SetUserID(const FString&) override {}
		virtual FString GetUserID() const override { return FString(); }
		virtual void RecordEvent(const FString&, const TArray<FAnalyticsEventAttribute>&) override {}
	};

	struct FAnalyticsGuard
	{
		bool* Initialized = nullptr;
		IAnalyticsProvider** Provider = nullptr;
		bool OldInitialized = false;
		IAnalyticsProvider* OldProvider = nullptr;
		FNullAnalytics Null;

		FAnalyticsGuard()
		{
			// sub rsp,28h / cmp byte ptr [rip+A],0 / je / mov rax,[rip+B]  (A: bIsInitialized, B: Analytics' object pointer)
			const uint8* Code = reinterpret_cast<const uint8*>(&FEngineAnalytics::GetProvider);
			static const uint8 Head[] = { 0x48, 0x83, 0xEC, 0x28, 0x80, 0x3D };
			if (FMemory::Memcmp(Code, Head, sizeof(Head)) != 0 || Code[10] != 0x00 || Code[11] != 0x74 || Code[13] != 0x48 || Code[14] != 0x8B || Code[15] != 0x05)
			{
				UE_LOG(LogRefGen, Warning, TEXT("unexpected FEngineAnalytics::GetProvider code, FBX imports may assert"));
				return;
			}
			Initialized = const_cast<bool*>(reinterpret_cast<const bool*>(Code + 11 + *reinterpret_cast<const int32*>(Code + 6)));
			Provider = const_cast<IAnalyticsProvider**>(reinterpret_cast<IAnalyticsProvider* const*>(Code + 20 + *reinterpret_cast<const int32*>(Code + 16)));
			OldInitialized = *Initialized;
			OldProvider = *Provider;
			if (!OldInitialized || !OldProvider)
			{
				*Initialized = true;
				*Provider = &Null;
			}
		}
		~FAnalyticsGuard()
		{
			if (Initialized)
			{
				*Initialized = OldInitialized;
				*Provider = OldProvider;
			}
		}
	};
}
#endif

#if !UE_AT_LEAST(4, 4)
// UFbxFactory::bDetectImportTypeOnImport is protected: explicit instantiation may name it (the usual member pointer access)
struct FFbxDetectTypeTag
{
	typedef bool UFbxFactory::*Type;
	friend Type Get(FFbxDetectTypeTag);
};
template <typename Tag, typename Tag::Type Member>
struct TFbxMemberAccess
{
	friend typename Tag::Type Get(Tag) { return Member; }
};
template struct TFbxMemberAccess<FFbxDetectTypeTag, &UFbxFactory::bDetectImportTypeOnImport>;
#endif

TArray<UObject*> RefGen::ImportFile(const FString& File, const FString& Path, const FString& Name, UObject* Options)
{
#if !UE_AT_LEAST(4, 7)
	FAnalyticsGuard AnalyticsGuard;
#endif
#if UE_AT_LEAST(4, 20)
	UAssetImportTask* Task = NewObject<UAssetImportTask>();
	Task->Filename = File;
	Task->DestinationPath = Path;
	Task->DestinationName = Name;
	Task->bAutomated = true;
	Task->bReplaceExisting = true;
	Task->bSave = false;
	Task->Options = Options;
	Tools().ImportAssetTasks({ Task });
	TArray<UObject*> Result;
	for (const FString& ObjectPath : Task->ImportedObjectPaths)
		if (UObject* Object = LoadObject<UObject>(nullptr, *ObjectPath))
			Result.Add(Object);
#else
	// no UAssetImportTask before 4.20: automated import with an explicit factory carrying the options. The asset is
	// named after the file, so import a copy named <Name>.<ext> when they differ.
	FString Source = File;
	if (FPaths::GetBaseFilename(File) != Name)
	{
		const FString Dir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("RefGenImport"));
		IFileManager::Get().MakeDirectory(*Dir, true);
		Source = Dir / Name + FPaths::GetExtension(File, true);
		IFileManager::Get().Copy(*Source, *File);
	}
#if !UE_AT_LEAST(4, 14)
	// 4.13: no automated import data; drive the factory directly (GIsAutomationTesting suppresses the FBX dialog)
	TArray<UObject*> Result;
	{
		UFactory* Factory = nullptr;
		if (UFbxImportUI* Ui = RG_AS_FBX_IMPORT_UI(Options))
		{
			UFbxFactory* Fbx = RG_NEW_FBX_FACTORY();
			Fbx->ImportUI = Ui;
#if !UE_AT_LEAST(4, 4) // 4.3 re-detects the type from the file, which turns animation imports into (failing) mesh imports
			Fbx->*Get(FFbxDetectTypeTag()) = false;
#endif
			Factory = Fbx;
		}
		else
		{
			for (TObjectIterator<UClass> It; It && !Factory; ++It)
				if (It->IsChildOf(UFactory::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract))
				{
					UFactory* Candidate = It->GetDefaultObject<UFactory>();
					const FString Ext = FPaths::GetExtension(Source).ToLower();
					bool bExtension = false;
					for (const FString& Format : Candidate->Formats)
						bExtension |= Format.StartsWith(Ext + TEXT(";"), ESearchCase::IgnoreCase);
					if (Candidate->bEditorImport && bExtension && !It->GetName().Contains(TEXT("Reimport")))  // (FactoryCanImport only handles .t3d in 4.13)
						Factory = NewObject<UFactory>(GetTransientPackage(), *It);
				}
		}
		if (Factory)
		{
			TArray<UObject*> Keep = { Factory, Options };
			for (UObject* Object : Keep)
				if (Object)
					Object->AddToRoot();
			TGuardValue<bool> Automation(GIsAutomationTesting, true);
			Source = FPaths::ConvertRelativePathToFull(Source);
			UPackage* Package = CreatePackage(nullptr, *(Path / Name));
			bool bCanceled = false;
			UObject* Object = UFactory::StaticImportObject(Factory->ResolveSupportedClass(), Package, FName(*Name),
				RF_Public | RF_Standalone | RF_Transactional, bCanceled, *Source, nullptr, Factory);
			for (UObject* Kept : Keep)
				if (Kept)
					Kept->RemoveFromRoot();
			if (Object)
			{
				FAssetRegistryModule::AssetCreated(Object);
				Object->MarkPackageDirty();
				Result.Add(Object);
			}
		}
	}
#else
	UAutomatedAssetImportData* Data = NewObject<UAutomatedAssetImportData>();
	Data->Filenames.Add(Source);
	Data->DestinationPath = Path;
	Data->bReplaceExisting = true;
	if (UFbxImportUI* Ui = RG_AS_FBX_IMPORT_UI(Options))
	{
		UFbxFactory* Factory = RG_NEW_FBX_FACTORY();
		Factory->ImportUI = Ui;
		Data->Factory = Factory;
	}
	// the import can run a GC: keep the import data, factory and options alive
	TArray<UObject*> Keep = { Data, Data->Factory, Options };
	for (UObject* Object : Keep)
		if (Object)
			Object->AddToRoot();
	TArray<UObject*> Result = Tools().ImportAssetsAutomated(RG_IMPORT_DATA(Data));
	for (UObject* Object : Keep)
		if (Object)
			Object->RemoveFromRoot();
	// skeletal imports return nothing here even when they succeed: look the asset up
	if (Result.Num() == 0)
		if (UObject* Object = LoadObject<UObject>(nullptr, *(Path / Name + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet))
			Result.Add(Object);
#endif
#endif
	if (Result.Num() == 0)
		Fail(FString::Printf(TEXT("import %s"), *File));
	return Result;
}

bool RefGen::SetPropertyText(UObject* Object, const TCHAR* PropertyName, const TCHAR* Value)
{
	FProperty* Property = RG_FIND_PROPERTY(Object->GetClass(), PropertyName);
	if (!Property)
	{
		Fail(FString::Printf(TEXT("property %s on %s"), PropertyName, *Object->GetName()));
		return false;
	}
#if UE_AT_LEAST(5, 1)
	const TCHAR* End = Property->ImportText_InContainer(Value, Object, Object, PPF_None);
#else
	const TCHAR* End = Property->ImportText(Value, Property->ContainerPtrToValuePtr<void>(Object), PPF_None, Object);
#endif
	if (!End)
		Fail(FString::Printf(TEXT("import text %s=%s on %s"), PropertyName, Value, *Object->GetName()));
	return End != nullptr;
}
