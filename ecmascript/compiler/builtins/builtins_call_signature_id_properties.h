/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_BUILTINS_CALL_SIGNATURE_ID_PROPERTIES_H
#define ECMASCRIPT_COMPILER_BUILTINS_CALL_SIGNATURE_ID_PROPERTIES_H

#include "ecmascript/compiler/builtins/builtins_call_signature_list.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::kungfu::stubcsigns {
enum BuiltinsStubCSignsID {
#define DEF_STUB_ID(name) name,
#define DEF_STUB_ID_DYN(name, type, ...) type##name,
        PADDING_BUILTINS_STUB_LIST(DEF_STUB_ID)
        BUILTINS_STUB_LIST(DEF_STUB_ID, DEF_STUB_ID_DYN, DEF_STUB_ID)
        NUM_OF_BUILTINS_STUBS,
        BUILTINS_NOSTUB_LIST(DEF_STUB_ID)
        AOT_BUILTINS_STUB_LIST(DEF_STUB_ID)
        AOT_BUILTINS_INLINE_LIST(DEF_STUB_ID)
#undef DEF_STUB_ID_DYN
#undef DEF_STUB_ID
        NUM_OF_BUILTINS_ID,
        BUILTINS_CONSTRUCTOR_STUB_FIRST = BooleanConstructor,
        TYPED_BUILTINS_FIRST = JsonStringify,
        TYPED_BUILTINS_LAST = IteratorProtoReturn,
        INVALID = 0xFFFF,
};
} // namespace panda::ecmascript::kungfu::stubcsigns

namespace panda::ecmascript::kungfu::builtinssubid {
// SubID indicates the index in callSigns[] for builtins stubs, including the StwCopy version ones
enum SubID {
#define DEF_STUB_ID(name) name,
#define DEF_STUB_ID_DYN(name, type, ...) type##name,
    PADDING_BUILTINS_STUB_LIST(DEF_STUB_ID)
    BUILTINS_STUB_LIST(DEF_STUB_ID, DEF_STUB_ID_DYN, DEF_STUB_ID)
#undef DEF_STUB_ID_DYN
#undef DEF_STUB_ID
#define DEF_STUB_ID(name) name##StwCopy,
#define DEF_STUB_ID_DYN(name, type, ...) type##name##StwCopy,
    BUILTINS_STW_COPY_STUB_LIST(DEF_STUB_ID, DEF_STUB_ID_DYN, DEF_STUB_ID)
#undef DEF_STUB_ID_DYN
#undef DEF_STUB_ID
    NUM_OF_BUILTINS_STUBS_EXTEND,
};
} // namespace panda::ecmascript::kungfu::builtinssubid

#endif // ECMASCRIPT_COMPILER_BUILTINS_CALL_SIGNATURE_ID_PROPERTIES_H
