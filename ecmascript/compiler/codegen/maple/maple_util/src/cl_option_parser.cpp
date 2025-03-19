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

#include "cl_option.h"

using namespace maplecl;

/* ################################################################
 * Utility Fuctions
 * ################################################################ */

/* Anonymous namespace to restrict visibility of utility functions */
namespace {

#ifdef ARK_LITECG_DEBUG
bool IsPrefixDetected(std::string_view opt)
{
    if (opt.substr(0, 2) == "--") { // whether opt start with 2 char "--"
        return true;
    }

    /* It should be "--" or "-" */
    return (opt[0] == '-');
}

/* NOTE: Returning size_t parameter is used to show how many command line arguments
 * handled with this key. argsIndex must be incremented with this parameter outside of ExtractValue. */
std::pair<RetCode, size_t> ExtractValue(size_t argsIndex, const std::deque<std::string_view> &args,
                                        const OptionInterface &opt, KeyArg &keyArg)
{
    /* The option like "--key= " does not contain a value after equal symbol */
    if (keyArg.isEqualOpt == true && keyArg.val.empty()) {
        return {RetCode::valueEmpty, 0};
    }

    bool canValBeSet = (keyArg.isEqualOpt || opt.IsJoinedValPermitted());
    /* Value is parsed outside OptionType::Parse because it's the option
     * like "--key=val" or joined option like -DValue */
    if (canValBeSet && !keyArg.val.empty()) {
        if (opt.ExpectedVal() == ValueExpectedType::kValueDisallowed) {
            return {RetCode::unnecessaryValue, 0};
        }
        return {RetCode::noError, 0};
    } else {    // Need to parse second command line argument to check options value
        /* Optional value can be set only with "=" like this --key=value  */
        if (opt.ExpectedVal() == ValueExpectedType::kValueDisallowed ||
            opt.ExpectedVal() == ValueExpectedType::kValueOptional) {
            return {RetCode::noError, 0};
        }

        /* localArgsIndex is used to be sure that nobody breaks the logic by
         * changing argsIndex to reference. original argsIndex must be not changed here. */
        size_t localArgsIndex = argsIndex + 1;
        /* Second command line argument does not exist */
        if (localArgsIndex >= args.size() || args[localArgsIndex].empty()) {
            RetCode ret =
                (opt.ExpectedVal() == ValueExpectedType::kValueRequired) ? RetCode::valueEmpty : RetCode::noError;
            return {ret, 0};
        }

        keyArg.val = args[localArgsIndex];
        return {RetCode::noError, 1};
    }
}
#endif
} /* Anonymous namespace  */

/* ################################################################
 * Option Value Parsers Implementation
 * ################################################################ */

template <>
RetCode Option<bool>::ParseBool(size_t &argsIndex, const std::deque<std::string_view> &args)
{
#ifdef ARK_LITECG_DEBUG
    /* DisabledName should set it to false, like --fno-omit-framepointer vs --fomit-framepointer */
    auto disabledNames = GetDisabledName();
    auto it = std::find(disabledNames.begin(), disabledNames.end(), args[argsIndex]);
    SetValue(it == disabledNames.end());

    ++argsIndex;
#endif
    return RetCode::noError;
}

/* NOTE: argsIndex must be incremented only if option is handled successfully */
template <>
RetCode Option<std::string>::ParseString(size_t &argsIndex, const std::deque<std::string_view> &args, KeyArg &keyArg)
{
#ifdef ARK_LITECG_DEBUG
    RetCode err = RetCode::noError;
    size_t indexIncCnt = 0;
    std::tie(err, indexIncCnt) = ExtractValue(argsIndex, args, *this, keyArg);
    if (err != RetCode::noError) {
        return err;
    }

    if (keyArg.val.empty()) {
        if (ExpectedVal() == ValueExpectedType::kValueRequired) {
            return RetCode::valueEmpty;
        }
        ++argsIndex;
        return RetCode::noError;
    }

    /* Value is not empty but ValueDisallowed is set */
    if (ExpectedVal() == ValueExpectedType::kValueDisallowed) {
        return RetCode::unnecessaryValue;
    }

    if (!keyArg.isEqualOpt && IsPrefixDetected(keyArg.val)) {
        if (ExpectedVal() == ValueExpectedType::kValueRequired) {
            return RetCode::valueEmpty;
        }
        ++argsIndex;
        return RetCode::noError;
    }

    SetValue(std::string(keyArg.val));
    if (keyArg.isEqualOpt && !keyArg.isJoinedOpt) {
        /* isJoinedOpt is used to prevent -DMACRO=VALUE.
         * -DMACRO=VALUE uses "=" sign but it's not the separator between Option and Value,
         * In this case it's a part of Value.
         */
        SetEqualType(EqualType::kWithEqual);
    }

    argsIndex += 1 + indexIncCnt;  // 1 for key, indexIncCnt for Key Value from ExtractValue
#endif
    return RetCode::noError;
}

