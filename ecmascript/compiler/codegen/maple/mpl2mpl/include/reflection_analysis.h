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

#ifndef MPL2MPL_INCLUDE_REFLECTION_ANALYSIS_H
#define MPL2MPL_INCLUDE_REFLECTION_ANALYSIS_H

namespace maple {
// maple field index definition
enum class ClassRO : uint32 {
    kClassName,
    kIfields,
    kMethods,
    kSuperclass,
    kNumOfFields,
    kNumofMethods,
#ifndef USE_32BIT_REF
    kFlag,
    kNumOfSup,
    kPadding,
#endif  //! USE_32BIT_REF
    kMod,
    kAnnotation,
    kClinitAddr
};

enum StaticFieldName {
    kClassName = 0,
    kFieldName = 1,
    kTypeName = 2,
};

enum class ClassProperty : uint32 {
    kShadow,
    kMonitor,
    kClassloader,
    kObjsize,
#ifdef USE_32BIT_REF
    FLAG,
    NUMOFSUP,
#endif  // USE_32BIT_REF
    kItab,
    kVtab,
    kGctib,
    kInfoRo,
#ifdef USE_32BIT_REF
    kInstanceOfCacheFalse,
#endif
    kClint
};

enum class MethodProperty : uint32 {
    kVtabIndex,
    kDeclarclass,
    kPaddrData,
    kMod,
    kMethodName,
    kSigName,
    kAnnotation,
    kFlag,
    kArgsize,
#ifndef USE_32BIT_REF
    padding
#endif
};

enum class MethodInfoCompact : uint32 { kVtabIndex, kPaddrData };

enum class FieldProperty : uint32 {
    kPOffset,
    kMod,
    kFlag,
    kIndex,
    kTypeName,
    kName,
    kAnnotation,
    kDeclarclass,
    kPClassType
};

enum class FieldPropertyCompact : uint32 { kPOffset, kMod, kTypeName, kIndex, kName, kAnnotation };

enum class MethodSignatureProperty : uint32 { kSignatureOffset, kParameterTypes };

}  // namespace maple
#endif  // MPL2MPL_INCLUDE_REFLECTION_ANALYSIS_H
