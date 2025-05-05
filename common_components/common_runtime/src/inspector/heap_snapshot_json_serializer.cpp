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
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <pthread.h>
#include "common_components/common_runtime/src/mutator/mutator_manager.h"
#include "common_components/common_runtime/src/unwind_stack/stack_metadata_helper.h"
#include "common_components/common_runtime/src/object_model/m_array.inline.h"
#include "common_components/common_runtime/src/inspector/file_stream.h"
#include "common_components/common_runtime/src/inspector/heap_data.h"
#include "common_components/common_runtime/src/inspector/heap_snapshot_json_serializer.h"

namespace panda {

bool ArkHeapDataForIDE::Serialize()
{
    ScopedStopTheWorld scopedStopTheWorld("serialize-heap-data");
    ProcessHeap();
    HeapProfilerStream* stream = &panda::HeapProfilerStream::GetInstance();
    stream->SetContext(DUMPHEAPSNAPSHOT);
    writer_ = new StreamWriter(stream);
    writer_->WriteChar('{');
    SerializeFixedHeader();
    SerializeString();
    SerializeAllClassLoad();
    SerializeAllStructClassLoad();
    SerializeStackTrace();
    SerializeStartThread();
    SerializeHeapDump();
    writer_->WriteChar('}');
    writer_->End();
    stream->SetContext(END);
    writer_->WriteString("{\"id\":");
    writer_->WriteString(stream->GetMessageID());
    writer_->WriteString(",\"result\":{}");
    writer_->End();
    delete writer_;
    return true;
}

void ArkHeapDataForIDE::SerializeFixedHeader()
{
    const u4 idSize = sizeof(u4);
    struct timeval timeNow {};
    gettimeofday(&timeNow, nullptr);
    const uint64_t msecsTime = (timeNow.tv_sec * 1000) + (timeNow.tv_usec / 1000);
    const uint32_t timeHigh = static_cast<uint32_t>(msecsTime >> 32);
    const uint32_t timeLow = static_cast<uint32_t>(msecsTime & 0xFFFFFFFF);
    writer_->WriteString("\\\"HEADER\\\":[");
    writer_->WriteNumber(idSize);
    writer_->WriteChar(',');
    writer_->WriteNumber(timeHigh);
    writer_->WriteChar(',');
    writer_->WriteNumber(timeLow);
    writer_->WriteChar(']');
    writer_->WriteChar(',');
}

void ArkHeapDataForIDE::SerializeString()
{
    writer_->WriteString("\\\"STRING\\\":[");
    bool isFirstElement = true;
    for (auto string : strings) {
        if (!isFirstElement) {
            writer_->WriteChar(',');
        } else {
            isFirstElement = false;
        }
        writer_->WriteChar('[');
        ArkHeapDataStringId id = string.second;
        writer_->WriteNumber(GetId(id));
        writer_->WriteChar(',');
        const CString str = string.first;
        writer_->WriteString("\\\"");
        writer_->WriteString(str);
        writer_->WriteString("\\\"");
        writer_->WriteChar(']');
    }
    writer_->WriteChar(']');
    writer_->WriteChar(',');
}

void ArkHeapDataForIDE::SerializeAllClassLoad()
{
    writer_->WriteString("\\\"CLASSLOAD\\\":[");
    bool isFirstElement = true;
    for (auto klassInfo : dumpClassMap) {
        if (!isFirstElement) {
            writer_->WriteChar(',');
        } else {
            isFirstElement = false;
        }
        SerializeClassLoad(klassInfo.first, klassInfo.second, TAG_CLASS_LOAD);
    }
    writer_->WriteChar(']');
    writer_->WriteChar(',');
}

void ArkHeapDataForIDE::SerializeAllClass()
{
    writer_->WriteString("\\\"CLASS\\\":[");
    bool isFirstElement = true;
    for (auto klassInfo : dumpClassMap) {
        if (!isFirstElement) {
            writer_->WriteChar(',');
        } else {
            isFirstElement = false;
        }
        SerializeClass(klassInfo.first, klassInfo.second, TAG_CLASS_DUMP);
    }
    writer_->WriteChar(']');
    writer_->WriteChar(',');
}

void ArkHeapDataForIDE::SerializeAllStructClass()
{
    writer_->WriteString("\\\"STRUCTCLASS\\\":[");
    bool isFirstElement = true;
    for (auto klassInfo : dumpStructClassMap) {
        if (!isFirstElement) {
            writer_->WriteChar(',');
        } else {
            isFirstElement = false;
        }
        SerializeStructClass(klassInfo.first, klassInfo.second, TAG_CLASS_DUMP);
    }
    writer_->WriteChar(']');
    writer_->WriteChar(',');
}

void ArkHeapDataForIDE::SerializeClassLoad(TypeInfo* klass, ArkHeapDataStringId klassId, const u1 tag)
{
    writer_->WriteChar('[');
    writer_->WriteNumber(GetId(reinterpret_cast<ArkHeapDataStringId>(klass)));
    writer_->WriteChar(',');
    writer_->WriteNumber(GetId(reinterpret_cast<ArkHeapDataStringId>(klassId)));
    writer_->WriteChar(']');
}

void ArkHeapDataForIDE::SerializeAllStructClassLoad()
{
    writer_->WriteString("\\\"STRUCTCLASSLOAD\\\":[");
    bool isFirstElement = true;
    for (auto klassInfo : dumpStructClassMap) {
        if (!isFirstElement) {
            writer_->WriteChar(',');
        } else {
            isFirstElement = false;
        }
        SerializeClassLoad(klassInfo.first, klassInfo.second, TAG_CLASS_LOAD);
    }
    writer_->WriteChar(']');
    writer_->WriteChar(',');
}

void ArkHeapDataForIDE::SerializeStackFrame(FrameInfo& frame, uint32_t frameIdx)
{
    writer_->WriteChar('[');
    if (frameIdx > 0 && frame.GetFrameType() == FrameType::NATIVE) {
        return;
    }
    if (frame.GetFrameType() == FrameType::MANAGED) {
        StackMetadataHelper stackMetadataHelper(frame);
        methodName = frame.GetFuncName();
        fileName = frame.GetFileName();
        lineNumber = stackMetadataHelper.GetLineNumber();
    } else {
        Os::Loader::BinaryInfo binInfo;
        (void)Os::Loader::GetBinaryInfoFromAddress(frame.mFrame.GetIP(), &binInfo);
        fileName = CString(binInfo.filePathName);
        methodName = CString(binInfo.symbolName);
    }
    writer_->WriteNumber(frames[&frame]);
    writer_->WriteChar(',');
    writer_->WriteNumber(LookupStringId(methodName.Str()));
    writer_->WriteChar(',');
    writer_->WriteNumber(LookupStringId(fileName.Str()));
    writer_->WriteChar(',');
    writer_->WriteNumber(reinterpret_cast<u4>(lineNumber));
}


void ArkHeapDataForIDE::SerializeStackTrace()
{
    for (auto trace = stacktraces.begin(); trace != stacktraces.end(); trace++) {
        auto env = std::getenv("DumpStackDepth");
        // 10: Default Limit Max Dump Depth as 10 frames
        size_t size = CString::ParseNumFromEnv(env) == 0 ? 10 : CString::ParseNumFromEnv(env);
        size_t depth = trace->first->stacks.size() > size ? size : trace->first->stacks.size();
        std::vector<FrameInfo*> stack = trace->first->stacks;
        writer_->WriteString("\\\"STACKFRAME\\\":[");
        for (size_t i = 0; i < depth; ++i) {
            SerializeStackFrame(*stack[i], i);
        }
        writer_->WriteChar(']');
        writer_->WriteChar(',');
        writer_->WriteString("\\\"STACKTRACE\\\":[");
        writer_->WriteNumber(trace->second);
        writer_->WriteChar(',');
        writer_->WriteNumber(trace->first->GetStackTid());
        writer_->WriteChar(',');
        writer_->WriteNumber(depth);
        writer_->WriteChar(',');
        writer_->WriteChar('[');
        for (size_t i = 0; i < depth; ++i) {
            writer_->WriteNumber(frames[stack[i]]);
        }
        writer_->WriteChar(']');
    }
    writer_->WriteChar(']');
    writer_->WriteChar(',');
}

void ArkHeapDataForIDE::SerializeStartThread()
{
    writer_->WriteString("\\\"STARTTHREAD\\\":[");
    for (auto trace = stacktraces.begin(); trace != stacktraces.end(); trace++) {
        writer_->WriteNumber(trace->first->GetStackTid());
        writer_->WriteChar(',');
        writer_->WriteNumber(GetId(threadObjectId++));
        writer_->WriteChar(',');
        writer_->WriteNumber(trace->second);
        writer_->WriteChar(',');
        CString threadNameAll = trace->first->GetThreadName();
        writer_->WriteNumber(GetId(LookupStringId(threadNameAll)));
    }
    writer_->WriteChar(']');
    writer_->WriteChar(',');
}

void ArkHeapDataForIDE::SerializeHeapDump()
{
    SerializeAllClass();
    SerializeAllStructClass();
    SerializeAllObjects();
}

void ArkHeapDataForIDE::SerializeAllObjects()
{
    writer_->WriteString("\\\"OBJECTS\\\":[");
    bool isFirstElement = true;
    for (auto objectInfo : dumpObjects) {
        if (!isFirstElement) {
            writer_->WriteChar(',');
        } else {
            isFirstElement = false;
        }
        switch (objectInfo.tag) {
            case TAG_ROOT_THREAD_OBJECT:
                SerializeThreadObjectRoot(objectInfo.obj, objectInfo.tag, objectInfo.threadId, 0);
                break;
            case TAG_ROOT_LOCAL:
                SerializeLocalRoot(objectInfo.obj, objectInfo.tag, objectInfo.threadId, objectInfo.frameNum);
                break;
            case TAG_ROOT_GLOBAL:
                SerializeGlobalRoot(objectInfo.obj, objectInfo.tag);
                break;
            case TAG_ROOT_UNKNOWN:
                SerializeUnknownRoot(objectInfo.obj, objectInfo.tag);
                break;
            case TAG_OBJECT_ARRAY_DUMP:
                SerializeObjectArray(objectInfo.obj, objectInfo.tag);
                break;
            case  TAG_STRUCT_ARRAY_DUMP:
                SerializeStructArray(objectInfo.obj, objectInfo.tag);
                break;
            case TAG_PRIMITIVE_ARRAY_DUMP:
                SerializePrimitiveArray(objectInfo.obj, objectInfo.tag);
                break;
            case TAG_INSTANCE_DUMP:
                SerializeInstance(objectInfo.obj, objectInfo.tag);
                break;
            default:
                break;
        }
    }
    writer_->WriteChar(']');
}

/*
 * Record Global Root Info:
 *     u1 tag;     //denoting the type of this sub-record
 *     ID objId;   // object ID
 */
void ArkHeapDataForIDE::SerializeGlobalRoot(BaseObject*& obj, const u1 tag)
{
    writer_->WriteChar('[');
    writer_->WriteNumber(tag);
    writer_->WriteChar(',');
    ArkHeapDataID objAddr = (reinterpret_cast<ArkHeapDataID>(obj));
    u4 id = GetId(objAddr);
    CString name = obj->GetTypeInfo()->GetName();
    writer_->WriteNumber(id);
    writer_->WriteChar(']');
}

/*
 * Record Unknown Root Info:
 *     u1 tag;     // denoting the type of this sub-record
 *     ID objId;   // object ID
 */
void ArkHeapDataForIDE::SerializeUnknownRoot(BaseObject*& obj, const u1 tag)
{
    writer_->WriteChar('[');
    writer_->WriteNumber(tag);
    writer_->WriteChar(',');
    ArkHeapDataID objAddr = (reinterpret_cast<ArkHeapDataID>(obj));
    u4 id = GetId(objAddr);
    writer_->WriteNumber(id);
    writer_->WriteChar(']');
}

/*
 * Record Local Root Info:
 *     u1 tag;         // denoting the type of this sub-record
 *     ID objId;       // object ID
 *     u4 threadIdx;   // thread serial number
 *     u4 frame;       // frame number in stack trace (-1 for empty)
 */
void ArkHeapDataForIDE::SerializeLocalRoot(BaseObject*& obj, const u1 tag, const u4 tid, const u1 depth)
{
    writer_->WriteChar('[');
    writer_->WriteNumber(tag);
    writer_->WriteChar(',');
    ArkHeapDataID objAddr = (reinterpret_cast<ArkHeapDataID>(obj));
    u4 id = GetId(objAddr);
    writer_->WriteNumber(id);
    writer_->WriteChar(',');
    writer_->WriteNumber(tid);
    writer_->WriteChar(',');
    writer_->WriteNumber(depth);
    writer_->WriteChar(']');
}

/*
 * Record Thread Object Root Info:
 *     u1 tag;             // denoting the type of this sub-record
 *     ID threadObjId;     // thread object ID
 *     u4 threadIdx;       // thread serial number
 *     u4 stackTraceIdx;   // stack trace serial number
 *
 */
void ArkHeapDataForIDE::SerializeThreadObjectRoot(BaseObject*& obj, const u1 tag, const u4 tid, const u4 stackTraceIdx)
{
    writer_->WriteChar('[');
    writer_->WriteNumber(tag);
    writer_->WriteChar(',');
    ArkHeapDataID objAddr = (reinterpret_cast<ArkHeapDataID>(obj));
    u4 id = GetId(objAddr);
    writer_->WriteNumber(id);
    writer_->WriteChar(',');
    writer_->WriteNumber(tid);
    writer_->WriteChar(',');
    writer_->WriteNumber(stackTraceIdx);
    writer_->WriteChar(']');
}

/*
 * Record Class Info:
 *     u1 tag;          // denoting the type of this sub-record
 *     ID classObjId;   // class object ID
 *     u4 size;         // instance size (in bytes)
 */
void ArkHeapDataForIDE::SerializeClass(TypeInfo* klass, ArkHeapDataStringId klassId, const u1 tag)
{
    writer_->WriteChar('[');
    writer_->WriteNumber(tag);
    writer_->WriteChar(',');
    writer_->WriteNumber(GetId(reinterpret_cast<ArkHeapDataStringId>(klass)));
    writer_->WriteChar(',');
    // 8-byte alignment
    if (!klass->IsObjectType()) {
        writer_->WriteNumber(0);
        writer_->WriteChar(']');
        return;
    }
    u4 size = AlignUp<u4>((klass->GetInstanceSize() + sizeof(TypeInfo*)), alignment);
    // 8 bytes for each field
    writer_->WriteNumber(size);
    writer_->WriteChar(']');
}

/*
 * Record Object Array Info:
 *     u1 tag;             // denoting the type of this sub-record
 *     ID arrObjId;        // array object ID
 *     u4 num;             // number of elements
 *     ID arrClassObjId;   // array class object ID
 *     ID elements[num];   // elements
 *
 */
void ArkHeapDataForIDE::SerializeObjectArray(BaseObject*& obj, const u1 tag)
{
    writer_->WriteChar('[');
    writer_->WriteNumber(tag);
    writer_->WriteChar(',');
    ArkHeapDataID objAddr = (reinterpret_cast<ArkHeapDataID>(obj));
    u4 id = GetId(objAddr);
    writer_->WriteNumber(id);
    writer_->WriteChar(',');
    u4 num = 0;
    std::stack<u4> VAL;
    RefFieldVisitor visitor = [&VAL, &num, this](RefField<>& arrayContent) {
        VAL.push(GetId(reinterpret_cast<ArkHeapDataID>(arrayContent.GetTargetObject())));
        num++;
    };
    // take array length and content.
    MArray* mArray = reinterpret_cast<MArray*>(obj);
    MIndex arrayLengthVal = mArray->GetLength();
    RefField<>* arrayContent = reinterpret_cast<RefField<>*>(mArray->ConvertToCArray());
    // for each object in array.
    for (MIndex i = 0; i < arrayLengthVal; ++i) {
        visitor(arrayContent[i]);
    }
    writer_->WriteNumber(num);
    writer_->WriteChar(',');
    writer_->WriteNumber(GetId(reinterpret_cast<ArkHeapDataID>(obj->GetTypeInfo())));
    writer_->WriteChar(',');
    writer_->WriteChar('[');
    bool isFirstElement = true;
    while (!VAL.empty()) {
        if (!isFirstElement) {
            writer_->WriteChar(',');
        } else {
            isFirstElement = false;
        }
        u4 val = VAL.top();
        VAL.pop();
        writer_->WriteNumber(val);
    }
    writer_->WriteChar(']');
    writer_->WriteChar(']');
}

/*
 * Record struct Array Info:
 *     u1 tag;             // denoting the type of this sub-record
 *     ID arrObjId;        // array object ID
 *     u4 componentNum;        // component Num
 *     u4 num;             // number of ref fields
 *     ID arrClassObjId;   // array class object ID
 *     ID elements[num];   // elements
 *
 */

void ArkHeapDataForIDE::SerializeStructArray(BaseObject*& obj, const u1 tag)
{
    writer_->WriteChar('[');
    writer_->WriteNumber(tag);
    writer_->WriteChar(',');
    ArkHeapDataID objAddr = (reinterpret_cast<ArkHeapDataID>(obj));
    u4 id = GetId(objAddr);
    writer_->WriteNumber(id);
    writer_->WriteChar(',');
    u4 num = 0;
    std::stack<u4> VAL;
    RefFieldVisitor visitor = [&VAL, &num, this](RefField<>& arrayContent) {
        VAL.push(GetId(reinterpret_cast<ArkHeapDataID>(arrayContent.GetTargetObject())));
        num++;
    };
    // take array length and content.
    MArray* mArray = reinterpret_cast<MArray*>(obj);
    MIndex arrayLengthVal = mArray->GetLength();
    TypeInfo* componentTypeInfo = mArray->GetComponentTypeInfo();
    GCTib gcTib = componentTypeInfo->GetGCTib();
    HeapAddress contentAddr = reinterpret_cast<uintptr_t>(mArray) + MArray::GetContentOffset();
    for (MIndex i = 0; i < arrayLengthVal; ++i) {
        gcTib.ForEachBitmapWord(contentAddr, visitor);
        contentAddr += mArray->GetElementSize();
    }
    writer_->WriteNumber(arrayLengthVal);
    writer_->WriteChar(',');
    writer_->WriteNumber(num);
    writer_->WriteChar(',');
    writer_->WriteNumber(GetId(reinterpret_cast<ArkHeapDataID>(obj->GetTypeInfo())));
    writer_->WriteChar(',');
    writer_->WriteChar('[');
    bool isFirstElement = true;
    while (!VAL.empty()) {
        if (!isFirstElement) {
            writer_->WriteChar(',');
        } else {
            isFirstElement = false;
        }
        u4 val = VAL.top();
        VAL.pop();
        writer_->WriteNumber(val);
    }
    writer_->WriteChar(']');
    writer_->WriteChar(']');
}

/*
 * Record Primitive Array Info:
 *     u1 tag;          // denoting the type of this sub-record
 *     ID arrObjId;     // array object
 *     u4 num;          // number of elements
 *     u1 type;         // element type
 */

void ArkHeapDataForIDE::SerializePrimitiveArray(BaseObject*& obj, const u1 tag)
{
    writer_->WriteChar('[');
    writer_->WriteNumber(tag);
    writer_->WriteChar(',');
    ArkHeapDataID objAddr = (reinterpret_cast<ArkHeapDataID>(obj));
    u4 id = GetId(objAddr);
    writer_->WriteNumber(id);
    writer_->WriteChar(',');
    MArray* mArray = reinterpret_cast<MArray*>(obj);
    writer_->WriteNumber(mArray->GetLength());
    writer_->WriteChar(',');
    uint32_t ComponentSize = obj->GetTypeInfo()->GetComponentSize();
    switch (ComponentSize) {
        // bool:1 bytes
        case 1:
            writer_->WriteNumber(BOOLEAN);
            break;
        // short:2 bytes
        case 2:
            writer_->WriteNumber(SHORT);
            break;
        // int:4 bytes
        case 4:
            writer_->WriteNumber(INT);
            break;
        // long:8 bytes
        case 8:
            writer_->WriteNumber(LONG);
            break;
        default:
            break;
    }
    writer_->WriteChar(']');
}

/*
 * Record Struct Class Info:
 *     u1 tag;          // denoting the type of this sub-record
 *     ID classObjId;   // class object ID
 *     u4 size;         // instance size (in bytes)
 */
void ArkHeapDataForIDE::SerializeStructClass(TypeInfo* klass, ArkHeapDataStringId klassId, const u1 tag)
{
    writer_->WriteChar('[');
    writer_->WriteNumber(tag);
    writer_->WriteChar(',');
    writer_->WriteNumber(GetId(reinterpret_cast<ArkHeapDataStringId>(klass)));
    TypeInfo* componentKlass = klass->GetComponentTypeInfo();
    // No alignment required
    u4 size = componentKlass->GetInstanceSize();
    writer_->WriteChar(',');
    writer_->WriteNumber(size);
    writer_->WriteChar(']');
}

/*
 * Record Instance Info:
 *     u1 tag;           // denoting the type of this sub-record
 *     ID objId;         // object ID
 *     ID classObjId;    // class object ID
 *     u4 num;           // number of ref fields
 *     VAL entry[];      // ref contents in instance field values (this class, followed by super class, etc)
 */
void ArkHeapDataForIDE::SerializeInstance(BaseObject*& obj, const u1 tag)
{
    writer_->WriteChar('[');
    writer_->WriteNumber(tag);
    writer_->WriteChar(',');
    u4 id = GetId(reinterpret_cast<ArkHeapDataID>(obj));
    writer_->WriteNumber(id);
    writer_->WriteChar(',');
    writer_->WriteNumber(GetId(reinterpret_cast<ArkHeapDataID>(obj->GetTypeInfo())));
    writer_->WriteChar(',');
    u4 num = 0;
    std::stack<u4> VAL;
    RefFieldVisitor visitor = [&VAL, &num, this](RefField<>& fieldAddr) {
        VAL.push(GetId(reinterpret_cast<ArkHeapDataID>(fieldAddr.GetTargetObject())));
        num++;
    };
    TypeInfo* currentClass = obj->GetTypeInfo();
    if (obj->HasRefField()) {
        GCTib gcTib = currentClass->GetGCTib();
        HeapAddress objAddr = reinterpret_cast<HeapAddress>(obj) + sizeof(TypeInfo*);
        gcTib.ForEachBitmapWord(objAddr, visitor);
    }
    writer_->WriteNumber(num);
    writer_->WriteChar(',');
    writer_->WriteChar('[');
    bool isFirstElement = true;
    while (!VAL.empty()) {
        if (!isFirstElement) {
            writer_->WriteChar(',');
        } else {
            isFirstElement = false;
        }
        u4 val = VAL.top();
        VAL.pop();
        writer_->WriteNumber(val);
    }
    writer_->WriteChar(']');
    writer_->WriteChar(']');
}

ArkHeapData::u4 ArkHeapDataForIDE::GetId(ArkHeapDataStringId klassId)
{
    if (stringIdxMap_.find(klassId) == stringIdxMap_.end()) {
        stringIdxMap_[klassId] = ++stringIdx;
    }
    return stringIdxMap_[klassId];
}

}
