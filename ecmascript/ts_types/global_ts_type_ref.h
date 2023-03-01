/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TS_TYPES_GLOBAL_TS_TYPE_REF_H
#define ECMASCRIPT_TS_TYPES_GLOBAL_TS_TYPE_REF_H

#include "ecmascript/ecma_macros.h"
#include "libpandabase/utils/bit_field.h"

namespace panda::ecmascript {
/*
 * To maintain consistency with the frontend, ITERATOR_INSTANCE should always be at
 * the end of TSTypeKind, because there is no ITERATOR_INSTANCE in the frontend.
 */
enum class TSTypeKind : int {
    PRIMITIVE = 0,
    CLASS,
    CLASS_INSTANCE,
    FUNCTION,
    UNION,
    ARRAY,
    OBJECT,
    IMPORT,
    INTERFACE_KIND,
    ITERATOR_INSTANCE,
    UNKNOWN
};

enum class TSPrimitiveType : int {
    ANY = 0,
    NUMBER,
    BOOLEAN,
    VOID_TYPE,
    STRING,
    SYMBOL,
    NULL_TYPE,
    UNDEFINED,
    INT,
    BIG_INT,
    DOUBLE,
    END
};

/*
 * Indicates the position of the iterator object in the runtime table.
 * Position 0 in the runtime table stores the number of existing types, so start from 1.
 */
enum class TSRuntimeType : int {
    ITERATOR_RESULT = 1,
    ITERATOR_FUNCTION,
    ITERATOR
};

class GlobalTSTypeRef {
public:
    explicit GlobalTSTypeRef(uint32_t type = 0) : type_(type) {}
    explicit GlobalTSTypeRef(int moduleId, int localId) : type_(0)
    {
        SetLocalId(static_cast<uint16_t>(localId));
        SetModuleId(static_cast<uint16_t>(moduleId));
    }

    ~GlobalTSTypeRef() = default;

    static GlobalTSTypeRef Default()
    {
        return GlobalTSTypeRef(0u);
    }

    uint32_t GetType() const
    {
        return type_;
    }

    void SetType(uint32_t type)
    {
        type_ = type;
    }

    void Clear()
    {
        type_ = 0;
    }

    bool IsDefault() const
    {
        return type_ == 0;
    }

    bool operator <(const GlobalTSTypeRef &other) const
    {
        return type_ < other.type_;
    }

    bool operator ==(const GlobalTSTypeRef &other) const
    {
        return type_ == other.type_;
    }

    bool operator !=(const GlobalTSTypeRef &other) const
    {
        return type_ != other.type_;
    }

    void Dump() const
    {
        uint32_t moduleId = GetModuleId();
        uint32_t localId = GetLocalId();
        LOG_ECMA(ERROR) << " moduleId: " << moduleId
                        << " localId: " << localId;
    }

    static constexpr uint32_t GetSizeBits()
    {
        return SIZE_BITS;
    }

    static constexpr uint32_t LOCAL_ID_BITS = 14;
    static constexpr uint32_t MODULE_ID_BITS = 14;
    static constexpr uint32_t SIZE_BITS = LOCAL_ID_BITS + MODULE_ID_BITS;
    // MAX_LOCAL_ID types in one ts file can be stored in TSTypeTable
    static constexpr uint64_t MAX_LOCAL_ID = (1LLU << LOCAL_ID_BITS) - 1;
    // (MAX_MODULE_ID - TSModuleTable::DEFAULT_NUMBER_OF_TABLES) ts files with types can be stored in TSModuleTable
    static constexpr uint64_t MAX_MODULE_ID = (1LLU << MODULE_ID_BITS) - 1;

    FIRST_BIT_FIELD(Type, LocalId, uint32_t, LOCAL_ID_BITS);
    NEXT_BIT_FIELD(Type, ModuleId, uint32_t, MODULE_ID_BITS, LocalId);

    static inline bool IsVaildLocalId(uint32_t localId)
    {
        return (static_cast<uint64_t>(localId) & ~MAX_LOCAL_ID) == 0;
    }

    static inline bool IsVaildModuleId(uint32_t moduleId)
    {
        return (static_cast<uint64_t>(moduleId) & ~MAX_MODULE_ID) == 0;
    }

private:
    uint32_t type_ {0};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_GLOBAL_TS_TYPE_REF_H
