/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BASE_FAST_JSON_STRINGIFY_H
#define ECMASCRIPT_BASE_FAST_JSON_STRINGIFY_H

#include "ecmascript/ecma_context.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/global_env.h"
#include "ecmascript/mem/c_containers.h"

namespace panda::ecmascript::base {
class FastJsonStringifier {
public:
    static constexpr int32_t INVALID_INDEX = -1;
    static constexpr int32_t JSON_CACHE_MASK = 62;
    static constexpr int32_t JSON_CACHE_SIZE = 64;
    static constexpr int32_t CACHE_MINIMUN_SIZIE = 5;
    FastJsonStringifier() = default;

    explicit FastJsonStringifier(JSThread *thread) : thread_(thread) {}

    ~FastJsonStringifier() = default;
    NO_COPY_SEMANTIC(FastJsonStringifier);
    NO_MOVE_SEMANTIC(FastJsonStringifier);

    JSHandle<JSTaggedValue> Stringify(const JSHandle<JSTaggedValue> &value);

private:
    JSTaggedValue SerializeJSONProperty(const JSHandle<JSTaggedValue> &value);
    JSTaggedValue GetSerializeValue(const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value);
    CString SerializeObjectKey(const JSHandle<JSTaggedValue> &key, bool hasContent);

    bool SerializeJSONObject(const JSHandle<JSTaggedValue> &value);

    bool SerializeJSArray(const JSHandle<JSTaggedValue> &value);
    bool SerializeJSProxy(const JSHandle<JSTaggedValue> &object);

    void SerializePrimitiveRef(const JSHandle<JSTaggedValue> &primitiveRef);

    bool PushValue(const JSHandle<JSTaggedValue> &value);

    void PopValue();

    bool AppendJsonString(bool hasContent, CVector<std::pair<CString, int>> &strCache, int index);
    bool FastAppendJsonString(bool hasContent, CString &key);
    bool TryCacheSerializeElements(const JSHandle<JSObject> &obj, bool hasContent,
                                   CVector<std::pair<CString, int>> &strCache);
    bool SerializeElementsWithCache(const JSHandle<JSObject> &obj, bool hasContent,
                                    CVector<std::pair<CString, int>> &strCache, uint32_t &cacheIndex,
                                    uint32_t elementSize);
    bool TryCacheSerializeKeys(const JSHandle<JSObject> &obj, bool hasContent,
                               CVector<std::pair<CString, int>> &strCache);
    bool TryCacheSerializeKeysFromPropertiesArray(const JSHandle<JSObject> &obj, bool hasContent,
                                                  CVector<std::pair<CString, int>> &strCache);
    bool TryCacheSerializeKeysFromEnumCache(const JSHandle<JSObject> &obj, bool hasContent,
                                            CVector<std::pair<CString, int>> &strCache);
    bool TryCacheSerializeKeysFromGlobalObject(const JSHandle<JSObject> &obj, bool hasContent,
                                               CVector<std::pair<CString, int>> &strCache);
    bool TryCacheSerializeKeysFromNameDictionary(const JSHandle<JSObject> &obj, bool hasContent,
                                                 CVector<std::pair<CString, int>> &strCache);
    bool SerializeKeysWithCache(const JSHandle<JSObject> &obj, bool hasContent,
                                CVector<std::pair<CString, int>> &strCache, uint32_t &cacheIndex);
    bool AppendJsonString(bool hasContent);
    bool DefaultSerializeKeys(const JSHandle<JSObject> &obj, bool hasContent);
    bool SerializeKeysFromCache(const JSHandle<JSObject> &obj, JSTaggedValue enumCache,
                                const JSHandle<TaggedArray> &propertiesArr, bool hasContent);
    bool SerializeKeysFromLayout(const JSHandle<JSObject> &obj, const JSHandle<JSHClass> &jsHclass,
                                 const JSHandle<TaggedArray> &propertiesArr, bool hasContent);
    bool SerializeKeysFromGlobalDictionary(const JSHandle<JSObject> &obj,
                                           const JSHandle<TaggedArray> &propertiesArr,
                                           bool hasContent);
    bool SerializeKeysFromNameDictionary(const JSHandle<JSObject> &obj,
                                         const JSHandle<TaggedArray> &propertiesArr,
                                         bool hasContent);
    bool SerializeKeyValue(const JSHandle<JSObject> &obj, JSTaggedValue key,
                           const JSHandle<TaggedArray> &propertiesArr, bool hasContent);
    bool DefaultSerializeElements(const JSHandle<JSObject> &obj, bool hasContent);
    bool DefaultSerializeObject(const JSTaggedValue &object, uint32_t numOfKeys, uint32_t numOfElements);

    inline void EraseKeyString(CString &keyStr, bool hasContent)
    {
        size_t keyLength = keyStr.length() + (hasContent ? 1 : 0) + 1;
        result_.erase(result_.end() - keyLength, result_.end());
    }

    inline void FastSerializeObjectKey(CString &key, bool hasContent)
    {
        if (hasContent) {
            result_ += ",";
        }

        result_ += key;
        result_ += ":";
    }

    inline int32_t FindCache(JSHClass *hclass, size_t numOfKeys)
    {
        size_t index = GetHash(hclass, numOfKeys);
        JSTaggedValue cacheHclass = hclassCache_->Get(index);
        if (cacheHclass != JSTaggedValue::Hole()) {
            if (JSHClass::Cast(cacheHclass.GetTaggedObject()) == hclass) {
                return index;
            } else {
                cacheHclass = hclassCache_->Get(++index);
                if (JSHClass::Cast(cacheHclass.GetTaggedObject()) == hclass) {
                    return index;
                } else {
                    return INVALID_INDEX;
                }
            }
        }
        return INVALID_INDEX;
    }

    inline void SetCache(JSHClass *hclass, size_t numOfKeys, CVector<std::pair<CString, int>> &value)
    {
        size_t index = GetHash(hclass, numOfKeys);
        JSTaggedValue cacheHclass = hclassCache_->Get(index);
        if (cacheHclass != JSTaggedValue::Hole()) {
            cacheHclass = hclassCache_->Get(++index);
            if (cacheHclass != JSTaggedValue::Hole()) {
                --index;
            }
        }
        hclassCache_->Set(thread_, index, JSTaggedValue(hclass));
        thread_->GetCurrentEcmaContext()->SetJsonStringifyCache(index, value);
    }

    inline size_t GetHash(JSHClass *hclass, size_t numOfKeys)
    {
        uintptr_t ptr = reinterpret_cast<uintptr_t>(hclass);
        size_t hash = (ptr + numOfKeys) & JSON_CACHE_MASK;
        return hash;
    }

    CString result_;
    JSThread *thread_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    CVector<JSHandle<JSTaggedValue>> stack_;
    JSMutableHandle<JSTaggedValue> handleKey_ {};
    JSMutableHandle<JSTaggedValue> handleValue_ {};
    bool cacheable_ {true};
    JSHandle<TaggedArray> hclassCache_ {};
};
}  // namespace panda::ecmascript::basekey
#endif  // ECMASCRIPT_BASE_FAST_JSON_STRINGIFY_H
