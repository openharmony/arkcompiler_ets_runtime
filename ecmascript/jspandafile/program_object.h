/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JSPANDAFILE_PROGRAM_OBJECT_H
#define ECMASCRIPT_JSPANDAFILE_PROGRAM_OBJECT_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda {
namespace ecmascript {
class JSThread;
class NativeAreaAllocator;

class Program : public ECMAObject {
public:
    DECL_CAST(Program)

    static constexpr size_t MAIN_FUNCTION_OFFSET = ECMAObject::SIZE;
    ACCESSORS(MainFunction, MAIN_FUNCTION_OFFSET, SIZE)

    DECL_VISIT_OBJECT(MAIN_FUNCTION_OFFSET, SIZE)
    DECL_DUMP()
};

/*
 *       ConstantPool
 *      +------------+
 *      |  hClass    +
 *      +------------+
 *      |  length    |
 *      +------------+
 *      |  cache...  |
 *      +------------+
 *      |js_pandafile|
 *      +------------+
 */
class ConstantPool : public TaggedArray {
public:
    static constexpr size_t JS_PANDA_FILE_INDEX = 1;
    static constexpr size_t RESERVED_POOL_LENGTH = JS_PANDA_FILE_INDEX;

    static ConstantPool *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsConstantPool());
        return static_cast<ConstantPool *>(object);
    }

    static size_t ComputeSize(uint32_t cacheSize)
    {
        return TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), cacheSize + RESERVED_POOL_LENGTH);
    }

    inline void InitializeWithSpecialValue(JSTaggedValue initValue, uint32_t capacity)
    {
        ASSERT(initValue.IsSpecial());
        SetLength(capacity + RESERVED_POOL_LENGTH);
        SetExtractLength(0);
        for (uint32_t i = 0; i < capacity; i++) {
            size_t offset = JSTaggedValue::TaggedTypeSize() * i;
            Barriers::SetDynPrimitive<JSTaggedType>(GetData(), offset, initValue.GetRawData());
        }
        SetJSPandaFile(nullptr);
    }

    inline uint32_t GetCacheLength() const
    {
        return GetLength() - RESERVED_POOL_LENGTH;
    }

    inline void SetJSPandaFile(const void *jsPandaFile)
    {
        Barriers::SetDynPrimitive(GetData(), GetJSPandaFileOffset(), jsPandaFile);
    }

    inline void *GetJSPandaFile() const
    {
        return Barriers::GetDynValue<void *>(GetData(), GetJSPandaFileOffset());
    }

    inline void SetObjectToCache(JSThread *thread, uint32_t index, JSTaggedValue value)
    {
        Set(thread, index, value);
    }

    inline JSTaggedValue GetObjectFromCache(uint32_t index) const
    {
        return Get(index);
    }

    std::string PUBLIC_API GetStdStringByIdx(size_t index) const;

    DECL_VISIT_ARRAY(DATA_OFFSET, GetCacheLength());
    DECL_VISIT_NATIVE_FIELD(GetLastOffset() - JSTaggedValue::TaggedTypeSize(), GetLastOffset());

    DECL_DUMP()

private:
    inline size_t GetJSPandaFileOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * (GetLength() - JS_PANDA_FILE_INDEX);
    }

    inline size_t GetLastOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * GetLength() + LAST_OFFSET;
    }
};
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_JSPANDAFILE_PROGRAM_OBJECT_H
