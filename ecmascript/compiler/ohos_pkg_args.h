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

#ifndef ECMASCRIPT_COMPILER_OHOS_PKG_ARGS_H
#define ECMASCRIPT_COMPILER_OHOS_PKG_ARGS_H

#include <limits>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/base/json_parser.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/platform/file.h"

namespace panda::ecmascript::kungfu {
class OhosPkgArgs {
public:
    constexpr static const char *const KEY_BUNDLE_NAME = "bundleName";
    constexpr static const char *const KEY_MODULE_NAME = "moduleName";
    constexpr static const char *const KEY_PKG_PATH = "pkgPath";
    constexpr static const char *const KEY_FILE_NAME = "abcName";
    constexpr static const char *const KEY_ABC_OFFSET = "abcOffset";
    constexpr static const char *const KEY_ABC_SIZE = "abcSize";

    OhosPkgArgs() = default;

    static bool ParseListFromJson(EcmaVM *vm, const std::string &jsonInfo, std::list<OhosPkgArgs> &infoList)
    {
        LocalScope scope(vm);
        ObjectFactory *factory = vm->GetFactory();
        ecmascript::base::Utf8JsonParser parser(vm->GetJSThread());

        JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII(jsonInfo.c_str()));
        JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(vm->GetAssociatedJSThread(), handleMsg));  // JSON Object
        JSHandle<JSTaggedValue> result = parser.Parse(*handleStr);
        JSTaggedValue resultValue(static_cast<JSTaggedType>(result->GetRawData()));
        if (!resultValue.IsArray(vm->GetJSThread())) {
            LOG_COMPILER(ERROR) << "Pkg list info parse failed. result is not an array. jsonData: " << jsonInfo.c_str();
            return false;
        }
        JSHandle<JSArray> valueHandle(vm->GetJSThread(), resultValue);
        JSHandle<TaggedArray> elements(vm->GetJSThread(), valueHandle->GetElements());
        for (uint32_t i = 0; i < elements->GetLength(); i++) {
            OhosPkgArgs pkgInfo;
            JSHandle<JSTaggedValue> entry(vm->GetJSThread(), elements->Get(i));
            if (entry->IsHole()) {
                continue;
            }
            JSTaggedValue entryValue(static_cast<JSTaggedType>(entry->GetRawData()));
            JSHandle<JSObject> entryHandle(vm->GetJSThread(), entryValue);
            if (!pkgInfo.ParseFromJsObject(vm, entryHandle)) {
                LOG_COMPILER(ERROR) << "Pkg list entry info parse failed. jsonData: " << jsonInfo.c_str();
                return false;
            }
            infoList.emplace_back(pkgInfo);
        }
        return true;
    }

    bool ParseFromJson(EcmaVM *vm, const std::string &jsonInfo)
    {
        LocalScope scope(vm);
        ObjectFactory *factory = vm->GetFactory();
        ecmascript::base::Utf8JsonParser parser(vm->GetJSThread());

        JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII(jsonInfo.c_str()));
        JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(vm->GetAssociatedJSThread(), handleMsg));  // JSON Object
        JSHandle<JSTaggedValue> result = parser.Parse(*handleStr);
        JSTaggedValue resultValue(static_cast<JSTaggedType>(result->GetRawData()));
        if (!resultValue.IsECMAObject()) {
            LOG_COMPILER(ERROR) << "Pkg info parse failed. result is not an object. jsonData: " << jsonInfo.c_str();
            return false;
        }
        JSHandle<JSObject> valueHandle(vm->GetJSThread(), resultValue);
        return ParseFromJsObject(vm, valueHandle);
    }

    bool ParseFromJsObject(EcmaVM *vm, JSHandle<JSObject> &valueHandle)
    {
        LocalScope scope(vm);
        JSHandle<TaggedArray> nameList(JSObject::EnumerableOwnNames(vm->GetJSThread(), valueHandle));
        for (uint32_t i = 0; i < nameList->GetLength(); i++) {
            JSHandle<JSTaggedValue> key(vm->GetJSThread(), nameList->Get(i));
            JSHandle<JSTaggedValue> value = JSObject::GetProperty(vm->GetJSThread(), valueHandle, key).GetValue();
            if (!key->IsString() || !value->IsString()) {
                LOG_COMPILER(ERROR) << "Pkg info parse from js object failed. key and value must be string type.";
                return false;
            }
            UpdateProperty(ConvertToString(*JSTaggedValue::ToString(vm->GetJSThread(), key)).c_str(),
                           ConvertToString(*JSTaggedValue::ToString(vm->GetJSThread(), value)).c_str());
        }
        return Valid();
    }

    void UpdateProperty(const char *key, const char *value)
    {
        if (strcmp(key, KEY_BUNDLE_NAME) == 0) {
            bundleName_ = value;
        } else if (strcmp(key, KEY_MODULE_NAME) == 0) {
            moduleName_ = value;
        } else if (strcmp(key, KEY_PKG_PATH) == 0) {
            pkgPath_ = value;
        } else if (strcmp(key, KEY_FILE_NAME) == 0) {
            abcName_ = value;
        } else if (strcmp(key, KEY_ABC_OFFSET) == 0) {
            char *str = nullptr;
            abcOffset_ = strtol(value, &str, 0);
        } else if (strcmp(key, KEY_ABC_SIZE) == 0) {
            char *str = nullptr;
            abcSize_ = static_cast<uint32_t>(strtol(value, &str, 0));
        } else {
            LOG_COMPILER(ERROR) << "Unknown keyword when parse pkg info. key: " << key << ", value: " << value;
        }
    }

    bool Valid() const
    {
        if (!base::StringHelper::EndsWith(abcName_, ".abc")) {
            LOG_COMPILER(ERROR) << "The last argument must be abc file, but now is: " << abcName_ << std::endl;
            return false;
        }
        return !bundleName_.empty() && !moduleName_.empty() && !pkgPath_.empty() && (abcOffset_ != INVALID_VALUE) &&
               (abcSize_ != INVALID_VALUE);
    }

    void Dump() const
    {
        LOG_ECMA(INFO) << "PkgInfo: " << KEY_BUNDLE_NAME << ": " << bundleName_ << ", " << KEY_MODULE_NAME << ": "
                       << moduleName_ << ", " << KEY_PKG_PATH << ": " << pkgPath_ << ", " << KEY_ABC_OFFSET << ": "
                       << std::hex << abcOffset_ << ", " << KEY_ABC_SIZE << ": " << abcSize_;
    }

    const std::string &GetBundleName() const
    {
        return bundleName_;
    }

    const std::string &GetModuleName() const
    {
        return moduleName_;
    }

    const std::string &GetPath() const
    {
        return pkgPath_;
    }

    std::string GetFullName() const
    {
        return pkgPath_ + GetPathSeparator() + moduleName_ + GetPathSeparator() + abcName_;
    }

    uint32_t GetOffset() const
    {
        return abcOffset_;
    }

    uint32_t GetSize() const
    {
        return abcSize_;
    }

private:
    static constexpr uint32_t INVALID_VALUE = std::numeric_limits<uint32_t>::max();
    std::string bundleName_;
    std::string moduleName_;
    std::string pkgPath_;
    std::string abcName_;
    uint32_t abcOffset_ {INVALID_VALUE};
    uint32_t abcSize_ {INVALID_VALUE};
};
}  // namespace panda::ecmascript::kungfu
#endif