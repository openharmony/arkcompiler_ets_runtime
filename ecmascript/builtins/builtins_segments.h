/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_SEGMENTS_H
#define ECMASCRIPT_BUILTINS_BUILTINS_SEGMENTS_H

#include "ecmascript/base/builtins_base.h"

namespace panda::ecmascript::builtins {
class BuiltinsSegments : public base::BuiltinsBase {
public:
    // 18.5.2(1) %SegmentsPrototype%.containing ( index )
    static JSTaggedValue Containing(EcmaRuntimeCallInfo *argv);

    // 18.5.2(2) %SegmentsPrototype% [ @@iterator ] ( )
    static JSTaggedValue GetSegmentIterator(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_SEGMENTS_H