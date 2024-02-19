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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_SEGMENTER_H
#define ECMASCRIPT_BUILTINS_BUILTINS_SEGMENTER_H

#include "ecmascript/base/builtins_base.h"

namespace panda::ecmascript::builtins {
class BuiltinsSegmenter : public base::BuiltinsBase {
public:
    // 18.1.1 Intl.Segmenter ( [ locales [ , options ] ] )
    static JSTaggedValue SegmenterConstructor(EcmaRuntimeCallInfo *argv);

    // 18.2.2 Intl.Segmenter.supportedLocalesOf ( locales [ , options ] )
    static JSTaggedValue SupportedLocalesOf(EcmaRuntimeCallInfo *argv);

    // 18.3.3 Intl.Segmenter.prototype.segment ( string )
    static JSTaggedValue Segment(EcmaRuntimeCallInfo *argv);

    // 18.3.4 Intl.Segmenter.prototype.resolvedOptions ( )
    static JSTaggedValue ResolvedOptions(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_SEGMENTER_H