#include "Engine/Source/Runtime/Json/Serialization/JsonWriter.h"

#include <charconv>
#include <cmath>

namespace ue5parse::runtime {

void FJsonWriter::WriteIdentifierPrefix(std::string_view Identifier) {
    BeginValue();
    WriteString(Identifier);
    Output += ": ";
    bAfterIdentifier = true;
}

void FJsonWriter::WriteNull() {
    BeginValue();
    Output += "null";
}

void FJsonWriter::WriteValue(bool Value) {
    BeginValue();
    Output += Value ? "true" : "false";
}

void FJsonWriter::WriteValue(std::string_view Value) {
    BeginValue();
    WriteString(Value);
}

void FJsonWriter::BeginValue() {
    if (bAfterIdentifier) {
        bAfterIdentifier = false;
        return;
    }
    if (ScopeHasItems.empty())
        return;
    if (ScopeHasItems.back())
        Output += ',';
    ScopeHasItems.back() = true;
    WriteNewLine();
}

void FJsonWriter::WriteOpen(char Bracket) {
    BeginValue();
    Output += Bracket;
    ScopeHasItems.push_back(false);
}

void FJsonWriter::WriteClose(char Bracket) {
    const bool bHadItems = ScopeHasItems.back();
    ScopeHasItems.pop_back();
    if (bHadItems)
        WriteNewLine();
    Output += Bracket;
}

void FJsonWriter::WriteNewLine() {
    Output += '\n';
    Output.append(ScopeHasItems.size() * 2, ' ');
}

void FJsonWriter::WriteString(std::string_view Value) {
    static constexpr char Hex[] = "0123456789abcdef";
    Output += '"';
    for (const char Char : Value) {
        const auto Byte = static_cast<unsigned char>(Char);
        if (Char == '"' || Char == '\\') {
            Output += '\\';
            Output += Char;
        } else if (Char == '\n') {
            Output += "\\n";
        } else if (Char == '\r') {
            Output += "\\r";
        } else if (Char == '\t') {
            Output += "\\t";
        } else if (Byte < 0x20) {
            Output += "\\u00";
            Output += Hex[Byte >> 4];
            Output += Hex[Byte & 15];
        } else {
            Output += Char;
        }
    }
    Output += '"';
}

void FJsonWriter::WriteSigned(std::int64_t Value) {
    BeginValue();
    char Buffer[24];
    Output.append(Buffer, std::to_chars(Buffer, Buffer + sizeof(Buffer), Value).ptr);
}

void FJsonWriter::WriteUnsigned(std::uint64_t Value) {
    BeginValue();
    char Buffer[24];
    Output.append(Buffer, std::to_chars(Buffer, Buffer + sizeof(Buffer), Value).ptr);
}

//! JSON has no NaN or infinity; they are written as the strings that Newtonsoft.Json writes.
template <std::floating_point T>
void FJsonWriter::WriteFloatingPoint(T Value) {
    if (std::isnan(Value)) {
        WriteValue("NaN");
        return;
    }
    if (std::isinf(Value)) {
        WriteValue(Value > 0 ? "Infinity" : "-Infinity");
        return;
    }
    BeginValue();
    char Buffer[32];
    Output.append(Buffer, std::to_chars(Buffer, Buffer + sizeof(Buffer), Value).ptr);
}

void FJsonWriter::WriteValue(float Value) {
    WriteFloatingPoint(Value);
}

void FJsonWriter::WriteValue(double Value) {
    WriteFloatingPoint(Value);
}

}
