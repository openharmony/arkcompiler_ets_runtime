/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef MAPLE_UTIL_INCLUDE_OPTION_H
#define MAPLE_UTIL_INCLUDE_OPTION_H

#include "cl_parser.h"
#include "mpl_logging.h"

#include <any>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

namespace maplecl {

template <typename T>
constexpr inline bool digitalCheck = (std::is_same_v<std::uint8_t, T> || std::is_same_v<std::uint16_t, T> ||
                                      std::is_same_v<std::uint32_t, T> || std::is_same_v<std::uint64_t, T> ||
                                      std::is_same_v<std::int64_t, T> || std::is_same_v<std::int32_t, T> ||
                                      std::is_same_v<std::int16_t, T> || std::is_same_v<std::int8_t, T>);

/* is key VALUE needed ? */
enum class ValueExpectedType {
    kValueOptional,   /* "--key VALUE" and "--key" are allowed */
    kValueRequired,   /* only "--key VALUE" is allowed */
    kValueDisallowed, /* only "--key" is allowed */
};

/* is "joined option" (like -DMACRO) allowed ? */
enum class ValueJoinedType {
    kValueSeparated,
    kValueJoined, /* joined option (like -DMACRO) is allowed */
};

/* is option visible from Help ? */
enum class OptionVisibilityType {
    kVisibleOption, /* an Option will be visible in Help */
    kHidedOption    /* an Option will be NOT visible in Help */
};

/* This enum is used as the option attribute to detect "=" in the original option.
 * It can be useful to forward original option in an external tool.
 * As example: user sets maple -std=gnu11 ; In this case, maple will forward -std
 * option in clang, but clang can detect this option only with "=":
 * clang -std=gnu11 (NOT clang -std gnu11). So this attribute is used for correct forwarding.
 */
enum class EqualType { kWithEqual, kWithoutEqual };

/* These constexpr are needed to use short name in option description, like this:
 * maplecl::Option<int32_t> option({"--option"}, "Description", optionalValue);
 * instead of:
 * maplecl::Option<int32_t> option({"--option"}, "Description", ValueExpectedType::kValueOptional);
 */
constexpr ValueExpectedType optionalValue = ValueExpectedType::kValueOptional;
constexpr ValueExpectedType requiredValue = ValueExpectedType::kValueRequired;
constexpr ValueExpectedType disallowedValue = ValueExpectedType::kValueDisallowed;
constexpr ValueJoinedType joinedValue = ValueJoinedType::kValueJoined;
constexpr ValueJoinedType separatedValue = ValueJoinedType::kValueSeparated;
constexpr OptionVisibilityType visible = OptionVisibilityType::kVisibleOption;
constexpr OptionVisibilityType hide = OptionVisibilityType::kHidedOption;

/* Initializer is used to set default value for an option */
template <typename T>
struct Init {
    const T &defaultVal;
    explicit Init(const T &val) : defaultVal(val) {}
};

/* DisableWithData is used to set additional option name to disable this option.
 * Example: Base option: --opt; Additional option: --no-opt.
 * --no-opt will disable this option in this example.
 */
struct DisableWith {
    const std::string &disableWith;
    explicit DisableWith(const std::string &val) : disableWith(val) {}
};
struct DisableEvery {
    const std::vector<std::string> &disableEvery;
    explicit DisableEvery(const std::vector<std::string> &val) : disableEvery(val) {}
};

using OptionCategoryRefWrp = std::reference_wrapper<OptionCategory>;

class OptionWrp {
public:
    template <typename T>
    OptionWrp(T v) : val(v) /* implicit */
    {
    }

    template <typename T>
    operator T()
    {
        T *pval = std::any_cast<T>(&val);
        DEBUG_ASSERT(pval != nullptr, "pval should not be nullptr");
        return *pval;
    }

    std::any val;
};

/* Interface for templated Option class */
class OptionInterface {
public:
    virtual ~OptionInterface() = default;

    virtual RetCode Parse(size_t &argsIndex, const std::deque<std::string_view> &args, KeyArg &keyArg) = 0;
    virtual void Clear() = 0;
    virtual std::vector<std::string> GetRawValues() = 0;
    virtual OptionWrp GetCommonValue() const = 0;

    void FinalizeInitialization(const std::vector<std::string> &optnames, const std::string &descr,
                                const std::vector<OptionCategoryRefWrp> &optionCategories);

    bool IsEnabledByUser() const
    {
        return isEnabledByUser;
    }

    ValueExpectedType ExpectedVal() const
    {
        return valueExpected;
    }

    bool IsJoinedValPermitted() const
    {
        return (valueJoined == ValueJoinedType::kValueJoined);
    }

    bool IsVisibleOption() const
    {
        return (visibleOption == OptionVisibilityType::kVisibleOption);
    }

    const std::vector<std::string> &GetDisabledName() const
    {
        return disableWith;
    }

