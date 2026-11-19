// Pre-4.18 names of the soft reference types (and other 4.18 renames), for the generator's .cpp files.
// (.inl on purpose: ue4_compat.py rewrites these names in every .h for UnrealHeaderTool)
#if !UE_AT_LEAST(4, 18)
#include "Misc/StringAssetReference.h"
#include "Misc/StringClassReference.h"
#include "UObject/AssetPtr.h"
using FSoftObjectPath = FStringAssetReference;
using FSoftClassPath = FStringClassReference;
template <class T> using TSoftObjectPtr = TAssetPtr<T>;
template <class T> using TSoftClassPtr = TAssetSubclassOf<T>;
// renamed in 4.18 as well
#if UE_AT_LEAST(4, 9)
#define PC_SoftObject PC_Asset
#define PC_SoftClass PC_AssetClass
#else // 4.8 has no asset pin categories: plain object/class references
#define PC_SoftObject PC_Object
#define PC_SoftClass PC_Class
#endif
#define ProjectContentDir GameContentDir
#define ProjectSavedDir GameSavedDir
#endif
