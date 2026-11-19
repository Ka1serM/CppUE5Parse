#pragma once

#include <concepts>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ue5parse::runtime {

class FJsonWriter {
public:
    explicit FJsonWriter(std::string& InOutput) : Output(InOutput) {}

    void WriteObjectStart() { WriteOpen('{'); }
    void WriteObjectStart(std::string_view Identifier) {
        WriteIdentifierPrefix(Identifier);
        WriteOpen('{');
    }
    void WriteObjectEnd() { WriteClose('}'); }
    void WriteArrayStart() { WriteOpen('['); }
    void WriteArrayStart(std::string_view Identifier) {
        WriteIdentifierPrefix(Identifier);
        WriteOpen('[');
    }
    void WriteArrayEnd() { WriteClose(']'); }
    void WriteIdentifierPrefix(std::string_view Identifier);

    void WriteNull();
    void WriteValue(bool Value);
    void WriteValue(const char* Value) { WriteValue(std::string_view(Value)); }
    void WriteValue(std::string_view Value);
    template <std::integral T>
    void WriteValue(T Value) {
        if constexpr (std::is_signed_v<T>)
            WriteSigned(Value);
        else
            WriteUnsigned(Value);
    }
    void WriteValue(float Value);
    void WriteValue(double Value);
    template <class T>
    void WriteValue(std::string_view Identifier, const T& Value) {
        WriteIdentifierPrefix(Identifier);
        WriteValue(Value);
    }

private:
    void BeginValue();
    void WriteOpen(char Bracket);
    void WriteClose(char Bracket);
    void WriteNewLine();
    void WriteString(std::string_view Value);
    void WriteSigned(std::int64_t Value);
    void WriteUnsigned(std::uint64_t Value);
    template <std::floating_point T>
    void WriteFloatingPoint(T Value);

    std::string& Output;
    std::vector<bool> ScopeHasItems;
    bool bAfterIdentifier = false;
};

}
