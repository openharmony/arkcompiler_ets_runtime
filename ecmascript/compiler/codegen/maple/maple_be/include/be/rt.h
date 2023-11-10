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

#ifndef MAPLEBE_INCLUDE_BE_RT_H
#define MAPLEBE_INCLUDE_BE_RT_H

#include <cstdint>
#include <string>

namespace maplebe {
/*
 * This class contains constants about the ABI of the runtime, such as symbols
 * for GC-related metadata in generated binary files.
 */
class RTSupport {
public:
    static RTSupport &GetRTSupportInstance()
    {
        static RTSupport RtSupport;
        return RtSupport;
    }
    uint64_t GetObjectAlignment() const
    {
        return kObjectAlignment;
    }
    int64_t GetArrayContentOffset() const
    {
        return kArrayContentOffset;
    }
    int64_t GetArrayLengthOffset() const
    {
        return kArrayLengthOffset;
    }
    uint64_t GetFieldSize() const
    {
        return kRefFieldSize;
    }
    uint64_t GetFieldAlign() const
    {
        return kRefFieldAlign;
    }

protected:
    uint64_t kObjectAlignment;  /* Word size. Suitable for all Java types. */
    uint64_t kObjectHeaderSize; /* java object header used by MM. */

#ifdef USE_32BIT_REF
    uint32_t kRefFieldSize; /* reference field in java object */
    uint32_t kRefFieldAlign;
#else
    uint32_t kRefFieldSize; /* reference field in java object */
    uint32_t kRefFieldAlign;
#endif /* USE_32BIT_REF */
    /* The array length offset is fixed since CONTENT_OFFSET is fixed to simplify code */
    int64_t kArrayLengthOffset; /* shadow + monitor + [padding] */
    /* The array content offset is aligned to 8B to alow hosting of size-8B elements */
    int64_t kArrayContentOffset; /* fixed */
    int64_t kGcTibOffset;
    int64_t kGcTibOffsetAbs;

private:
    RTSupport()
    {
        kObjectAlignment = 8;
        kObjectHeaderSize = 8;
#ifdef USE_32BIT_REF
        kRefFieldSize = 4;
        kRefFieldAlign = 4;
#else
        kRefFieldSize = 8;
        kRefFieldAlign = 8;
#endif /* USE_32BIT_REF */
        kArrayLengthOffset = 12;
        kArrayContentOffset = 16;
        kGcTibOffset = -8;
        kGcTibOffsetAbs = -kGcTibOffset;
    }
    static const std::string kObjectMapSectionName;
    static const std::string kGctibLabelArrayOfObject;
    static const std::string kGctibLabelJavaObject;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_BE_RT_H */