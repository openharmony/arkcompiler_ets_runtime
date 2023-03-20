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

#ifndef ECMASCRIPT_NUMBER_GATE_INFO_H
#define ECMASCRIPT_NUMBER_GATE_INFO_H
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/js_hclass.h"

namespace panda::ecmascript::kungfu {

enum class TypeInfo {
    NONE,
    INT1,
    INT32,
    FLOAT64,
    TAGGED,
};

class UseInfo {
public:
    UseInfo(uint8_t tag) : tag_(tag) {}
    static constexpr uint8_t UNUSED = 0;
    static constexpr uint8_t BOOL = 1 << 0;
    static constexpr uint8_t INT32 = 1 << 1;
    static constexpr uint8_t FLOAT64 = 1 << 2;
    static constexpr uint8_t NATIVE = BOOL | INT32 | FLOAT64;
    static constexpr uint8_t TAGGED = 1 << 3;
    bool AddUse(const UseInfo &UseInfo)
    {
        uint8_t oldTag = tag_;
        tag_ |= UseInfo.tag_;
        return oldTag != tag_;
    }
    bool UsedAsBool() const
    {
        return ((tag_ & BOOL) == BOOL);
    }
    bool UsedAsFloat64() const
    {
        return ((tag_ & FLOAT64) == FLOAT64);
    }
    bool UsedAsNative() const
    {
        return ((tag_ & NATIVE) != 0);
    }
    bool UsedAsTagged() const
    {
        return ((tag_ & TAGGED) != 0);
    }
    static UseInfo UnUsed()
    {
        return UseInfo(UNUSED);
    }
    static UseInfo BoolUse()
    {
        return UseInfo(BOOL);
    }
    static UseInfo Int32Use()
    {
        return UseInfo(INT32);
    }
    static UseInfo Float64Use()
    {
        return UseInfo(FLOAT64);
    }
    static UseInfo TaggedUse()
    {
        return UseInfo(TAGGED);
    }
private:
    uint8_t tag_ {0};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_NUMBER_GATE_INFO_H
