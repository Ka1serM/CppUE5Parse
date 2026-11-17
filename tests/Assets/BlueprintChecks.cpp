#include <cstdio>
#include <vector>

#include "AssetSuite.h"

namespace test::assets {

namespace {

void checkBlueprintDefaults(const std::string& path, const Json& value) {
    rt::UClass* generated = loadObject<rt::UClass>(path);
    if (generated == nullptr || !CHECK(generated->GetDefaultObject() != nullptr))
        return;
    rt::UObject* defaults = generated->GetDefaultObject();
    for (const auto& [name, expected] : value.getObject()) {
        const rt::FProperty* property = generated->FindPropertyByName(rt::FName(name));
        if (property == nullptr)
            fail(describe(path, name), "no such property");
        else
            checkPropertyValue(property, property->ContainerPtrToValuePtr<void>(defaults), expected, describe(path, name));
    }
}

void checkBlueprintCalls(const std::string& path, const Json& value) {
    rt::UClass* generated = loadObject<rt::UClass>(path);
    if (generated == nullptr || !CHECK(generated->GetDefaultObject() != nullptr))
        return;
    rt::UObject* defaults = generated->GetDefaultObject();
    for (const auto& [call, expected] : value.getObject()) {
        std::string name = call.substr(0, call.find('('));
        double arguments[2] = {};
        std::sscanf(call.c_str() + name.size(), "(%lf,%lf)", &arguments[0], &arguments[1]);
        rt::UFunction* function = defaults->FindFunction(rt::FName(name));
        if (function == nullptr) {
            fail(describe(path, name), "no such function");
            continue;
        }
        std::vector<std::byte> parameters(function->ParmsSize);
        rt::FNumericProperty* inputs[] = {rt::CastField<rt::FNumericProperty>(function->FindPropertyByName(rt::FName("Value"))),
                                          rt::CastField<rt::FNumericProperty>(function->FindPropertyByName(rt::FName("Factor")))};
        auto* output = rt::CastField<rt::FNumericProperty>(function->FindPropertyByName(rt::FName("Scaled")));
        if (!CHECK(inputs[0] != nullptr && inputs[1] != nullptr && output != nullptr))
            continue;
        for (int i = 0; i < 2; i++)
            inputs[i]->SetFloatingPointPropertyValue(inputs[i]->ContainerPtrToValuePtr<void>(parameters.data()), arguments[i]);
        defaults->ProcessEvent(function, parameters.data());
        checkNear(output->GetFloatingPointPropertyValue(output->ContainerPtrToValuePtr<void>(parameters.data())), expected.getNumber(), valueTolerance,
                  describe(path, call));
    }
}

}

void addBlueprintChecks(Checks& checks) {
    checks.values.emplace("bp.call", checkBlueprintCalls);
    checks.values.emplace("bp.default", checkBlueprintDefaults);
}

}