/* NOTE: argsIndex must be incremented only if option is handled successfully */
template <typename T>
RetCode Option<T>::ParseDigit(size_t &argsIndex, const std::deque<std::string_view> &args, KeyArg &keyArg)
{
#ifdef ARK_LITECG_DEBUG
    static_assert(digitalCheck<T>, "Expected (u)intXX types");

    RetCode err = RetCode::noError;
    size_t indexIncCnt = 0;
    std::tie(err, indexIncCnt) = ExtractValue(argsIndex, args, *this, keyArg);
    if (err != RetCode::noError) {
        return err;
    }

    if (keyArg.val.empty()) {
        if (ExpectedVal() == ValueExpectedType::kValueRequired) {
            return RetCode::valueEmpty;
        }
        ++argsIndex;
        return RetCode::noError;
    }

    /* Value is not empty but ValueDisallowed is set */
    if (ExpectedVal() == ValueExpectedType::kValueDisallowed) {
        return RetCode::unnecessaryValue;
    }

    T resDig = 0;
    uint64_t udig = 0;
    int64_t dig = 0;
    char *endStrPtr;
    errno = 0;
    if constexpr (std::is_same_v<uint64_t, T> || std::is_same_v<uint32_t, T> || std::is_same_v<uint16_t, T> ||
                  std::is_same_v<uint8_t, T>) {
        if (keyArg.val.data()[0] == '-') {
            return RetCode::incorrectValue;
        }

        udig = std::strtoull(keyArg.val.data(), &endStrPtr, 0);
        resDig = static_cast<T>(udig);
    } else {
        dig = std::strtoll(keyArg.val.data(), &endStrPtr, 0);
        resDig = static_cast<T>(dig);
    }

    bool u32Type = std::is_same<T, uint32_t>::value;
    bool i32Type = std::is_same<T, int32_t>::value;
    bool u16Type = std::is_same<T, uint16_t>::value;
    bool u8Type = std::is_same<T, uint8_t>::value;
    bool i16Type = std::is_same<T, int16_t>::value;
    bool i8Type = std::is_same<T, int8_t>::value;

    if (*endStrPtr != '\0') {
        return RetCode::incorrectValue;
    }

    if (errno != 0) {
        return RetCode::outOfRange;
    }

    if (u32Type && udig > UINT32_MAX) {
        return RetCode::outOfRange;
    }

    if (i32Type && (dig > INT32_MAX || dig < INT32_MIN)) {
        return RetCode::outOfRange;
    }

    if (u16Type && udig > UINT16_MAX) {
        return RetCode::outOfRange;
    }

    if (i16Type && (dig > INT16_MAX || dig < INT16_MIN)) {
        return RetCode::outOfRange;
    }

    if (u8Type && udig > UINT8_MAX) {
        return RetCode::outOfRange;
    }

    if (i8Type && (dig > INT8_MAX || dig < INT8_MIN)) {
        return RetCode::outOfRange;
    }

    SetValue(resDig);
    if (keyArg.isEqualOpt && !keyArg.isJoinedOpt) {
        /* isJoinedOpt is used to prevent -DMACRO=VALUE.
         * -DMACRO=VALUE uses "=" sign but it's not the separator between Option and Value,
         * In this case it's a part of Value.
         */
        SetEqualType(EqualType::kWithEqual);
    }

    argsIndex += 1 + indexIncCnt;  // 1 for key, indexIncCnt for Key Value from ExtractValue
#endif
    return RetCode::noError;
}

/* Needed to describe OptionType<>::Parse template in this .cpp file */
template class maplecl::Option<uint8_t>;
template class maplecl::Option<uint16_t>;
template class maplecl::Option<uint32_t>;
template class maplecl::Option<uint64_t>;
template class maplecl::Option<int8_t>;
template class maplecl::Option<int16_t>;
template class maplecl::Option<int32_t>;
template class maplecl::Option<int64_t>;
