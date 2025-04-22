/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/snapshot/mem/snapshot.h"


#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"

namespace panda::ecmascript {
void Snapshot::Serialize(const CString &fileName)
{
    kungfu::AOTSnapshot &aotSnapshot = vm_->GetJSThread()->GetCurrentEcmaContext()->GetPTManager()->GetAOTSnapshot();
    JSTaggedValue root = aotSnapshot.GetSnapshotData();
    if (root == JSTaggedValue::Hole()) {
        // root equals hole means no data stored.
        LOG_COMPILER(ERROR) << "error: no data for ai file generation!";
        return;
    }
    Serialize(root.GetTaggedObject(), nullptr, fileName);
}

void Snapshot::Serialize(TaggedObject *objectHeader, const JSPandaFile *jsPandaFile, const CString &fileName)
{
    std::string realPath;
    if (!RealPath(std::string(fileName), realPath, false)) {
        LOG_FULL(FATAL) << "snapshot file path error";
    }
    std::fstream writer(realPath.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!writer.good()) {
        writer.close();
        LOG_FULL(ERROR) << "snapshot open file failed";
        return;
    }

    SnapshotProcessor processor(vm_);
    processor.Initialize();

    std::unordered_map<uint64_t, std::pair<uint64_t, EncodeBit>> data;
    CQueue<TaggedObject *> objectQueue;

    if (objectHeader->GetClass()->GetObjectType() == JSType::PROGRAM) {
        processor.SetProgramSerializeStart();
    }

    processor.EncodeTaggedObject(objectHeader, &objectQueue, &data);
    size_t rootObjSize = objectQueue.size();
    processor.ProcessObjectQueue(&objectQueue, &data);
    WriteToFile(writer, jsPandaFile, rootObjSize, processor);
}

void Snapshot::Serialize(uintptr_t startAddr, size_t size, const CString &fileName)
{
    std::string realPath;
    if (!RealPath(std::string(fileName), realPath, false)) {
        LOG_FULL(FATAL) << "snapshot file path error";
    }
    std::fstream writer(realPath.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!writer.good()) {
        writer.close();
        LOG_FULL(ERROR) << "snapshot open file failed";
        return;
    }

    SnapshotProcessor processor(vm_);
    processor.Initialize();

    std::unordered_map<uint64_t, std::pair<uint64_t, EncodeBit>> data;
    CQueue<TaggedObject *> objectQueue;

    ObjectSlot start(startAddr);
    ObjectSlot end(startAddr + size * sizeof(JSTaggedType));
    processor.EncodeTaggedObjectRange(start, end, &objectQueue, &data);

    size_t rootObjSize = objectQueue.size();
    processor.ProcessObjectQueue(&objectQueue, &data);
    WriteToFile(writer, nullptr, rootObjSize, processor);
}

void Snapshot::SerializeBuiltins(const CString &fileName)
{
    std::string realPath;
    if (!RealPath(std::string(fileName), realPath, false)) {
        LOG_FULL(FATAL) << "snapshot file path error";
    }
    std::fstream write(realPath.c_str(), std::ios::out | std::ios::binary | std::ios::app);
    if (!write.good()) {
        write.close();
        LOG_FULL(ERROR) << "snapshot open file failed";
        return;
    }
    // if builtins.snapshot file has exist, return directly
    if (write.tellg()) {
        LOG_FULL(DEBUG) << "snapshot already exist";
        write.close();
        return;
    }

    SnapshotProcessor processor(vm_);
    processor.Initialize();
    processor.SetBuiltinsSerializeStart();

    std::unordered_map<uint64_t, std::pair<uint64_t, EncodeBit>> data;
    CQueue<TaggedObject *> objectQueue;

    class SerializeBuiltinsRootVisitor final : public RootVisitor {
    public:
        explicit SerializeBuiltinsRootVisitor(SnapshotProcessor *processor,
            std::unordered_map<uint64_t, std::pair<uint64_t, EncodeBit>> *data, CQueue<TaggedObject *> *objectQueue)
            : processor_(processor), data_(data), objectQueue_(objectQueue) {}
        ~SerializeBuiltinsRootVisitor() = default;

        void VisitRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot slot) override {}
        void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override
        {
            processor_->EncodeTaggedObjectRange(start, end, objectQueue_, data_);
        }
        void VisitBaseAndDerivedRoot([[maybe_unused]] Root type,  [[maybe_unused]] ObjectSlot base,
            [[maybe_unused]] ObjectSlot derived, [[maybe_unused]] uintptr_t baseOldObject) override {}
    private:
        SnapshotProcessor *processor_;
        std::unordered_map<uint64_t, std::pair<uint64_t, EncodeBit>> *data_;
        CQueue<TaggedObject *> *objectQueue_;
    };
    SerializeBuiltinsRootVisitor serializeBuiltinsRootVisitor(&processor, &data, &objectQueue);

