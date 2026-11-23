#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>

#include "Engine/Plugins/Runtime/ComputeFramework/Source/ComputeFramework/ComputeFramework/ShaderParamTypeDefinition.h"
#include "Test.h"

namespace {

namespace rt = ue5parse::runtime;
using ue5parse::EGame;
using rt::EShaderFundamentalDimensionType;
using rt::EShaderFundamentalType;

class Writer {
public:
    template <typename T>
    void put(T value) {
        std::size_t at = bytes.size();
        bytes.resize(at + sizeof(T));
        std::memcpy(bytes.data() + at, &value, sizeof(T));
    }

    void name(std::string_view text) {
        put<std::int32_t>(static_cast<std::int32_t>(text.size() + 1));
        for (char c : text)
            put<char>(c);
        put<char>(0);
    }

    std::vector<std::byte> bytes;
};

class NameArchive : public rt::FMemoryReaderView {
public:
    using rt::FMemoryReaderView::FMemoryReaderView;
    using rt::FMemoryReaderView::operator<<;

    rt::FArchive& operator<<(rt::FName& Value) override {
        rt::FString Text;
        *this << Text;
        Value = rt::FName(Text.ToUtf8());
        return *this;
    }
};

void putFundamental(Writer& w, EShaderFundamentalType Type, bool bIsDynamicArray, EShaderFundamentalDimensionType Dimension) {
    w.put(Type);
    w.put<std::uint32_t>(bIsDynamicArray ? 1 : 0);
    w.put(Dimension);
}

}

TEST(shaderValueTypeHandleReadsADynamicArrayOfVectors) {
    Writer w;
    putFundamental(w, EShaderFundamentalType::Float, true, EShaderFundamentalDimensionType::Vector);
    w.put<std::uint8_t>(4);
    NameArchive Ar(w.bytes);
    Ar.SetGame(EGame::GAME_UE5_8);
    Ar.SetUnversioned(true);
    rt::FShaderValueTypeHandle Handle;
    CHECK(Handle.Serialize(Ar));
    CHECK(!Ar.IsError());
    CHECK(Ar.AtEnd());
    REQUIRE(Handle.IsValid());
    CHECK(Handle.ValueTypePtr->Type == EShaderFundamentalType::Float);
    CHECK(Handle.ValueTypePtr->bIsDynamicArray);
    CHECK(Handle.ValueTypePtr->DimensionType == EShaderFundamentalDimensionType::Vector);
    CHECK(Handle.ValueTypePtr->VectorElemCount == 4);
}

TEST(shaderValueTypeHandleReadsNestedStructElements) {
    Writer w;
    w.put(EShaderFundamentalType::Struct);
    w.put<std::uint32_t>(0);
    w.name("Outer");
    w.put<std::int32_t>(2);
    w.name("Transform");
    putFundamental(w, EShaderFundamentalType::Uint, false, EShaderFundamentalDimensionType::Matrix);
    w.put<std::uint8_t>(3);
    w.put<std::uint8_t>(4);
    w.name("Inner");
    w.put(EShaderFundamentalType::Struct);
    w.put<std::uint32_t>(1);
    w.name("InnerType");
    w.put<std::int32_t>(1);
    w.name("Flag");
    putFundamental(w, EShaderFundamentalType::Bool, false, EShaderFundamentalDimensionType::Scalar);
    NameArchive Ar(w.bytes);
    Ar.SetGame(EGame::GAME_UE5_4);
    Ar.SetUnversioned(true);
    rt::FShaderValueTypeHandle Handle;
    CHECK(Handle.Serialize(Ar));
    CHECK(!Ar.IsError());
    CHECK(Ar.AtEnd());
    REQUIRE(Handle.IsValid());
    const rt::FShaderValueType& Outer = *Handle.ValueTypePtr;
    CHECK(Outer.Name == rt::FName("Outer"));
    REQUIRE(Outer.StructElements.Num() == 2);
    const rt::FShaderValueType& Transform = *Outer.StructElements[0].Type.ValueTypePtr;
    CHECK(Outer.StructElements[0].Name == rt::FName("Transform"));
    CHECK(Transform.Type == EShaderFundamentalType::Uint);
    CHECK(Transform.MatrixRowCount == 3);
    CHECK(Transform.MatrixColumnCount == 4);
    const rt::FShaderValueType& Inner = *Outer.StructElements[1].Type.ValueTypePtr;
    CHECK(Inner.bIsDynamicArray);
    REQUIRE(Inner.StructElements.Num() == 1);
    CHECK(Inner.StructElements[0].Type.ValueTypePtr->Type == EShaderFundamentalType::Bool);
}

TEST(shaderValueTypeHandleHasNoDynamicArrayFlagBeforeUE51) {
    Writer w;
    w.put(EShaderFundamentalType::Int);
    w.put(EShaderFundamentalDimensionType::Scalar);
    NameArchive Ar(w.bytes);
    Ar.SetGame(EGame::GAME_UE5_0);
    Ar.SetUnversioned(true);
    rt::FShaderValueTypeHandle Handle;
    CHECK(Handle.Serialize(Ar));
    CHECK(!Ar.IsError());
    CHECK(Ar.AtEnd());
    REQUIRE(Handle.IsValid());
    CHECK(Handle.ValueTypePtr->Type == EShaderFundamentalType::Int);
    CHECK(!Handle.ValueTypePtr->bIsDynamicArray);
}

TEST(shaderValueTypeHandleRejectsTooDeepStructs) {
    Writer w;
    for (int Depth = 0; Depth < 70; Depth++) {
        w.put(EShaderFundamentalType::Struct);
        w.put<std::uint32_t>(0);
        w.name("S");
        w.put<std::int32_t>(1);
        w.name("E");
    }
    putFundamental(w, EShaderFundamentalType::Bool, false, EShaderFundamentalDimensionType::Scalar);
    NameArchive Ar(w.bytes);
    Ar.SetGame(EGame::GAME_UE5_8);
    Ar.SetUnversioned(true);
    rt::FShaderValueTypeHandle Handle;
    Handle.Serialize(Ar);
    CHECK(Ar.IsError());
    CHECK(!Handle.IsValid());
}
