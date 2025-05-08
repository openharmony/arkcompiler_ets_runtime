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

#ifndef ARK_HEAP_SNAPSHOT_JSON_SERIALIZER_H
#define ARK_HEAP_SNAPSHOT_JSON_SERIALIZER_H
#include <fstream>
#include <climits>
#include <sstream>
#include "common_components/common_runtime/src/inspector/file_stream.h"
#include "common_components/common_runtime/src/base/c_string.h"
#include "common_components/common_runtime/src/inspector/heap_data.h"
#include "securec.h"

namespace panda {
class ArkHeapDataForIDE : public ArkHeapData {
public:
    using HeapDataIDForIDE = u4;
    bool Serialize();
    void SerializeFixedHeader();
    void SerializeString();
    void SerializeRecordHeader(const u1 tag, const u4 time);
    void SerializeAllClassLoad();
    void SerializeAllStructClassLoad();
    void SerializeAllClass();
    void SerializeAllStructClass();
    void SerializeStackTrace();
    void SerializeStartThread();
    void SerializeHeapDump();
    void SerializeClassLoad(TypeInfo* klass, ArkHeapDataStringId klassId, const u1 tag);
    void SerializeAllObjects();
    void SerializeGlobalRoot(BaseObject*& obj, const u1 tag);
    void SerializeUnknownRoot(BaseObject*& obj, const u1 tag);
    void SerializeLocalRoot(BaseObject*& obj, const u1 tag, const u4 tid, const u1 depth);
    void SerializeThreadObjectRoot(BaseObject*& obj, const u1 tag, const u4 tid, const u4 stackTraceIdx);
    void SerializeObjectArray(BaseObject*& obj, const u1 tag);
    void SerializeStructArray(BaseObject*& obj, const u1 tag);
    void SerializePrimitiveArray(BaseObject*& obj, const u1 tag);
    void SerializeInstance(BaseObject*& obj, const u1 tag);
    void SerializeClass(TypeInfo* klass, ArkHeapDataStringId klassId, const u1 tag);
    void SerializeStructClass(TypeInfo* klass, ArkHeapDataStringId klassId, const u1 tag);
    u4 GetId(ArkHeapDataStringId klassId);
private:
    StreamWriter* writer_ = nullptr;
    std::unordered_map<ArkHeapDataStringId, u4> stringIdxMap_;
    u4 stringIdx_ = 0;
};
}
#endif
