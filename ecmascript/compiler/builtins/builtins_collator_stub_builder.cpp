/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecmascript/compiler/builtins/builtins_collator_stub_builder.h"

#include "ecmascript/compiler/new_object_stub_builder.h"

namespace panda::ecmascript::kungfu {
void BuiltinsCollatorStubBuilder::ResolvedOptions(GateRef glue, GateRef thisValue, [[maybe_unused]] GateRef numArgs,
                                                  Variable* res, Label* exit,
                                                  Label* slowPath)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);

    Label isHeapObject(env);
    Label isJsCollator(env);

    BRANCH_LIKELY(TaggedIsHeapObject(thisValue), &isHeapObject, slowPath);
    Bind(&isHeapObject);
    BRANCH_LIKELY(IsJSCollator(glue, thisValue), &isJsCollator, slowPath);

    Bind(&isJsCollator);
    GateRef globalEnv = GetCurrentGlobalEnv();
    GateRef funCtor = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::OBJECT_FUNCTION_INDEX);

    NewObjectStubBuilder newObjectStubBuilder(this);
    GateRef initialOptions = newObjectStubBuilder.NewJSObjectByConstructor(glue, funCtor, funCtor);
    DEFVARIABLE(options, VariableType::JS_ANY(), initialOptions);

    // [[Locale]]
    GateRef localeKey = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::LOCALE_STRING_INDEX);
    GateRef locale = Load(VariableType::JS_ANY(), glue, thisValue, IntPtr(JSCollator::LOCALE_OFFSET));
    CreateDataPropertyOrThrow(glue, *options, localeKey, locale);
    ReturnExceptionIfAbruptCompletion(glue);

    // [[Usage]]
    GateRef usageKey = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::USAGE_INDEX);
    GateRef usage = GetBitField<JSCollator::UsageBits>(thisValue);
    usage = UsageOptionsToEcmaString(glue, usage);
    CreateDataProperty(glue, *options, usageKey, usage);

    // [[Sensitivity]]
    GateRef sensitivityKey = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::SENSITIVITY_INDEX);
    GateRef sensitivity = GetBitField<JSCollator::SensitivityBits>(thisValue);
    sensitivity = SensitivityOptionsToEcmaString(glue, sensitivity);
    CreateDataProperty(glue, *options, sensitivityKey, sensitivity);

    // [[IgnorePunctuation]]
    GateRef ignorePunctuationKey = GetGlobalConstantValue(VariableType::JS_ANY(), glue,
                                                          ConstantIndex::IGNORE_PUNCTUATION_INDEX);
    GateRef ignorePunctuation = GetBitField<JSCollator::IgnorePunctuationBits>(thisValue);
    ignorePunctuation = BooleanToTaggedBooleanPtr(TruncInt32ToInt1(ignorePunctuation));
    CreateDataProperty(glue, *options, ignorePunctuationKey, ignorePunctuation);

    // [[Collation]]
    Label undefined(env);
    Label notUndefined(env);
    GateRef collation = Load(VariableType::JS_ANY(), glue, thisValue, IntPtr(JSCollator::COLLATION_OFFSET));
    GateRef collationKey = GetGlobalConstantValue(VariableType::JS_ANY(), glue,
                                                  ConstantIndex::COLLATION_STRING_CLASS_INDEX);
    DEFVARIABLE(collationVar, VariableType::JS_ANY(), collation);
    BRANCH_NO_WEIGHT(TaggedIsUndefined(*collationVar), &undefined, &notUndefined);
    Bind(&undefined);
    {
        collationVar = CallRuntime(glue, RTSTUB_ID(GetCollationValueFromIcuCollator), {thisValue});
        Jump(&notUndefined);
    }
    Bind(&notUndefined);
    CreateDataProperty(glue, *options, collationKey, *collationVar);

    // [[Numeric]]
    GateRef numericKey = GetGlobalConstantValue(VariableType::JS_ANY(), glue,
                                                ConstantIndex::NUMERIC_STRING_CLASS_INDEX);
    GateRef numeric = GetBitField<JSCollator::NumericBits>(thisValue);
    numeric = BooleanToTaggedBooleanPtr(TruncInt32ToInt1(numeric));
    CreateDataProperty(glue, *options, numericKey, numeric);

    // [[CaseFirst]]
    GateRef caseFirstKey = GetGlobalConstantValue(VariableType::JS_ANY(), glue,
                                                  ConstantIndex::CASE_FIRST_STRING_CLASS_INDEX);
    GateRef caseFirst = GetBitField<JSCollator::CaseFirstBits>(thisValue);
    caseFirst = CaseFirstOptionsToEcmaString(glue, caseFirst);
    CreateDataProperty(glue, *options, caseFirstKey, caseFirst);
    res->WriteVariable(*options);
    Jump(exit);
    env->SubCfgExit();
}