    virtual std::string GetName() const
    {
        DEBUG_ASSERT(names.size() > 0, "names.size should be non zero");
        return names[0];
    }

    const std::string &GetDescription() const
    {
        return optDescription;
    }

    EqualType GetEqualType() const
    {
        return equalSign;
    }

    void SetEqualType(EqualType equal)
    {
        equalSign = equal;
    }

    std::string rawKey;
    std::vector<OptionCategory *> optCategories;  // The option is registred in these categories

protected:
    std::vector<std::string> names;        // names of the option
    std::string optDescription;            // overview of option
    std::vector<std::string> disableWith;  // name to disable the option

    bool isEnabledByUser = false;  // it's set if the option is in command line

    ValueExpectedType valueExpected = ValueExpectedType::kValueRequired;        // whether the value is expected
    ValueJoinedType valueJoined = ValueJoinedType::kValueSeparated;             // Joined option like -DMACRO
    OptionVisibilityType visibleOption = OptionVisibilityType::kVisibleOption;  // Visible in Help

    EqualType equalSign = EqualType::kWithoutEqual;
};

/* Option class describes command line option */
template <typename T>
class Option : public OptionInterface {
public:
    /* variadic template is used to apply any number of options parameters in any order */
    template <typename... ArgsT>
    explicit Option(const std::vector<std::string> &optnames, const std::string &descr, const ArgsT &... args)
    {
        /* It's needed to avoid empty Apply() */
        if constexpr (sizeof...(ArgsT) > 0) {
            Apply(args...);
        }
        FinalizeInitialization(optnames, descr, {});
    }

    /* variadic template is used to apply any number of options parameters in any order */
    template <typename... ArgsT>
    explicit Option(const std::vector<std::string> &optnames, const std::string &descr,
                    const std::vector<OptionCategoryRefWrp> &optionCategories, const ArgsT &... args)
    {
        /* It's needed to avoid empty Apply() */
        if constexpr (sizeof...(ArgsT) > 0) {
            Apply(args...);
        }
        FinalizeInitialization(optnames, descr, optionCategories);
    }

    // options must not be copyable and assignment
    Option(const Option &) = delete;
    Option &operator=(const Option &) = delete;

    /* Conversation operator is needed to use the option like this:
     * strding test = option1; or int dig = option2 - here will be implicit conversation.
     */
    operator T() /*implicit*/
    {
        return GetValue();
    }

    RetCode Parse(size_t &argsIndex, const std::deque<std::string_view> &args, KeyArg &keyArg) override
    {
        RetCode err = RetCode::noError;
#ifdef ARK_LITECG_DEBUG
        auto &key = args[argsIndex];
        if constexpr (digitalCheck<T>) {
            err = ParseDigit(argsIndex, args, keyArg);
        } else if constexpr (std::is_same_v<std::string, T>) {
            err = ParseString(argsIndex, args, keyArg);
        } else if constexpr (std::is_same_v<bool, T>) {
            err = ParseBool(argsIndex, args);
        } else {
            /* Type dependent static_assert. Simple static_assert(false") does not work */
            static_assert(false && sizeof(T), "T not supported");
        }

        if (err == RetCode::noError) {
            isEnabledByUser = true;
            rawKey = key;
        }
#endif
        return err;
    }

    void Clear() override
    {
        if (defaultValue.isSet) {
            value = defaultValue.defaultValue;
        } else {
            if constexpr (digitalCheck<T>) {
                value = 0;
            } else if constexpr (std::is_same_v<std::string, T>) {
                value = "";
            } else if constexpr (std::is_same_v<bool, T>) {
                value = false;
            } else {
                /* Type dependent static_assert. Simple static_assert(false") does not work */
                static_assert(false && sizeof(T), "T not supported");
            }
        }

        for (auto &category : optCategories) {
            category->Remove(this);
        }

        isEnabledByUser = false;
    }

    std::vector<std::string> GetRawValues() override
    {
        std::vector<std::string> rawVals;
        FillVal(value, rawVals);
        return rawVals;
    }

    std::string GetName() const override
    {
        if constexpr (std::is_same_v<bool, T>) {
            DEBUG_ASSERT(names.size() > 0, "names.size should be non zero");
            return ((value == true) ? names[0] : this->GetDisabledName()[0]);
        } else {
            return OptionInterface::GetName();
        }
    }

    const T &GetValue() const
    {
        return value;
    }

    OptionWrp GetCommonValue() const override
    {
        return value;
    }

    virtual void SetValue(const T &val)
    {
        value = val;
    }

protected:
    RetCode ParseDigit(size_t &argsIndex, const std::deque<std::string_view> &args, KeyArg &keyArg);
    RetCode ParseString(size_t &argsIndex, const std::deque<std::string_view> &args, KeyArg &keyArg);
    RetCode ParseBool(size_t &argsIndex, const std::deque<std::string_view> &args);

