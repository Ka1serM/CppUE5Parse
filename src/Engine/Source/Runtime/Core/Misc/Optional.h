#pragma once

#include <new>
#include <utility>

#include "CppUE5Parse/Core/Check.h"
#include "Engine/Source/Runtime/Core/Templates/TypeHash.h"

namespace ue5parse::runtime {

//! The layout is FOptionalProperty's: the value's bytes, then the set flag.
template <class OptionalType>
class TOptional {
public:
    TOptional() = default;
    TOptional(const OptionalType& InValue) { Emplace(InValue); }
    TOptional(OptionalType&& InValue) { Emplace(std::move(InValue)); }
    TOptional(const TOptional& Other) {
        if (Other.bIsSet)
            Emplace(Other.GetValue());
    }
    TOptional(TOptional&& Other) noexcept {
        if (Other.bIsSet)
            Emplace(std::move(Other.GetValue()));
    }
    TOptional& operator=(const TOptional& Other) {
        if (this == &Other)
            return *this;
        Reset();
        if (Other.bIsSet)
            Emplace(Other.GetValue());
        return *this;
    }
    TOptional& operator=(TOptional&& Other) noexcept {
        if (this == &Other)
            return *this;
        Reset();
        if (Other.bIsSet)
            Emplace(std::move(Other.GetValue()));
        return *this;
    }
    TOptional& operator=(const OptionalType& InValue) {
        Reset();
        Emplace(InValue);
        return *this;
    }
    ~TOptional() { Reset(); }

    template <class... ArgTypes>
    OptionalType& Emplace(ArgTypes&&... Args) {
        Reset();
        OptionalType* Value = new (Storage) OptionalType(std::forward<ArgTypes>(Args)...);
        bIsSet = true;
        return *Value;
    }
    void Reset() {
        if (!bIsSet)
            return;
        bIsSet = false;
        GetValue().~OptionalType();
    }

    bool IsSet() const { return bIsSet; }
    explicit operator bool() const { return bIsSet; }
    OptionalType& GetValue() {
        UE5PARSE_CHECK(bIsSet);
        return *std::launder(reinterpret_cast<OptionalType*>(Storage));
    }
    const OptionalType& GetValue() const {
        UE5PARSE_CHECK(bIsSet);
        return *std::launder(reinterpret_cast<const OptionalType*>(Storage));
    }
    OptionalType& operator*() { return GetValue(); }
    const OptionalType& operator*() const { return GetValue(); }
    OptionalType* operator->() { return &GetValue(); }
    const OptionalType* operator->() const { return &GetValue(); }
    const OptionalType& Get(const OptionalType& DefaultValue) const { return bIsSet ? GetValue() : DefaultValue; }
    OptionalType* GetPtrOrNull() { return bIsSet ? &GetValue() : nullptr; }
    const OptionalType* GetPtrOrNull() const { return bIsSet ? &GetValue() : nullptr; }

    bool operator==(const TOptional& Other) const { return bIsSet == Other.bIsSet && (!bIsSet || GetValue() == Other.GetValue()); }
    friend uint32 GetTypeHash(const TOptional& Optional) { return Optional.bIsSet ? GetTypeHash(Optional.GetValue()) : 0; }

private:
    alignas(OptionalType) unsigned char Storage[sizeof(OptionalType)];
    bool bIsSet = false;
};

}
