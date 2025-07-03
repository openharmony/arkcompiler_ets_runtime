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

#include "ecmascript/log_wrapper.h"
#include "libpandabase/utils/bit_field.h"

namespace panda::ecmascript {
enum class TSTypeKind : uint8_t {
    PRIMITIVE = 0,
    // the following typekinds are recorded in abc files and are consistent with the corresponding values
    CLASS,
    CLASS_INSTANCE,
    FUNCTION,
    UNION,
    ARRAY,
    OBJECT,
    IMPORT,
    INTERFACE,
    BUILTIN_INSTANCE,
    GENERIC_INSTANCE,
    INDEXSIG,

    // the following typekinds are not recorded in abc files and will be created at compile time
    ITERATOR_INSTANCE,
    NAMESPACE,
    UNKNOWN,

    TYPEKIND_FIRST = CLASS,
    TYPEKIND_LAST = INDEXSIG,
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

enum class ModuleTableIdx : uint8_t {
    PRIMITIVE = 0,
    BUILTIN,
    INFERRED,
    RUNTIME,
    GENERICS,
    NUM_OF_DEFAULT_TABLES,
};

class GlobalTSTypeRef {
public:
    explicit GlobalTSTypeRef(uint32_t type = 0) : type_(type) {}
    GlobalTSTypeRef(int moduleId, int localId) : type_(0)
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

#define IS_TYPE_MODULE_LIST(V)               \
    V(Primitive, ModuleTableIdx::PRIMITIVE)  \
    V(Builtin, ModuleTableIdx::BUILTIN)      \
    V(Inferred, ModuleTableIdx::INFERRED)    \
    V(Runtime, ModuleTableIdx::RUNTIME)      \
    V(Generics, ModuleTableIdx::GENERICS)

#define IS_TYPE_MODULE(NAME, MODULEID)                              \
    inline bool Is##NAME##Module() const                            \
    {                                                               \
        return (GetModuleId() == static_cast<uint32_t>(MODULEID));  \
    }

    IS_TYPE_MODULE_LIST(IS_TYPE_MODULE)
#undef IS_TYPE_MODULE

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
    // (MAX_MODULE_ID - ModuleTableIdx::NUM_OF_DEFAULT_TABLES) ts files with types can be stored in TSModuleTable
    static constexpr uint64_t MAX_MODULE_ID = (1LLU << MODULE_ID_BITS) - 1;

    using LocalIdBits = BitField<uint32_t, 0, LOCAL_ID_BITS>;
    using ModuleIdBits = LocalIdBits::NextField<uint32_t, MODULE_ID_BITS>;

    inline uint32_t GetLocalId() const
    {
        return LocalIdBits::Decode(GetType());
    }

    inline void SetLocalId(uint32_t value)
    {
        SetType(LocalIdBits::Update(GetType(), value));
    }

    inline uint32_t GetModuleId() const
    {
        return ModuleIdBits::Decode(GetType());
    }

    inline void SetModuleId(uint32_t value)
    {
        SetType(ModuleIdBits::Update(GetType(), value));
    }

    static inline bool IsValidLocalId(uint32_t localId)
    {
        return (static_cast<uint64_t>(localId) & ~MAX_LOCAL_ID) == 0;
    }

    static inline bool IsValidModuleId(uint32_t moduleId)
    {
        return (static_cast<uint64_t>(moduleId) & ~MAX_MODULE_ID) == 0;
    }

private:
    uint32_t type_ {0};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_GLOBAL_TS_TYPE_REF_H