    void FillVal(const T &val, std::vector<std::string> &vals)
    {
        if constexpr (digitalCheck<T>) {
            vals.emplace_back(std::to_string(val));
        } else if constexpr (std::is_same_v<std::string, T>) {
            vals.emplace_back(val);
        } else if constexpr (std::is_same_v<bool, T>) {
            vals.emplace_back("");
        } else {
            /* Type dependent static_assert. Simple static_assert(false") does not work */
            static_assert(false && sizeof(T), "T not supported");
        }
    }

    struct DefaultValue {
        T defaultValue;
        bool isSet = false;
    } defaultValue;

private:
    /* To apply input arguments in any order */
    template <typename ArgT, typename... ArgsT>
    void Apply(const ArgT &arg, const ArgsT &... args)
    {
        if constexpr (std::is_same_v<ValueExpectedType, ArgT>) {
            SetExpectingAttribute(arg);
        } else if constexpr (std::is_same_v<ValueJoinedType, ArgT>) {
            SetJoinAttribute(arg);
        } else if constexpr (std::is_same_v<OptionVisibilityType, ArgT>) {
            SetVisibilityAttribute(arg);
        } else if constexpr (std::is_same_v<DisableWith, ArgT>) {
            SetDisablingAttribute(arg);
        } else if constexpr (std::is_same_v<DisableEvery, ArgT>) {
            SetDisablingAttribute(arg);
        } else {
            SetDefaultAttribute(arg);
        }

        /* It's needed to avoid empty Apply() */
        if constexpr (sizeof...(ArgsT) > 0) {
            Apply(args...);
        }
    }

    template <typename InitT>
    void SetDefaultAttribute(const Init<InitT> &initializer)
    {
        SetValue(initializer.defaultVal);
        defaultValue.isSet = true;
        defaultValue.defaultValue = value;
    }

    void SetExpectingAttribute(ValueExpectedType value)
    {
        valueExpected = value;
    }

    void SetJoinAttribute(ValueJoinedType value)
    {
        valueJoined = value;
    }

    void SetVisibilityAttribute(OptionVisibilityType value)
    {
        visibleOption = value;
    }

    void SetDisablingAttribute(const DisableWith &value)
    {
        disableWith.push_back(value.disableWith);
    }

    void SetDisablingAttribute(const DisableEvery &value)
    {
        for (auto &val : value.disableEvery) {
            disableWith.push_back(val);
        }
    }

    T value;
};

template <typename T>
bool operator==(Option<T> &opt, const T &arg)
{
    return (opt.GetValue() == arg);
}

template <typename T>
bool operator==(const T &arg, Option<T> &opt)
{
    return opt == arg;
}

/* To handle the comparation of "const char *" and "Option<string>".
 * This comparation can not be handled with comparation template above! */
template <typename T>
bool operator==(Option<T> &opt, const char *arg)
{
    return (opt.GetValue() == arg);
}

template <typename T>
bool operator==(const char *arg, Option<T> &opt)
{
    return opt == arg;
}

template <typename T>
void CopyIfEnabled(T &dst, maplecl::Option<T> &src)
{
    if (src.IsEnabledByUser()) {
        dst = src;
    }
}

template <typename T>
void CopyIfEnabled(T &dst, const T &src, OptionInterface &opt)
{
    if (opt.IsEnabledByUser()) {
        dst = src;
    }
}

template <typename T>
class List : public Option<T> {
public:
    // options must not be copyable and assignment
    List(const List &) = delete;
    List &operator=(const List &) = delete;

    /* variadic template is used to apply any number of options parameters in any order */
    template <typename... ArgsT>
    explicit List(const std::vector<std::string> &optnames, const std::string &descr, const ArgsT &... args)
        : Option<T>(optnames, descr, args...) {};

    template <typename... ArgsT>
    explicit List(const std::vector<std::string> &optnames, const std::string &descr,
                  const std::vector<OptionCategoryRefWrp> &optionCategories, const ArgsT &... args)
        : Option<T>(optnames, descr, optionCategories, args...) {};

    void Clear() override
    {
        values.clear();
        if (this->defaultValue.isSet) {
            SetValue(this->defaultValue.defaultValue);
        }
        this->isEnabledByUser = false;

        for (auto &category : this->optCategories) {
            category->Remove(this);
        }
    }

    void SetValue(const T &val) override
    {
        values.push_back(val);
    }

    const T &GetValue() const
    {
        static_assert(false && sizeof(T), "GetValue must be not used for List");
        return T();
    }

    const std::vector<T> &GetValues() const
    {
        return values;
    }

    std::vector<std::string> GetRawValues() override
    {
        std::vector<std::string> rawVals;
        for (const auto &val : values) {
            this->FillVal(val, rawVals);
        }

        return rawVals;
    }

private:
    std::vector<T> values;
};

}  // namespace maplecl

#endif /* MAPLE_UTIL_INCLUDE_OPTION_H */