    auto globalEnvHandle = vm_->GetGlobalEnv();
    auto constant = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
    constant->Iterate(serializeBuiltinsRootVisitor);
    processor.EncodeTaggedObject(*globalEnvHandle, &objectQueue, &data);
    size_t rootObjSize = objectQueue.size();
    processor.ProcessObjectQueue(&objectQueue, &data);
    WriteToFile(write, nullptr, rootObjSize, processor);
}

bool Snapshot::DeserializeInternal(SnapshotType type, const CString &snapshotFile, SnapshotProcessor &processor,
                                   MemMap &fileMap)
{
    if (fileMap.GetOriginAddr() == nullptr) {
        LOG_FULL(FATAL) << "file mmap failed";
        UNREACHABLE();
    }
    auto readFile = ToUintPtr(fileMap.GetOriginAddr());
    auto hdr = *ToNativePtr<const SnapShotHeader>(readFile);
    if (!hdr.Verify(GetLastVersion())) {
        FileUnMap(fileMap);
        LOG_ECMA(ERROR) << "file verify failed.";
        return false;
    }
    uintptr_t oldSpaceBegin = readFile + sizeof(SnapShotHeader);
    uintptr_t stringBegin = oldSpaceBegin + hdr.oldSpaceObjSize + hdr.nonMovableObjSize +
        hdr.machineCodeObjSize + hdr.snapshotObjSize + hdr.hugeObjSize;
    uintptr_t stringEnd = stringBegin + hdr.stringSize;
    [[maybe_unused]] EcmaHandleScope stringHandleScope(vm_->GetJSThread());
    processor.DeserializeString(stringBegin, stringEnd);

    processor.DeserializeObjectExcludeString(oldSpaceBegin, hdr.oldSpaceObjSize, hdr.nonMovableObjSize,
                                             hdr.machineCodeObjSize, hdr.snapshotObjSize, hdr.hugeObjSize);

#if !defined(CROSS_PLATFORM)
    FileUnMap(MemMap(fileMap.GetOriginAddr(), hdr.pandaFileBegin));
#endif
    std::shared_ptr<JSPandaFile> jsPandaFile;
    if (static_cast<uint32_t>(fileMap.GetSize()) > hdr.pandaFileBegin) {
        uintptr_t pandaFileMem = readFile + hdr.pandaFileBegin;
        auto pf = panda_file::File::OpenFromMemory(os::mem::ConstBytePtr(ToNativePtr<std::byte>(pandaFileMem),
            static_cast<uint32_t>(fileMap.GetSize()) - hdr.pandaFileBegin, os::mem::MmapDeleter));
        jsPandaFile = JSPandaFileManager::GetInstance()->NewJSPandaFile(pf.release(), "");
    }
    // relocate object field
    processor.Relocate(type, jsPandaFile.get(), hdr.rootObjectSize);
    processor.AddRootObjectToAOTFileManager(type, snapshotFile);
    LOG_COMPILER(INFO) << "loaded ai file: " << snapshotFile.c_str();
    return true;
}

bool Snapshot::Deserialize(SnapshotType type, const CString &snapshotFile, bool isBuiltins)
{
    std::string realPath;
    if (!RealPath(std::string(snapshotFile), realPath, false)) {
        LOG_FULL(FATAL) << "snapshot file path error";
        UNREACHABLE();
    }

    std::ifstream file(realPath);
    if (!file.good()) {
        return false;
    }
    file.close();

    SnapshotProcessor processor(vm_);
    if (isBuiltins) {
        processor.SetBuiltinsDeserializeStart();
    }

    MemMap fileMap = FileMap(realPath.c_str(), FILE_RDONLY, PAGE_PROT_READWRITE);
    return DeserializeInternal(type, snapshotFile, processor, fileMap);
}

