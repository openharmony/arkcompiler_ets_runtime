/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
#include "ecmascript/compiler/aot_file/an_file_data_manager.h"
#include "ecmascript/compiler/aot_file/an_file_info.h"
#include "ecmascript/compiler/aot_file/aot_file_info.h"
#include "ecmascript/compiler/aot_file/binary_buffer_parser.h"
#include "ecmascript/compiler/aot_file/module_section_des.h"
#include "ecmascript/compiler/aot_file/stub_file_info.h"
#include "ecmascript/compiler/aot_snapshot/snapshot_constantpool_data.h"
#include "ecmascript/compiler/binary_section.h"
#include "ecmascript/deoptimizer/calleeReg.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/platform/map.h"
#include "ecmascript/stackmap/ark_stackmap.h"

#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
#include "ecmascript/ecma_context.h"
#endif

namespace panda::ecmascript {
class JSpandafile;
class JSThread;
namespace kungfu {
    class ArkStackMapParser;
} // namespace kungfu

/*                  AOTLiteralInfo
 *      +-----------------------------------+----
 *      |              ...                  |  ^
 *      |              ...                  |  |
 *      | cache(store function entry index) |  AOT Function Entry Index, if its value is -1,
 *      |              ...                  |  means the function has not been compiled with AOT.
 *      |              ...                  |  v
 *      +-----------------------------------+----
 *      |         Literal Type              |  JSTaggedValue(int32_t)
 *      +-----------------------------------+----
 *      |      AOT Instance Hclass (IHC)    |  JSTaggedValue(HClass)
 *      +-----------------------------------+----
 *      |   AOT Constructor Hclass (CHC)    |  JSTaggedValue(HClass)
 *      +-----------------------------------+----
 */
class AOTLiteralInfo : public TaggedArray {
public:
    static constexpr size_t NO_FUNC_ENTRY_VALUE = -1;
    static constexpr size_t AOT_CHC_INDEX = 1;
    static constexpr size_t AOT_IHC_INDEX = 2;
    static constexpr size_t LITERAL_TYPE_INDEX = 3;
    static constexpr size_t RESERVED_LENGTH = LITERAL_TYPE_INDEX;

    static constexpr int32_t METHOD_LITERAL_TYPE = 1;
    static constexpr int32_t INVALID_LITERAL_TYPE = 0;

    static AOTLiteralInfo* Cast(TaggedObject* object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<AOTLiteralInfo*>(object);
    }

    static size_t ComputeSize(uint32_t cacheSize)
    {
        return TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), cacheSize + RESERVED_LENGTH);
    }

    void InitializeWithSpecialValue(JSTaggedValue initValue, uint32_t capacity,
                                    uint32_t extraLength = 0);

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

    inline void SetLiteralType(JSTaggedValue value)
    {
        Barriers::SetPrimitive(GetData(), GetLiteralTypeOffset(), value.GetRawData());
    }

    inline int GetLiteralType() const
    {
        return JSTaggedValue(Barriers::GetValue<JSTaggedType>(GetData(), GetLiteralTypeOffset())).GetInt();
    }

    void SetObjectToCache(JSThread *thread, uint32_t index, JSTaggedValue value);

    inline JSTaggedValue GetObjectFromCache(uint32_t index) const
    {
        return Get(index);
    }

private:
    inline size_t GetIhcOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * (GetLength() - AOT_IHC_INDEX);
    }

    inline size_t GetChcOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * (GetLength() - AOT_CHC_INDEX);
    }

    inline size_t GetLiteralTypeOffset() const
    {
        return JSTaggedValue::TaggedTypeSize() * (GetLength() - LITERAL_TYPE_INDEX);
    }
};

class AOTFileManager {
public:
    explicit AOTFileManager(EcmaVM* vm);
    virtual ~AOTFileManager();

