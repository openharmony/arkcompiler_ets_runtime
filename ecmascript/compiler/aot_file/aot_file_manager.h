/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_COMPILER_AOT_FILE_AOT_FILE_MANAGER_H
#define ECMASCRIPT_COMPILER_AOT_FILE_AOT_FILE_MANAGER_H

#include <string>
#include <utility>

#include "ecmascript/base/file_header.h"
#include "ecmascript/compiler/aot_file/an_file_info.h"
#include "ecmascript/compiler/aot_file/aot_file_info.h"
#include "ecmascript/compiler/aot_file/binary_buffer_parser.h"
#include "ecmascript/compiler/aot_file/module_section_des.h"
#include "ecmascript/compiler/aot_file/stub_file_info.h"
#include "ecmascript/compiler/binary_section.h"
#include "ecmascript/deoptimizer/calleeReg.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/platform/map.h"
#include "ecmascript/stackmap/ark_stackmap.h"

namespace panda::ecmascript {
class JSpandafile;
class JSThread;

/*                  AOTLiteralInfo
 *      +--------------------------------+----
 *      |             cache              |  ^
 *      |              ...               |  |
 *      |   -1 (No AOT Function Entry)   |  |
 *      |     AOT Function Entry Index   |  |
 *      |     AOT Function Entry Index   |  |
 *      +--------------------------------+----
 *      |      AOT Instance Hclass (IHC) |
 *      |   AOT Constructor Hclass (CHC) |
 *      +--------------------------------+
 */
class AOTLiteralInfo : public TaggedArray {
public:
    static AOTLiteralInfo *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<AOTLiteralInfo *>(object);
    }

    static size_t ComputeSize(uint32_t cacheSize)
    {
        return TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), cacheSize + RESERVED_LENGTH);
    }

    inline void InitializeWithSpecialValue(JSTaggedValue initValue, uint32_t capacity, uint32_t extraLength = 0)
    {
        TaggedArray::InitializeWithSpecialValue(initValue, capacity + RESERVED_LENGTH, extraLength);
        SetIhc(JSTaggedValue::Undefined());
        SetChc(JSTaggedValue::Undefined());
    }

    inline uint32_t GetCacheLength() const
    {
        return GetLength() - RESERVED_LENGTH;
    }

    inline void SetIhc(JSTaggedValue value)
    {
        Barriers::SetPrimitive(GetData(), GetIhcOffset(), value.GetRawData());
    }

    inline JSTaggedValue GetIhc() const
    {
        return JSTaggedValue(Barriers::GetValue<JSTaggedType>(GetData(), GetIhcOffset()));
    }

    inline void SetChc(JSTaggedValue value)
    {
        Barriers::SetPrimitive(GetData(), GetChcOffset(), value.GetRawData());
    }

    inline JSTaggedValue GetChc() const
    {
        return JSTaggedValue(Barriers::GetValue<JSTaggedType>(GetData(), GetChcOffset()));
    }

    inline void SetObjectToCache(JSThread *thread, uint32_t index, JSTaggedValue value)
    {
        Set(thread, index, value);
    }

    inline JSTaggedValue GetObjectFromCache(uint32_t index) const
    {
        return Get(index);
    }
private:
    static constexpr size_t AOT_CHC_INDEX = 1;
    static constexpr size_t AOT_IHC_INDEX = 2;
    static constexpr size_t RESERVED_LENGTH = AOT_IHC_INDEX;

    inline size_t GetIhcOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * (GetLength() - AOT_IHC_INDEX);
    }

    inline size_t GetChcOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * (GetLength() - AOT_CHC_INDEX);
    }
};

class AOTFileManager {
public:
    explicit AOTFileManager(EcmaVM *vm);
    virtual ~AOTFileManager();

    static constexpr char FILE_EXTENSION_AN[] = ".an";
    static constexpr char FILE_EXTENSION_AI[] = ".ai";
    static constexpr uint8_t DESERI_CP_ITEM_SIZE = 2;

    void LoadStubFile(const std::string &fileName);
    static bool LoadAnFile(const std::string &fileName);
    static AOTFileInfo::CallSiteInfo CalCallSiteInfo(uintptr_t retAddr);
    static bool TryReadLock();
    static bool InsideStub(uintptr_t pc);
    static bool InsideAOT(uintptr_t pc);
    void Iterate(const RootVisitor &v);

    const std::shared_ptr<AnFileInfo> GetAnFileInfo(const JSPandaFile *jsPandaFile) const;
    bool IsLoad(const JSPandaFile *jsPandaFile) const;
    bool IsLoadMain(const JSPandaFile *jsPandaFile, const CString &entry) const;
    uint32_t GetAnFileIndex(const JSPandaFile *jsPandaFile) const;
    void SetAOTMainFuncEntry(JSHandle<JSFunction> mainFunc, const JSPandaFile *jsPandaFile,
                             std::string_view entryPoint);
    void SetAOTFuncEntry(const JSPandaFile *jsPandaFile, Method *method,
                         uint32_t entryIndex, bool *canFastCall = nullptr);
    bool LoadAiFile([[maybe_unused]] const std::string &filename);
    bool LoadAiFile(const JSPandaFile *jsPandaFile);
    kungfu::ArkStackMapParser* GetStackMapParser() const;
    static JSTaggedValue GetAbsolutePath(JSThread *thread, JSTaggedValue relativePathVal);
    static bool GetAbsolutePath(const CString &relativePathCstr, CString &absPathCstr);
    static bool RewriteDataSection(uintptr_t dataSec, size_t size, uintptr_t newData, size_t newSize);
    void AddConstantPool(const CString &snapshotFileName, JSTaggedValue deserializedCPList);
    JSHandle<JSTaggedValue> GetDeserializedConstantPool(const JSPandaFile *jsPandaFile, int32_t cpID);
    const Heap *GetHeap();

    static void DumpAOTInfo() DUMP_API_ATTR;

private:
    static void PrintAOTEntry(const JSPandaFile *file, const Method *method, uintptr_t entry);
    void InitializeStubEntries(const std::vector<AnFileInfo::FuncEntryDes>& stubs);
    static void AdjustBCStubAndDebuggerStubEntries(JSThread *thread,
                                                   const std::vector<AOTFileInfo::FuncEntryDes> &stubs,
                                                   const AsmInterParsedOption &asmInterOpt);
    EcmaVM *vm_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    std::unordered_map<uint32_t, CMap<int32_t, JSTaggedValue>> desCPs_ {};
    kungfu::ArkStackMapParser *arkStackMapParser_ {nullptr};

    friend class AnFileInfo;
    friend class StubFileInfo;
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_COMPILER_AOT_FILE_AOT_FILE_MANAGER_H
