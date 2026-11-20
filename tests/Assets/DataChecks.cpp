#include <cstdlib>
#include <set>

#include "AssetSuite.h"
#include "Engine/Source/Runtime/Engine/Curves/CurveFloat.h"
#include "Engine/Source/Runtime/Engine/Curves/CurveLinearColor.h"
#include "Engine/Source/Runtime/Engine/Curves/CurveVector.h"
#include "Engine/Source/Runtime/Engine/Engine/CurveTable.h"
#include "Engine/Source/Runtime/Engine/Engine/DataTable.h"
#include "Engine/Source/Runtime/Engine/Engine/UserDefinedEnum.h"

namespace test::assets {

namespace {

float parseTime(const std::string& time) {
    return std::strtof(time.c_str(), nullptr);
}

void checkFloatCurve(const std::string& path, const Json& value) {
    if (rt::UCurveFloat* curve = loadObject<rt::UCurveFloat>(path))
        for (const auto& [time, expected] : value.getObject())
            checkNear(curve->GetFloatValue(parseTime(time)), expected.getNumber(), valueTolerance, describe(path, time));
}

void checkVectorCurve(const std::string& path, const Json& value) {
    if (rt::UCurveVector* curve = loadObject<rt::UCurveVector>(path))
        for (const auto& [time, expected] : value.getObject())
            checkVector(curve->GetVectorValue(parseTime(time)), expected, valueTolerance, describe(path, time));
}

void checkColorCurve(const std::string& path, const Json& value) {
    if (rt::UCurveLinearColor* curve = loadObject<rt::UCurveLinearColor>(path))
        for (const auto& [time, expected] : value.getObject())
            checkColor(curve->GetUnadjustedLinearColorValue(parseTime(time)), expected, describe(path, time));
}

void checkCurveTable(const std::string& path, const Json& value) {
    rt::UCurveTable* table = loadObject<rt::UCurveTable>(path);
    if (table == nullptr)
        return;
    for (const auto& [key, expected] : value.getObject()) {
        std::size_t at = key.find('@');
        const rt::FRealCurve* curve = table->FindCurve(rt::FName(key.substr(0, at)));
        if (curve == nullptr) {
            fail(describe(path, key), "no such row");
            continue;
        }
        checkNear(curve->Eval(parseTime(key.substr(at + 1))), expected.getNumber(), valueTolerance, describe(path, key));
    }
}

void checkDataTableRows(const std::string& path, const Json& value) {
    rt::UDataTable* table = loadObject<rt::UDataTable>(path);
    if (table == nullptr)
        return;
    std::set<std::string> actual;
    for (const rt::FName& row : table->GetRowNames())
        actual.insert(row.toString());
    std::set<std::string> expected;
    for (const Json& row : value.getArray())
        expected.insert(row.getString());
    CHECK(actual == expected);
}

std::string escapeText(std::string_view text) {
    std::string escaped;
    for (char c : text) {
        if (c == '\\')
            escaped += "\\\\";
        else if (c == '\n')
            escaped += "\\n";
        else if (c == '\r')
            escaped += "\\r";
        else if (c == '\t')
            escaped += "\\t";
        else if (c == '\'')
            escaped += "\\'";
        else if (c == '"')
            escaped += "\\\"";
        else
            escaped += c;
    }
    return escaped;
}

std::string quote(std::string_view text) {
    return "\"" + escapeText(text) + "\"";
}

//! UE's UEnum::GetNameStringByIndex drops the "EType::" scope, and a user defined enum authors the source string of its display name.
std::string getAuthoredEnumName(const rt::UEnum* enumType, rt::int64 value) {
    std::string name = enumType->GetNameStringByValue(value).ToUtf8();
    std::size_t scope = name.rfind("::");
    if (scope != std::string::npos)
        name = name.substr(scope + 2);
    const auto* userEnum = rt::Cast<rt::UUserDefinedEnum>(enumType);
    if (userEnum == nullptr)
        return name;
    const rt::FText* displayName = userEnum->DisplayNameMap.Find(rt::FName(enumType->GetNameStringByValue(value).ToUtf8()));
    return displayName == nullptr ? name : displayName->ToString().ToUtf8();
}

//! Follows FProperty::ExportText_Direct with PPF_ExternalEditor, as DataTableUtils::GetPropertyValueAsString of UE 5.4 calls it.
std::optional<std::string> exportExternalText(const rt::FProperty* property, const void* value, bool delimited) {
    if (const auto* boolean = rt::CastField<rt::FBoolProperty>(property))
        return boolean->GetPropertyValue(value) ? "True" : "False";
    if (const auto* enumProperty = rt::CastField<rt::FEnumProperty>(property))
        return getAuthoredEnumName(enumProperty->GetEnum(), enumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(value));
    if (const auto* numeric = rt::CastField<rt::FNumericProperty>(property)) {
        if (numeric->IsEnum())
            return getAuthoredEnumName(numeric->GetIntPropertyEnum(), numeric->GetSignedIntPropertyValue(value));
        char text[64];
        if (numeric->IsFloatingPoint())
            std::snprintf(text, sizeof(text), "%f", numeric->GetFloatingPointPropertyValue(value));
        else
            std::snprintf(text, sizeof(text), "%lld", static_cast<long long>(numeric->GetSignedIntPropertyValue(value)));
        return text;
    }
    if (rt::CastField<rt::FNameProperty>(property)) {
        const rt::FName& name = *static_cast<const rt::FName*>(value);
        if (!delimited)
            return name.toString();
        return name.isNone() ? "\"\"" : quote(name.toString());
    }
    if (rt::CastField<rt::FStrProperty>(property)) {
        std::string text = static_cast<const rt::FString*>(value)->ToUtf8();
        return delimited ? quote(text) : text;
    }
    if (rt::CastField<rt::FTextProperty>(property)) {
        const rt::FText& text = *static_cast<const rt::FText*>(value);
        std::string display = text.ToString().ToUtf8();
        if (text.IsFromStringTable())
            return std::nullopt;
        if (text.IsCultureInvariant())
            return "INVTEXT(" + quote(display) + ")";
        if (!text.GetKey().IsEmpty())
            return "NSLOCTEXT(" + quote(text.GetNamespace().ToUtf8()) + ", " + quote(text.GetKey().ToUtf8()) + ", " + quote(display) + ")";
        return delimited ? quote(display) : display;
    }
    if (delimited)
        return std::nullopt;
    if (rt::CastField<rt::FSoftObjectProperty>(property)) {
        const rt::FSoftObjectPath& path = static_cast<const rt::FSoftObjectPtr*>(value)->GetUniqueID();
        return path.IsNull() ? "None" : path.ToString().ToUtf8();
    }
    if (const auto* object = rt::CastField<rt::FObjectPropertyBase>(property))
        return getPathName(object->GetObjectPropertyValue(value));
    if (const auto* array = rt::CastField<rt::FArrayProperty>(property)) {
        rt::FScriptArrayHelper elements(array, value);
        std::string text;
        for (rt::int32 i = 0; i < elements.Num(); i++) {
            std::optional<std::string> element = exportExternalText(array->Inner, elements.GetRawPtr(i), true);
            if (!element)
                return std::nullopt;
            text += (i == 0 ? "(" : ",") + *element;
        }
        return elements.Num() == 0 ? text : text + ")";
    }
    if (const auto* structProperty = rt::CastField<rt::FStructProperty>(property)) {
        const rt::UScriptStruct* type = structProperty->Struct;
        if (type->SuperStruct != nullptr || (type->StructFlags & rt::STRUCT_ExportTextItemNative) != 0 || type->GetClass()->GetName().ToUtf8() != "ScriptStruct")
            return std::nullopt;
        std::string text;
        for (const rt::FProperty* member = type->PropertyLink; member != nullptr; member = member->PropertyLinkNext) {
            if (member->GetSize() <= 0 || member->HasAnyPropertyFlags(rt::CPF_Deprecated | rt::CPF_Transient | rt::CPF_NonPIEDuplicateTransient))
                continue;
            if (member->ArrayDim != 1)
                return std::nullopt;
            std::optional<std::string> item = exportExternalText(member, member->ContainerPtrToValuePtr<void>(value), true);
            if (!item)
                return std::nullopt;
            text += (text.empty() ? "(" : ",") + member->GetName().ToUtf8() + "=" + *item;
        }
        return text.empty() ? "()" : text + ")";
    }
    return std::nullopt;
}

void checkDataTableColumns(const std::string& path, const Json& value) {
    rt::UDataTable* table = loadObject<rt::UDataTable>(path);
    if (table == nullptr || !CHECK(table->GetRowStruct() != nullptr))
        return;
    rt::TArray<rt::FName> rows = table->GetRowNames();
    for (const auto& [column, cells] : value.getObject()) {
        const rt::FProperty* property = table->GetRowStruct()->FindPropertyByName(rt::FName(column));
        if (property == nullptr) {
            fail(describe(path, column), "no such column");
            continue;
        }
        if (!CHECK(rows.Num() == static_cast<rt::int32>(cells.getArray().size())))
            continue;
        for (rt::int32 i = 0; i < rows.Num(); i++) {
            std::string what = describe(path, rows[i].toString() + "." + column);
            std::optional<std::string> actual = exportExternalText(property, property->ContainerPtrToValuePtr<void>(table->FindRowUnchecked(rows[i])), false);
            if (!actual)
                fail(what, "the test cannot export this property type as text");
            else
                checkText(*actual, cells.getArray()[i].getString(), what);
        }
    }
}

}

void addDataChecks(Checks& checks) {
    checks.values.emplace("curve.color", checkColorCurve);
    checks.values.emplace("curve.float", checkFloatCurve);
    checks.values.emplace("curve.vector", checkVectorCurve);
    checks.values.emplace("curvetable", checkCurveTable);
    checks.values.emplace("datatable.column", checkDataTableColumns);
    checks.values.emplace("datatable.rows", checkDataTableRows);
}

}
