#pragma once

// Shared helpers for the catalog generators (RefGenCatalog*.cpp): every asset, actor and component type an engine
// version has, created once. Classes are looked up by name at runtime, so one source covers all engine versions
// and plugin classes need no link dependency; anything an engine lacks is skipped (and logged).
#include "RefGenCommon.h"

// set by RefGenEditor.Build.cs per module the engine has (0 when missing)
#ifndef RG_WITH_UMG
#define RG_WITH_UMG 0
#endif
#ifndef RG_WITH_UMGEDITOR
#define RG_WITH_UMGEDITOR 0
#endif
#ifndef RG_WITH_PAPER2D
#define RG_WITH_PAPER2D 0
#endif
#ifndef RG_WITH_NIAGARA
#define RG_WITH_NIAGARA 0
#endif
#ifndef RG_WITH_NIAGARAEDITOR
#define RG_WITH_NIAGARAEDITOR 0
#endif
#ifndef RG_WITH_HAIRSTRANDSCORE
#define RG_WITH_HAIRSTRANDSCORE 0
#endif
#ifndef RG_WITH_HAIRSTRANDSEDITOR
#define RG_WITH_HAIRSTRANDSEDITOR 0
#endif
#ifndef RG_WITH_GEOMETRYCOLLECTIONENGINE
#define RG_WITH_GEOMETRYCOLLECTIONENGINE 0
#endif
#ifndef RG_WITH_FOLIAGE
#define RG_WITH_FOLIAGE 0
#endif
#ifndef RG_WITH_LANDSCAPE
#define RG_WITH_LANDSCAPE 0
#endif
#ifndef RG_WITH_BSPUTILS
#define RG_WITH_BSPUTILS 0
#endif

#if UE_AT_LEAST(4, 25)
#define RG_CAST_FIELD(Type, Field) CastField<Type>(Field)
#else
#define RG_CAST_FIELD(Type, Field) Cast<Type>(Field)
#endif
#if UE_AT_LEAST(4, 16)
#define RG_ENUM_VALUE(Enum, Index) (Enum)->GetValueByIndex(Index)
#else
#define RG_ENUM_VALUE(Enum, Index) (Index)
#endif

namespace RefGen
{
	// a class by its short name (no U/A prefix); null when this engine (or its enabled plugins) has none
	UClass* ClassNamed(const TCHAR* Name);
	// a factory that creates new assets of Class (prefers one named FactoryName), configured with the reference assets
	UFactory* FactoryFor(UClass* Class, const TCHAR* FactoryName = nullptr);
	// the reference assets, most specific first: used to fill empty asset references on generic objects
	const TArray<UObject*>& Candidates();
	void AddCandidate(UObject* Asset);
	// fills every empty, editable, non-instanced object reference of Object with the first candidate that fits
	void AssignAssets(UObject* Object);
	// sets a property from text if the property exists (no failure when it doesn't)
	bool TrySetPropertyText(UObject* Object, const TCHAR* PropertyName, const TCHAR* Value);
	struct FEnumEntry
	{
		FString Name; // short name without the enum prefix (BLEND_Additive -> Additive)
		int64 Value;
	};
	// enum values by path ("/Script/Engine.EBlendMode"), without the hidden _MAX entry
	TArray<FEnumEntry> EnumValues(const TCHAR* EnumPath);
	// registers an asset for later lookup by name (maps, other generators)
	void Remember(const TCHAR* Name, UObject* Asset);
	UObject* Recall(const TCHAR* Name);
	template <typename T>
	T* Recall(const TCHAR* Name)
	{
		return Cast<T>(Recall(Name));
	}

	void MakeCatalog(FAssets& A, const FString& SourceDir);
	void PopulateCatalog(UWorld* World, FAssets& A, bool bPartitioned);
	void MakeCatalogMaps(FAssets& A);
	// -Manifest=<file>: writes the reference values of the generated content (RefGenManifest.cpp); returns the failures
	int32 WriteManifest(const FString& File);
}
