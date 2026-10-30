#pragma once

#include "ue5parse/runtime/Core/Containers/Array.h"

namespace ue5parse::runtime {

// TEnumAsByte: an old-style enum stored in one byte, as a reflected FByteProperty (UE's Containers/EnumAsByte.h).
template <class TEnum>
class TEnumAsByte {
public:
    TEnumAsByte() = default;
    TEnumAsByte(TEnum InValue) : Value(static_cast<uint8>(InValue)) {}
    explicit TEnumAsByte(int32 InValue) : Value(static_cast<uint8>(InValue)) {}
    explicit TEnumAsByte(uint8 InValue) : Value(InValue) {}

    operator TEnum() const { return static_cast<TEnum>(Value); }
    TEnum GetValue() const { return static_cast<TEnum>(Value); }
    uint8 GetIntValue() const { return Value; }
    bool operator==(TEnum Other) const { return Value == static_cast<uint8>(Other); }
    bool operator==(TEnumAsByte Other) const { return Value == Other.Value; }

private:
    uint8 Value = 0;
};

}
