#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"

namespace ue5parse {

struct PropertyType;

}

namespace ue5parse::runtime {

FProperty* CreatePropertyFromMappings(FName Name, const ue5parse::PropertyType& Type, int32 ArrayDim, FFieldVariant Owner);

void SetPendingIdentity(UObjectBase* Object, UClass* Class, UObject* Outer, FName Name, EObjectFlags Flags);

//! Recursive: loads nest. A thread lets it go while it reads and while it waits for an object that another thread loads.
class FLoaderMutex {
public:
    void lock();
    void unlock();
    int32 Release();
    void Reacquire(int32 Depth);
    template <typename FDone>
    void Wait(FDone Done);
    void WaitForPausedLoads();
    void NotifyAll() { Changed.notify_all(); }

private:
    std::mutex Mutex;
    std::condition_variable Changed;
    std::atomic<std::thread::id> Owner;
    int32 Depth = 0;
    int32 PausedLoads = 0;
};

FLoaderMutex& GetLoaderMutex();

template <typename FDone>
void FLoaderMutex::Wait(FDone Done) {
    std::unique_lock Lock(Mutex, std::adopt_lock);
    const int32 Saved = Depth;
    Owner.store(std::thread::id(), std::memory_order_relaxed);
    Depth = 0;
    PausedLoads++;
    Changed.wait(Lock, Done);
    PausedLoads--;
    Owner.store(std::this_thread::get_id(), std::memory_order_relaxed);
    Depth = Saved;
    Lock.release();
    Changed.notify_all();
}

class FLoaderUnlock {
public:
    FLoaderUnlock() : Depth(GetLoaderMutex().Release()) {}
    ~FLoaderUnlock() { GetLoaderMutex().Reacquire(Depth); }
    FLoaderUnlock(const FLoaderUnlock&) = delete;
    FLoaderUnlock& operator=(const FLoaderUnlock&) = delete;

private:
    int32 Depth;
};

void WaitForLoad(const UObject* Object);

inline ScriptPointerType ToScriptPointer(const void* Pointer) { return static_cast<ScriptPointerType>(reinterpret_cast<uintptr_t>(Pointer)); }

class FLoadScope {
public:
    FLoadScope();
    ~FLoadScope();
    FLoadScope(const FLoadScope&) = delete;
    FLoadScope& operator=(const FLoadScope&) = delete;
    static void AddLoadedObject(UObject* Object);
};

}