template <typename BitType>
GateRef BuiltinsCollatorStubBuilder::GetBitField(GateRef collator)
{
    GateRef bitFieldOffset = IntPtr(JSCollator::BIT_FIELD_OFFSET);
    GateRef bitfield = LoadPrimitive(VariableType::INT32(), collator, bitFieldOffset);
    GateRef bits = Int32And(Int32LSR(bitfield, Int32(BitType::START_BIT)),
                            Int32((1LU << BitType::SIZE) - 1));
    return bits;
}

GateRef BuiltinsCollatorStubBuilder::UsageOptionsToEcmaString(GateRef glue, GateRef usage)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    Label sort(env);
    Label next(env);
    Label search(env);
    Label fatal(env);
    BRANCH_NO_WEIGHT(Int32Equal(usage, Int32(static_cast<int32_t>(UsageOption::SORT))), &sort, &next)
    Bind(&sort);
    {
        result = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::SORT_INDEX);
        Jump(&exit);
    }

    Bind(&next);
    BRANCH_NO_WEIGHT(Int32Equal(usage, Int32(static_cast<int32_t>(UsageOption::SEARCH))), &search, &fatal)
    Bind(&search);
    {
        result = GetGlobalConstantValue(VariableType::JS_ANY(), glue, ConstantIndex::SEARCH_INDEX);
        Jump(&exit);
    }
    Bind(&fatal);
    {
        FatalPrint(glue, {Int32(GET_MESSAGE_STRING_ID(ThisBranchIsUnreachable))});
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsCollatorStubBuilder::SensitivityOptionsToEcmaString(GateRef glue, GateRef sensitivity)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label undefined(env);
    Label fatal(env);

    Label labels[SENSITIVITY_OPTION_CASES_NUM] = {Label(env), Label(env), Label(env), Label(env), undefined};
    Switch(ZExtInt32ToInt64(sensitivity), &fatal, SensitivityOptionCases, labels, SENSITIVITY_OPTION_CASES_NUM);

    for (size_t i = 0; i < SENSITIVITY_OPTION_CASES_NUM - 1; i++) {
        Bind(&labels[i]);
        result = GetGlobalConstantValue(VariableType::JS_ANY(), glue, SensitivityOptionIndexes[i]);
        Jump(&exit);
    }

    Bind(&undefined);
    {
        Jump(&exit);
    }
    Bind(&fatal);
    {
        FatalPrint(glue, {Int32(GET_MESSAGE_STRING_ID(ThisBranchIsUnreachable))});
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsCollatorStubBuilder::CaseFirstOptionsToEcmaString(GateRef glue, GateRef sensitivity)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label fatal(env);

    Label labels[CASE_FIRST_OPTION_NUM] = {Label(env), Label(env), Label(env), Label(env)};
    Switch(ZExtInt32ToInt64(sensitivity), &fatal, CaseFirstOptionCases, labels, CASE_FIRST_OPTION_NUM);

    for (size_t i = 0; i < CASE_FIRST_OPTION_NUM; i++) {
        Bind(&labels[i]);
        result = GetGlobalConstantValue(VariableType::JS_ANY(), glue, CaseFirstOptionIndexes[i]);
        Jump(&exit);
    }

    Bind(&fatal);
    {
        FatalPrint(glue, {Int32(GET_MESSAGE_STRING_ID(ThisBranchIsUnreachable))});
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

int64_t BuiltinsCollatorStubBuilder::SensitivityOptionCases[SENSITIVITY_OPTION_CASES_NUM] = {
    static_cast<int64_t>(SensitivityOption::BASE), static_cast<int64_t>(SensitivityOption::ACCENT),
    static_cast<int64_t>(SensitivityOption::CASE), static_cast<int64_t>(SensitivityOption::VARIANT),
    static_cast<int64_t>(SensitivityOption::UNDEFINED)
};

ConstantIndex BuiltinsCollatorStubBuilder::SensitivityOptionIndexes[SENSITIVITY_OPTION_CASES_NUM - 1] = {
    ConstantIndex::BASE_INDEX, ConstantIndex::ACCENT_INDEX,
    ConstantIndex::CASE_INDEX, ConstantIndex::VARIANT_INDEX
};

int64_t BuiltinsCollatorStubBuilder::CaseFirstOptionCases[CASE_FIRST_OPTION_NUM] = {
    static_cast<int64_t>(CaseFirstOption::UPPER), static_cast<int64_t>(CaseFirstOption::LOWER),
    static_cast<int64_t>(CaseFirstOption::FALSE_OPTION), static_cast<int64_t>(CaseFirstOption::UNDEFINED)
};

ConstantIndex BuiltinsCollatorStubBuilder::CaseFirstOptionIndexes[CASE_FIRST_OPTION_NUM] = {
    ConstantIndex::UPPER_INDEX, ConstantIndex::LOWER_INDEX,
    ConstantIndex::FALSE_STRING_INDEX, ConstantIndex::UPPER_INDEX
};
}
