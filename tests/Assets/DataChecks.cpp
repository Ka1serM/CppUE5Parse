#include <cstdlib>
#include <set>

#include "AssetSuite.h"
#include "Engine/Source/Runtime/Engine/Curves/CurveFloat.h"
#include "Engine/Source/Runtime/Engine/Curves/CurveLinearColor.h"
#include "Engine/Source/Runtime/Engine/Curves/CurveVector.h"
#include "Engine/Source/Runtime/Engine/Engine/CurveTable.h"
#include "Engine/Source/Runtime/Engine/Engine/DataTable.h"

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
        for (rt::int32 i = 0; i < rows.Num(); i++)
            checkPropertyValue(property, property->ContainerPtrToValuePtr<void>(table->FindRowUnchecked(rows[i])), cells.getArray()[i],
                               describe(path, rows[i].toString() + "." + column));
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
