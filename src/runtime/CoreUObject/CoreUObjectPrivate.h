#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"

namespace ue5parse {
struct PropertyType;
}

// Helpers shared by the CoreUObject sources; not part of the public API.
namespace ue5parse::runtime {

// A runtime property from a mappings type tree (native classes and structs).
FProperty* CreatePropertyFromMappings(FName Name, const ue5parse::PropertyType& Type, int32 ArrayDim, FFieldVariant Owner);

// StaticAllocateObject's identity handoff and registration for memory the caller allocated (the native type objects,
// whose metaclass may still be unlinked during the bootstrap).
void SetPendingIdentity(UObjectBase* Object, UClass* Class, UObject* Outer, FName Name, EObjectFlags Flags);

// The global lock of loading and class building. Recursive: loads nest. As in UE's Zen loader, the reads, the
// decompression and the package headers run in parallel, and the objects are made and serialized one at a time: a
// thread lets the lock go while it reads (FLoaderUnlock) and while it waits for an object that another thread loads.
class FLoaderMutex {
public:
    void lock();
    void unlock();
    // Lets go of all of this thread's depth and returns it: 0 when the thread does not hold the lock.
    int32 Release();
    void Reacquire(int32 Depth);
    // With the lock held: lets it go until Done() holds. Done() runs with the lock's mutex held. The thread counts as
    // a paused load while it waits.
    template <typename FDone>
    void Wait(FDone Done);
    // With the lock held: waits until no other thread is paused in the middle of a load.
    void WaitForPausedLoads();
    // With the lock held: wakes the waiting threads, after a change to what they wait for.
    void NotifyAll() { Changed.notify_all(); }

private:
    std::mutex Mutex;
    std::condition_variable Changed;
    std::atomic<std::thread::id> Owner;
    int32 Depth = 0;
    // The threads that let the lock go in the middle of a load. The objects they hold must stay.
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

// Lets the loader lock go for a read or a decompression, and takes it again at the end of the scope. Does nothing when
// the thread does not hold the lock.
class FLoaderUnlock {
public:
    FLoaderUnlock() : Depth(GetLoaderMutex().Release()) {}
    ~FLoaderUnlock() { GetLoaderMutex().Reacquire(Depth); }
    FLoaderUnlock(const FLoaderUnlock&) = delete;
    FLoaderUnlock& operator=(const FLoaderUnlock&) = delete;

private:
    int32 Depth;
};

// With the loader lock held: waits while another thread loads the object, so that the caller gets it whole, as UE's
// loader waits for the imports of a package. When that thread waits (through others) for this one, a cycle across
// threads, the object is taken as it is, as one thread takes an object of a cycle that it still serializes.
void WaitForLoad(const UObject* Object);

// UObject* <-> 8-byte script pointer slots.
inline ScriptPointerType ToScriptPointer(const void* Pointer) { return static_cast<ScriptPointerType>(reinterpret_cast<uintptr_t>(Pointer)); }

// Tracks the objects loaded during the outermost load, so that PostLoad runs once the whole load finished (UE's
// EndLoad).
class FLoadScope {
public:
    FLoadScope();
    ~FLoadScope();
    FLoadScope(const FLoadScope&) = delete;
    FLoadScope& operator=(const FLoadScope&) = delete;
    static void AddLoadedObject(UObject* Object);
};

}