    static constexpr char FILE_EXTENSION_AN[] = ".an";
    static constexpr char FILE_EXTENSION_AI[] = ".ai";
    static constexpr uint32_t STUB_FILE_INDEX = 1;

#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
    static void SetJsAotReader(JsAotReaderCallback cb);
    static JsAotReaderCallback GetJsAotReader();
#endif
    void LoadStubFile(const std::string& fileName);
    static bool LoadAnFile(const std::string& fileName);
    static AOTFileInfo::CallSiteInfo CalCallSiteInfo(uintptr_t retAddr, bool isDeopt);
    static bool TryReadLock();
    static bool InsideStub(uintptr_t pc);
    static bool InsideAOT(uintptr_t pc);
    static bool AOTFileExist(const std::string &aotFileName, const std::string &extension);
    bool IsEnableAOT() const;
    void Iterate(const RootVisitor& v);

    const std::shared_ptr<AnFileInfo> GetAnFileInfo(const JSPandaFile* jsPandaFile) const;
    bool IsLoadMain(const JSPandaFile* jsPandaFile, const CString& entry) const;
    uint32_t GetFileIndex(uint32_t anFileInfoIndex, CString abcNormalizedName) const;
    std::list<CString> GetPandaFiles(uint32_t aotFileInfoIndex);
    uint32_t GetAnFileIndex(const JSPandaFile* jsPandaFile) const;
    void BindPreloadedPandaFilesInAotFile(const std::string& moduleName);
    bool HasPandaFile(uint32_t aotFileInfoIndex, const CString& abcNormalizedName) const;
    void BindPandaFileInAotFile(const std::string& aotFileBaseName, JSPandaFile* jsPandaFile) const;
    void SetAOTMainFuncEntry(JSHandle<JSFunction> mainFunc,
                             const JSPandaFile* jsPandaFile,
                             std::string_view entryPoint);
    void SetAOTFuncEntry(const JSPandaFile* jsPandaFile,
                         JSFunction* function,
                         Method* method,
                         uint32_t entryIndex = 0,
                         bool* canFastCall = nullptr);
    bool LoadAiFile([[maybe_unused]] const std::string& filename);
    bool LoadAiFile(const JSPandaFile* jsPandaFile);
    kungfu::ArkStackMapParser* GetStackMapParser() const;
    static JSTaggedValue GetAbsolutePath(JSThread* thread, JSTaggedValue relativePathVal);
    static bool GetAbsolutePath(const CString& relativePathCstr, CString& absPathCstr);
    static bool RewriteDataSection(uintptr_t dataSec, size_t size, uintptr_t newData, size_t newSize);
    void ParseDeserializedData(const CString& snapshotFileName, JSTaggedValue deserializedData);
    JSHandle<JSTaggedValue> GetDeserializedConstantPool(const JSPandaFile* jsPandaFile, int32_t cpID);
    const Heap* GetHeap();

    static void DumpAOTInfo() DUMP_API_ATTR;

private:
    using MultiConstantPoolMap = CMap<int32_t, JSTaggedValue>; // key: constpool id, value: constantpool

    struct PandaCpInfo {
        uint32_t fileIndex_;
        MultiConstantPoolMap multiCpsMap_;
    };
    using FileNameToMultiConstantPoolMap = CMap<CString, PandaCpInfo>;
    using AIDatum = CUnorderedMap<uint32_t, FileNameToMultiConstantPoolMap>; // key: ai file index

    static void PrintAOTEntry(const JSPandaFile *file, const Method *method, uintptr_t entry);
    void InitializeStubEntries(const std::vector<AnFileInfo::FuncEntryDes>& stubs);
    static void AdjustBCStubAndDebuggerStubEntries(JSThread *thread,
                                                   const std::vector<AOTFileInfo::FuncEntryDes> &stubs,
                                                   const AsmInterParsedOption &asmInterOpt);
    EcmaVM *vm_ {nullptr};
    ObjectFactory *factory_ {nullptr};
    AIDatum aiDatum_ {};
    kungfu::ArkStackMapParser *arkStackMapParser_ {nullptr};
#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
    static JsAotReaderCallback jsAotReader_;
#endif

    friend class AnFileInfo;
    friend class StubFileInfo;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_COMPILER_AOT_FILE_AOT_FILE_MANAGER_H
