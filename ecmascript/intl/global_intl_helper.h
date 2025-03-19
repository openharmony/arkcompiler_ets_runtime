/**
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_BASE_GLOBAL_INTL_HELPER_H
#define ECMASCRIPT_BASE_GLOBAL_INTL_HELPER_H

#include <iostream>
#include <cstdint>
#include <cmath>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <math.h>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/js_handle.h"
#include "base/global/i18n/frameworks/intl/include/collator.h"
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif
#include "base/global/i18n/frameworks/intl/include/number_format.h"
#include "base/global/i18n/frameworks/intl/include/date_time_format.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace panda::ecmascript::intl {
using GlobalCollator = OHOS::Global::I18n::Collator;
using GlobalNumberFormat = OHOS::Global::I18n::NumberFormat;
using GlobalDateFormatter = OHOS::Global::I18n::DateTimeFormat;
using GlobalCompareResult = OHOS::Global::I18n::CompareResult;

enum class GlobalFormatterType {
    DateFormatter,
    SimpleDateFormatDate,
    SimpleDateFormatTime,
    NumberFormatter,
    Collator
};

class GlobalIntlHelper {
public:
    GlobalIntlHelper(JSThread *thread, const GlobalFormatterType matterType);
    ~GlobalIntlHelper()
    {
        if (dateUnitArr != nullptr) {
            delete[] dateUnitArr;
        }
        globalObject = nullptr;
    }

    template<typename T>
    std::unique_ptr<T> GetGlobalObject(JSThread *thread, const JSHandle<JSTaggedValue> &locales,
        const JSHandle<JSTaggedValue> &options, GlobalFormatterType types)
    {
        std::vector<std::string> cacheEntryVector;
        cacheEntryVector = LocalesToVector(thread, locales);
        std::map<std::string, std::string> inputOptions;
        if (!options->IsUndefined()) {
            inputOptions = OptionsToMap(thread, options, types);
        }
        auto result = std::make_unique<T>(cacheEntryVector, inputOptions);
        return result;
    }

    template<typename T>
    T *GetGlobalObject(JSThread *thread, const JSHandle<JSTaggedValue> &locales,
        const JSHandle<JSTaggedValue> &options, GlobalFormatterType types, const bool cache)
    {
        IcuFormatterType icuType;
        switch (types) {
            case GlobalFormatterType::Collator:
                icuType = IcuFormatterType::COLLATOR;
                break;
            case GlobalFormatterType::SimpleDateFormatDate:
                icuType = IcuFormatterType::SIMPLE_DATE_FORMAT_DATE;
                break;
            case GlobalFormatterType::SimpleDateFormatTime:
                icuType = IcuFormatterType::SIMPLE_DATE_FORMAT_TIME;
                break;
            case GlobalFormatterType::DateFormatter:
                icuType = IcuFormatterType::SIMPLE_DATE_FORMAT_DEFAULT;
                break;
            case GlobalFormatterType::NumberFormatter:
                icuType = IcuFormatterType::NUMBER_FORMATTER;
                break;
        }
        EcmaVM *ecmaVm = thread->GetEcmaVM();
        std::string cacheEntry = locales->IsUndefined() ? "" :
            EcmaStringAccessor(locales.GetTaggedValue()).ToStdString();
        if (cache) {
            void *cachedCollator = ecmaVm->GetIntlCache().GetIcuFormatterFromCache(icuType, cacheEntry);
            if (cachedCollator != nullptr) {
                return reinterpret_cast<T *>(cachedCollator);
            }
        }
        std::unique_ptr<T> tObject = GetGlobalObject<T>(thread, locales, options, types);
        if (cache) {
            T *cacheObject = tObject.release();
            switch (icuType) {
                case IcuFormatterType::COLLATOR:
                    ecmaVm->SetIcuFormatterToCache(icuType, cacheEntry, cacheObject, FreeCollatorFormat);
                    break;
                case IcuFormatterType::SIMPLE_DATE_FORMAT_DATE:
                case IcuFormatterType::SIMPLE_DATE_FORMAT_TIME:
                case IcuFormatterType::SIMPLE_DATE_FORMAT_DEFAULT:
                    ecmaVm->SetIcuFormatterToCache(icuType, cacheEntry, cacheObject, FreeDateTimeFormat);
                    break;
                case IcuFormatterType::NUMBER_FORMATTER:
                    ecmaVm->SetIcuFormatterToCache(icuType, cacheEntry, cacheObject, FreeNumberFormat);
                    break;
            }
            return cacheObject;
        }
        globalObject = tObject.release();
        return reinterpret_cast<T *>(globalObject);
    }
    static void FreeCollatorFormat([[maybe_unused]] void *env, void *pointer, void *data)
    {
        if (pointer == nullptr) {
            return;
        }
        auto globalCollator = reinterpret_cast<GlobalCollator*>(pointer);
        globalCollator->~Collator();
        if (data != nullptr) {
            reinterpret_cast<EcmaVM *>(data)->GetNativeAreaAllocator()->FreeBuffer(pointer);
        }
    }
    static void FreeNumberFormat([[maybe_unused]] void *env, void *pointer, void *data)
    {
        if (pointer == nullptr) {
            return;
        }
        auto globalNumberFormat = reinterpret_cast<GlobalNumberFormat*>(pointer);
        globalNumberFormat->~NumberFormat();
        if (data != nullptr) {
            reinterpret_cast<EcmaVM *>(data)->GetNativeAreaAllocator()->FreeBuffer(pointer);
        }
    }
    static void FreeDateTimeFormat([[maybe_unused]] void *env, void *pointer, void *data)
    {
        if (pointer == nullptr) {
            return;
        }
        auto globalDateFormatter = reinterpret_cast<GlobalDateFormatter*>(pointer);
        globalDateFormatter->~DateTimeFormat();
        if (data != nullptr) {
            reinterpret_cast<EcmaVM *>(data)->GetNativeAreaAllocator()->FreeBuffer(pointer);
        }
    }
    std::vector<std::string> LocalesToVector(JSThread *thread, const JSHandle<JSTaggedValue> &locales);
    std::map<std::string, std::string> OptionsToMap(JSThread *thread,
        const JSHandle<JSTaggedValue> &options, GlobalFormatterType types);
    uint64_t *ConvertDateToUnit(uint64_t timestamp);
    static int64_t DoubleToInt64(double value);
private:
    void *globalObject = nullptr;
    uint64_t *dateUnitArr = nullptr;
    void InitCollatorData(const GlobalEnvConstants *globalConst);
    void InitNumberData(const GlobalEnvConstants *globalConst);
    void InitDateData(const GlobalEnvConstants *globalConst);
    std::map<std::string, std::string> OptionsWithDataFormatter(std::map<std::string,
        std::string> &options, GlobalFormatterType &types);
    std::map<GlobalFormatterType, std::map<std::string, JSHandle<JSTaggedValue>>> optionMaps;
    std::string EcmaConvertToStr(const JSHandle<EcmaString> &string);
    std::vector<std::string> TaggedArrayToVector(JSThread *thread, JSHandle<TaggedArray> &taggedarray);
};
}  // panda::ecmascript::base
#endif  // ECMASCRIPT_BASE_GLOBAL_INTL_HELPER_H