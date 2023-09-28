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

#ifndef ECMASCRIPT_PROPERTY_ACCESSOR_H
#define ECMASCRIPT_PROPERTY_ACCESSOR_H

#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
class JSObject;
class TaggedArray;
class PropertyAccessor {
public:
    PropertyAccessor(JSThread *thread, JSHandle<JSTaggedValue> object);

    JSHandle<JSTaggedValue> GetKeysFast();
    JSHandle<JSTaggedValue> GetKeysSlow();

    JSHandle<JSTaggedValue> GetCachedHclass();
    uint32_t GetActualKeyLength() const;

private:
    void PreLoad();
    void CollectPrototypeInfo();
    void InitSimplePropertiesEnumCache();
    void AccumulateKeyLength(uint32_t length);
    void AccumulateShadowKeyLength(uint32_t length);
    void PushRemainingKeys(JSHandle<JSObject> object, std::vector<JSHandle<TaggedArray>> &remainings);
    void MergeRemainings(const std::vector<JSHandle<TaggedArray>> &remainings,
                         const std::vector<JSHandle<JSTaggedValue>> &visited);
    void SetActualKeyLength(uint32_t length);
    void AddKeysEndIfNeeded(JSHandle<TaggedArray> keys);
    void TryInitEnumCacheWithProtoChainInfo();

    JSThread *thread_{nullptr};
    JSMutableHandle<JSTaggedValue> receiver_;
    JSMutableHandle<JSTaggedValue> fastKeysArray_;
    JSMutableHandle<JSTaggedValue> cachedHclass_;
    uint32_t keyLength_ {0};
    uint32_t shadowKeyLength_ {0};
    // receiver has no elements, and is not dictionary mode and has empty prototype
    bool onlyHasSimpleProperties_ {true};
    bool canUseEnumCache_ {true};
    bool hasSlowProperties_ {false};
    JSMutableHandle<TaggedArray> slowKeysArray_;
    uint32_t acutalKeyLength_ {0};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PROPERTY_ACCESSOR_H
