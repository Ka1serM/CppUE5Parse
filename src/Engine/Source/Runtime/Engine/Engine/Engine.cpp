#include "Engine/Source/Runtime/Engine/Engine/Engine.h"

#include <mutex>

#include "CppUE5Parse/Core/Check.h"
#include "Engine/Source/Runtime/Core/Misc/ConfigCacheIni.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

namespace {

UObject* CreateGameSingleton() {
    FString ClassName;
    if (!GetEngineIni().GetString("/Script/Engine.Engine", "GameSingletonClassName", ClassName) || ClassName.IsEmpty())
        return nullptr;
    UClass* SingletonClass = StaticLoadClass(UObject::StaticClass(), nullptr, ClassName);
    UE5PARSE_CHECK(SingletonClass != nullptr);
    return NewObject<UObject>(GetTransientPackage(), SingletonClass);
}

}

UObject* GetGameSingleton() {
    static std::mutex Mutex;
    static const FileProvider* CreatedProvider = nullptr;
    static UObject* GameSingleton = nullptr;
    std::lock_guard Lock(Mutex);
    const FileProvider& Provider = GetRuntimeProvider();
    if (CreatedProvider != &Provider) {
        GameSingleton = CreateGameSingleton();
        CreatedProvider = &Provider;
    }
    return GameSingleton;
}

}
