/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ENUM_CACHE_H
#define ECMASCRIPT_ENUM_CACHE_H
 
#include "ecmascript/ecma_macros.h"
#include "ecmascript/mem/barriers.h"

// EnumCacheKind is only used in for-in.
enum class EnumCacheKind : uint8_t {
    NONE,          // No Cache, go slow path
    SIMPLE,        // Simple enum cache
    PROTOCHAIN,    // Prototype chain info enum cache
};

// +---------------+-------------------------------------------+-------------------------------------------------+
// | EnumCacheKind | Status Description                        | Valid Conditions                                |
// +---------------+-------------------------------------------+-------------------------------------------------+
// | None          | - Uninitialized or in SlowPath mode.      | - None (No additional conditions required)      |
// |               |   No active cache optimization.           |                                                 |
// +---------------+-------------------------------------------+-------------------------------------------------+
// | SIMPLE        | - Fast path for simple enum cache check.  | - Receiver's proto is NOT a HeapObject,         |
// |               |   Indicates:                              |   OR                                            |
// |               |   • Properties exist ONLY on the object   | - Receiver.proto.ProtoChainInfoEnumCache ==     |
// |               |   itself (no prototype chain properties)  |   Undefined. (Ensure no keys on prototype chain)|
// |               |   • NO elements (indexed properties) on   |                                                 |
// |               |     self/proto                            |                                                 |
// +---------------+-------------------------------------------+-------------------------------------------------+
// | PROTOCHAIN    | - Fast path for protochain enum cache.    | - Receiver's proto is a HeapObject, AND         |
// |               |   Indicates:                              | - Receiver.ProtoChainInfoEnumCache ==           |
// |               |   • Properties exist on the object itself |   receiver.proto.EnumCacheAll (Cache matches    |
// |               |   or prototype chain                      |   prototype's full enum cache), AND             |
// |               |   • NO elements (indexed properties) on   | - Receiver.ProtoChainInfoEnumCache != Null      |
// |               |     self/proto                            |   (Valid cache exists).                         |
// +---------------+-------------------------------------------+-------------------------------------------------+

namespace panda {
namespace ecmascript {
class EnumCache : public TaggedObject {
public:
    CAST_CHECK(EnumCache, IsEnumCache);

    static constexpr size_t ENUM_CACHE_OWN_OFFSET = TaggedObjectSize();
    
    static bool CheckSelfAndProtoEnumCache(const JSThread *thread, JSTaggedValue enumCacheOwn,
        JSTaggedValue enumCacheProto)
    {
        if (!enumCacheOwn.IsEnumCache() || !enumCacheProto.IsEnumCache()) {
            return false;
        }
        auto keyOwn =  EnumCache::Cast(enumCacheOwn.GetTaggedObject())->GetProtoChainInfoEnumCache(thread);
        auto keyProto = EnumCache::Cast(enumCacheProto.GetTaggedObject())->GetEnumCacheAll(thread);
        if (keyOwn != keyProto || keyOwn == JSTaggedValue::Null()) {
            return false;
        }
        return true;
    }

    inline bool IsEnumCacheAllValid(const JSThread *thread) const
    {
        return GetEnumCacheAll(thread) != JSTaggedValue::Null();
    }

    inline bool IsEnumCacheOwnValid(const JSThread *thread) const
    {
        return GetEnumCacheOwn(thread) != JSTaggedValue::Null();
    }

    inline bool IsEnumCacheProtoInfoUndefined(const JSThread *thread) const
    {
        return GetProtoChainInfoEnumCache(thread) == JSTaggedValue::Undefined();
    }

    inline void SetInvalidState(const JSThread *thread)
    {
        SetEnumCacheAll(thread, JSTaggedValue::Null());
        SetProtoChainInfoEnumCache(thread, JSTaggedValue::Null());
    }

    // EnumCacheOwn holds enum keys on itself.
    ACCESSORS(EnumCacheOwn, ENUM_CACHE_OWN_OFFSET, ENUM_CACHE_ALL_OFFSET);

    // EnumCacheAll holds all enum keys on the prototype chain, including its own keys.
    ACCESSORS(EnumCacheAll, ENUM_CACHE_ALL_OFFSET, PROTO_CHAIN_INFO_ENUM_CACHE_OFFSET);
    
    // ProtoChainInfoEnumCache holds all enum keys on the prototype chain, excluding its own keys.
    ACCESSORS(ProtoChainInfoEnumCache, PROTO_CHAIN_INFO_ENUM_CACHE_OFFSET, ENUM_CACHE_KIND_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(EnumCacheKind, uint32_t, ENUM_CACHE_KIND_OFFSET, LAST_OFFSET);

    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    
    DECL_VISIT_OBJECT(ENUM_CACHE_OWN_OFFSET, ENUM_CACHE_KIND_OFFSET)

    DECL_DUMP()
};
}  // namespace ecmascript
}  // namespace panda
 
#endif  // ECMASCRIPT_IC_ENUM_CACHE_H
 