#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
bool Snapshot::Deserialize(SnapshotType type, const CString &snapshotFile, [[maybe_unused]] std::function<bool
    (std::string fileName, uint8_t **buff, size_t *buffSize)> ReadAOTCallBack, bool isBuiltins)
{
    SnapshotProcessor processor(vm_);
    if (isBuiltins) {
        processor.SetBuiltinsDeserializeStart();
    }

    std::string fileName = std::string(snapshotFile);
    uint8_t *buff = nullptr;
    size_t buffSize = 0;
    MemMap fileMap = {};
    size_t found = fileName.find_last_of("/");
    if (found != std::string::npos) {
        fileName = fileName.substr(found + 1);
    }
    
    LOG_ECMA(INFO) << "Call JsAotReader to load: " << fileName;
    if (ReadAOTCallBack(fileName, &buff, &buffSize)) {
        fileMap = MemMap(buff, buffSize);
    }

    return DeserializeInternal(type, snapshotFile, processor, fileMap);
}
#endif

size_t Snapshot::AlignUpPageSize(size_t spaceSize)
{
    if (spaceSize % Constants::PAGE_SIZE_ALIGN_UP == 0) {
        return spaceSize;
    }
    return Constants::PAGE_SIZE_ALIGN_UP * (spaceSize / Constants::PAGE_SIZE_ALIGN_UP + 1);
}

void Snapshot::WriteToFile(std::fstream &writer, const JSPandaFile *jsPandaFile,
                           size_t size, SnapshotProcessor &processor)
{
    uint32_t totalStringSize = 0U;
    CVector<uintptr_t> stringVector = processor.GetStringVector();
    for (size_t i = 0; i < stringVector.size(); ++i) {
        auto str = reinterpret_cast<EcmaString *>(stringVector[i]);
        size_t objectSize = AlignUp(EcmaStringAccessor(str).ObjectSize(),
            static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
        totalStringSize += objectSize;
    }

    std::vector<uint32_t> objSizeVector = processor.StatisticsObjectSize();
    size_t totalObjSize = totalStringSize;
    for (uint32_t objSize : objSizeVector) {
        totalObjSize += objSize;
    }
    uint32_t pandaFileBegin = RoundUp(totalObjSize + sizeof(SnapShotHeader), Constants::PAGE_SIZE_ALIGN_UP);
    SnapShotHeader hdr(GetLastVersion());
    hdr.oldSpaceObjSize = objSizeVector[0]; // 0: oldSpaceObj
    hdr.nonMovableObjSize = objSizeVector[1]; // 1: nonMovableObj
    hdr.machineCodeObjSize = objSizeVector[2]; // 2: machineCodeObj
    hdr.snapshotObjSize = objSizeVector[3]; // 3: snapshotObj
    hdr.hugeObjSize = objSizeVector[4]; // 4: hugeObj
    hdr.stringSize = totalStringSize;
    hdr.pandaFileBegin = pandaFileBegin;
    hdr.rootObjectSize = static_cast<uint32_t>(size);
    writer.write(reinterpret_cast<char *>(&hdr), sizeof(hdr));
    processor.WriteObjectToFile(writer);

    for (size_t i = 0; i < stringVector.size(); ++i) {
        auto str = reinterpret_cast<EcmaString *>(stringVector[i]);
        size_t strSize = AlignUp(EcmaStringAccessor(str).ObjectSize(),
            static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
        int index = 0; // 0 represents the line string.
        // Write the index in the head of string.
        uint8_t headerSize = JSTaggedValue::TaggedTypeSize();
        JSTaggedType indexHeader = JSTaggedValue(index).GetRawData();
        writer.write(reinterpret_cast<char *>(&indexHeader), headerSize);
        writer.write(reinterpret_cast<char *>(str) + headerSize, strSize - headerSize);
        writer.flush();
    }

    ASSERT(static_cast<size_t>(writer.tellp()) == totalObjSize + sizeof(SnapShotHeader));
    if (jsPandaFile) {
        writer.seekp(pandaFileBegin);
        writer.write(static_cast<const char *>(jsPandaFile->GetHeader()), jsPandaFile->GetFileSize());
    }
    writer.close();
}
}  // namespace panda::ecmascript
