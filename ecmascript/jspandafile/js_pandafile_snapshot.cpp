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

#include "ecmascript/jspandafile/js_pandafile_snapshot.h"

#include "common_components/taskpool/taskpool.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/platform/filesystem.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/module/module_path_helper.h"
#include "zlib.h"

namespace panda::ecmascript {
bool JSPandaFileSnapshot::ReadData(JSThread *thread, JSPandaFile *jsPandaFile, const CString &path,
    const CString &version)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "JSPandaFile::ReadData", "");
    LOG_ECMA(INFO) << "JSPandaFileSnapshot::ReadData";
    // check application white list & specific file
    if (filesystem::Exists(path.c_str()) && IsJSPandaFileSnapshotFileExist(jsPandaFile->GetJSPandaFileDesc(), path)) {
        return ReadDataFromFile(thread, jsPandaFile, path, version);
    }
    return false;
}

void JSPandaFileSnapshot::PostWriteDataToFileJob(const EcmaVM *vm, const CString &path, const CString &version)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "JSPandaFileSnapshot::PostWriteDataToFileJob", "");
    LOG_ECMA(INFO) << "JSPandaFileSnapshot::PostWriteDataToFileJob";
    std::unordered_set<std::shared_ptr<JSPandaFile>> jspandaFiles =
        JSPandaFileManager::GetInstance()->GetHapJSPandaFiles();
    JSThread *thread = vm->GetJSThread();
    int32_t tid = thread->GetThreadId();
    for (const auto &item : jspandaFiles) {
        common::Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<JSPandaFileSnapshotTask>(tid, thread, item.get(), path, version));
    }
}

bool JSPandaFileSnapshot::JSPandaFileSnapshotTask::Run(uint32_t threadIndex)
{
    WriteDataToFile(thread_, jsPandaFile_, path_, version_);
    return true;
}

bool JSPandaFileSnapshot::IsJSPandaFileSnapshotFileExist(const CString &fileName, const CString &path)
{
    CString serializeFileName = GetJSPandaFileFileName(fileName, path);
    return FileExist(serializeFileName.c_str());
}

CString JSPandaFileSnapshot::GetJSPandaFileFileName(const CString &fileName, const CString &path)
{
    CString moduleName = ModulePathHelper::GetModuleNameWithBaseFile(fileName);
    return base::ConcatToCString(path, moduleName, JSPANDAFILE_FILE_NAME);
}

void JSPandaFileSnapshot::RemoveSnapshotFiles(const CString &path)
{
    DeleteFilesWithSuffix(path.c_str(), SNAPSHOT_FILE_SUFFIX.data());
}

void JSPandaFileSnapshot::WriteDataToFile(JSThread *thread, JSPandaFile *jsPandaFile, const CString &path,
    const CString &version)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "JSPandaFileSnapshot::WriteDataToFile", "");
    CString filename = GetJSPandaFileFileName(jsPandaFile->GetJSPandaFileDesc(), path);
    LOG_ECMA(INFO) << "JSPandaFileSnapshot::WriteDataToFile: " << filename;
    if (FileExist(filename.c_str())) {
        LOG_ECMA(INFO) << "JSPandaFileSnapshot::WriteDataToFile file already exist";
        return;
    }
    // calculate file size
    size_t checksumSize = sizeof(uint32_t);
    size_t fileSize = sizeof(uint32_t);
    size_t appVersionCodeSize = sizeof(uint32_t);
    size_t versionStrLenSize = sizeof(uint32_t);
    size_t versionStrLen = version.size();
    size_t bufSize = appVersionCodeSize + versionStrLenSize + versionStrLen + fileSize + checksumSize;
    // add moduleName len & ptr
    CString moduleName = ModulePathHelper::GetModuleNameWithBaseFile(jsPandaFile->GetJSPandaFileDesc());
    uint32_t moduleNameLen = moduleName.size();
    bufSize += sizeof(uint32_t); // len
    bufSize += moduleNameLen; // ptr
    // add methodLiteral size
    uint32_t numMethods = jsPandaFile->numMethods_;
    bufSize += sizeof(numMethods); // numMethods
    bufSize += sizeof(MethodLiteral) * numMethods; // MethodLiteral Total size
    // calculate mainMethodIndex size
    uint32_t mainMethodIndexSize = 0;
    bufSize += sizeof(mainMethodIndexSize);
    // recordName size
    uint32_t mainMethodIndex = JSPandaFile::DEFAULT_MAIN_METHOD_INDEX;
    for (auto &[recordName, recordInfo]: jsPandaFile->GetJSRecordInfo()) {
        mainMethodIndex = recordInfo->mainMethodIndex;
        if (mainMethodIndex != JSPandaFile::DEFAULT_MAIN_METHOD_INDEX) {
            bufSize += sizeof(uint32_t); // mainMethodIndex
            bufSize += sizeof(uint32_t); // recordName len
            bufSize += recordName.size(); // recordName ptr
            mainMethodIndexSize++;
        }
    }

    MemMap fileMapMem =
        CreateFileMap(filename.c_str(), bufSize, FILE_RDWR | FILE_CREAT | FILE_TRUNC, PAGE_PROT_READWRITE);
    if (fileMapMem.GetOriginAddr() == nullptr) {
        LOG_ECMA(ERROR) << "WriteJSPandafileToFile: open file failed:" << filename;
        return;
    }
    MemMapScope memMapScope(fileMapMem);
    // write versionCode
    char* writeBuf = static_cast<char*>(fileMapMem.GetOriginAddr());
    uint32_t appVersionCode = thread->GetEcmaVM()->GetApplicationVersionCode();
    if (memcpy_s(writeBuf, sizeof(uint32_t), &appVersionCode, sizeof(uint32_t)) != EOK) {
        LOG_ECMA(ERROR) << "WriteJSPandafileToFile: appVersionCode memcpy_s failed";
        return;
    }
    writeBuf += sizeof(uint32_t);
    if (memcpy_s(writeBuf, sizeof(uint32_t), &versionStrLen, sizeof(uint32_t)) != EOK) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot: versionStrLen memcpy_s failed";
        return;
    }
    writeBuf += sizeof(uint32_t);
    if (memcpy_s(writeBuf, versionStrLen, version.c_str(), versionStrLen) != EOK) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot: versionStrLen memcpy_s failed";
        return;
    }
    writeBuf += versionStrLen;
    // write pandafile size
    uint32_t fsize = jsPandaFile->GetFileSize();
    if (memcpy_s(writeBuf, sizeof(fsize), &fsize, sizeof(fsize)) != EOK) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot: memcpy_s fsize failed";
    }
    writeBuf += sizeof(uint32_t);
    // write moduleName
    if (memcpy_s(writeBuf, sizeof(uint32_t), &moduleNameLen, sizeof(uint32_t)) != EOK) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot: memcpy_s moduleName len failed";
    }
    writeBuf += sizeof(uint32_t);
    if (memcpy_s(writeBuf, moduleNameLen, moduleName.c_str(), moduleNameLen) != EOK) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot: memcpy_s moduleName failed";
    }
    writeBuf += moduleNameLen;
    // write numMethods
    if (memcpy_s(writeBuf, sizeof(uint32_t), &numMethods, sizeof(uint32_t)) != EOK) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot: memcpy_s numMethods failed";
    }
    writeBuf += sizeof(uint32_t);
    size_t methodLiteralSize = sizeof(MethodLiteral) * numMethods;
    // write MethodLiterals
    if (memcpy_s(writeBuf, methodLiteralSize, jsPandaFile->GetMethodLiterals(), methodLiteralSize) != EOK) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot: memcpy_s methodLiterals failed";
        return;
    }
    MethodLiteral *methodLiterals = (MethodLiteral *) writeBuf;
    size_t methodIdx = 0;
    // get current pandafile base offset
    uintptr_t baseAddress = reinterpret_cast<uintptr_t>(jsPandaFile->GetBase());
    while (methodIdx < jsPandaFile->numMethods_) {
        MethodLiteral *methodLiteral = methodLiterals + (methodIdx++);
        uintptr_t nativePointerAddress = reinterpret_cast<uintptr_t>(methodLiteral->GetNativePointer());
        // calculate relative offset
        methodLiteral->SetNativePointer(reinterpret_cast<void*>(nativePointerAddress - baseAddress));
    }
    writeBuf += methodLiteralSize;
    // write mainMethodIndexSize
    if (memcpy_s(writeBuf, sizeof(uint32_t), &mainMethodIndexSize, sizeof(uint32_t)) != EOK) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot: memcpy_s mainMethodIndexSize failed";
        return;
    }
    writeBuf += sizeof(uint32_t);
    // write mainMethodIndex & recordName
    mainMethodIndex = JSPandaFile::DEFAULT_MAIN_METHOD_INDEX;
    for (auto &[recordName, recordInfo]: jsPandaFile->GetJSRecordInfo()) {
        mainMethodIndex = recordInfo->mainMethodIndex;
        if (mainMethodIndex != JSPandaFile::DEFAULT_MAIN_METHOD_INDEX) {
            if (memcpy_s(writeBuf, sizeof(uint32_t), &mainMethodIndex, sizeof(uint32_t)) != EOK) {
                LOG_ECMA(ERROR) << "JSPandaFileSnapshot: memcpy_s mainMethodIndex failed";
            }
            writeBuf += sizeof(uint32_t);
            uint32_t strLen = recordName.size();
            if (memcpy_s(writeBuf, sizeof(uint32_t), &strLen, sizeof(uint32_t)) != EOK) {
                LOG_ECMA(ERROR) << "JSPandaFileSnapshot: memcpy_s strLen failed";
            }
            writeBuf += sizeof(uint32_t);
            if (memcpy_s(writeBuf, strLen, recordName.c_str(), strLen) != EOK) {
                LOG_ECMA(ERROR) << "JSPandaFileSnapshot: memcpy_s recordName failed";
            }
            writeBuf += strLen;
        }
    }
    uint32_t contentSize = fileMapMem.GetSize() - checksumSize;
    uint32_t checksum = adler32(0, reinterpret_cast<const Bytef*>(fileMapMem.GetOriginAddr()), contentSize);
    if (memcpy_s(writeBuf, checksumSize, &checksum, checksumSize) != EOK) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot: checksum memcpy_s failed";
        return;
    }
    FileSync(fileMapMem, FILE_MS_SYNC);
}

bool JSPandaFileSnapshot::ReadDataFromFile(JSThread *thread, JSPandaFile *jsPandaFile, const CString &path,
    const CString &version)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "JSPandaFileSnapshot::ReadDataFromFile", "");
    CString filename = GetJSPandaFileFileName(jsPandaFile->GetJSPandaFileDesc(), path);
    LOG_ECMA(INFO) << "JSPandaFileSnapshot::ReadDataFromFile: " << filename;
    MemMap fileMapMem = FileMap(filename.c_str(), FILE_RDONLY, PAGE_PROT_READ, 0);
    if (fileMapMem.GetOriginAddr() == nullptr) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot: open file failed:" << filename;
        return false;
    }
    MemMapScope memMapScope(fileMapMem);

    const char* readBuf = static_cast<const char*>(fileMapMem.GetOriginAddr());
    size_t checksumSize = sizeof(uint32_t);
    uint32_t contentSize = fileMapMem.GetSize() - checksumSize;
    uint32_t checksum = adler32(0, reinterpret_cast<const Bytef*>(fileMapMem.GetOriginAddr()), contentSize);
    uint32_t readCheckSum = *reinterpret_cast<const uint32_t*>(readBuf + contentSize);
    if (checksum != readCheckSum) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot checksum compare failed, checksum: " << checksum
            << ", readCheckSum" << readCheckSum;
        RemoveSnapshotFiles(path);
        return false;
    }
    // verify version code
    uint32_t appVersionCode = thread->GetEcmaVM()->GetApplicationVersionCode();
    uint32_t readAppVersionCode = *reinterpret_cast<const uint32_t*>(readBuf);
    if (appVersionCode != readAppVersionCode) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot version compare failed, appVersionCode: " << appVersionCode
            << ", readAppVersionCode" << readAppVersionCode;
        RemoveSnapshotFiles(path);
        return false;
    }
    readBuf += sizeof(uint32_t);
    uint32_t readVersionStrLen = *reinterpret_cast<const uint32_t*>(readBuf);
    readBuf += sizeof(uint32_t);
    CString readVersion(readBuf, readVersionStrLen);
    if (version != readVersion) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot version compare failed, version: " << version
            << ", readVersion" << readVersion;
        RemoveSnapshotFiles(path);
        return false;
    }
    readBuf += readVersionStrLen;
    // verify filesize
    uint32_t fsize = *reinterpret_cast<const uint32_t*>(readBuf);
    if (fsize != jsPandaFile->GetFileSize()) {
        LOG_COMPILER(ERROR) << "JSPandaFileSnapshot: file size not equal, " << filename  << ", old = "
            << fsize << ", new = " << jsPandaFile->GetFileSize();
        RemoveSnapshotFiles(path);
        return false;
    }
    readBuf += sizeof(uint32_t);
    // verify moduleName
    // read moduleName len
    uint32_t moduleNameLen = *reinterpret_cast<const uint32_t*>(readBuf);
    readBuf += sizeof(uint32_t);
    // Get moduleName
    CString str(readBuf, moduleNameLen);
    CString curModuleName = ModulePathHelper::GetModuleNameWithBaseFile(jsPandaFile->GetJSPandaFileDesc());
    if (str != curModuleName) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot moduleName check failed, read moduleName is: " << str
            << ", current moduleName is:" << curModuleName;
        RemoveSnapshotFiles(path);
        return false;
    }
    readBuf += moduleNameLen;
    // read numMethods
    uint32_t numMethods = *reinterpret_cast<const uint32_t*>(readBuf);
    jsPandaFile->numMethods_ = numMethods;
    readBuf += sizeof(uint32_t);
    // read MethodLiterals
    MethodLiteral *methodLiterals = jsPandaFile->GetMethodLiterals();
    size_t methodLiteralSize = numMethods * sizeof(MethodLiteral);
    if (memcpy_s(methodLiterals, methodLiteralSize, readBuf, methodLiteralSize) != EOK) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot: memcpy_s failed";
        return false;
    }

    size_t methodIdx = 0;
    // get current pandafile base offset
    uintptr_t baseAddress = reinterpret_cast<uintptr_t>(jsPandaFile->GetBase());
    while (methodIdx < numMethods) {
        MethodLiteral *methodLiteral = methodLiterals + (methodIdx++);
        uintptr_t nativePointerOffset = reinterpret_cast<uintptr_t>(methodLiteral->GetNativePointer());
        // calculate relative offset
        methodLiteral->SetNativePointer(reinterpret_cast<void*>(nativePointerOffset + baseAddress));
    }
    jsPandaFile->SetAllMethodLiteralToMap();

    readBuf += methodLiteralSize;
    // read needUpdate size
    uint32_t mainMethodIndexSize = *reinterpret_cast<const uint32_t*>(readBuf);
    readBuf += sizeof(uint32_t);
    // calculte mainMethodIndex and recordName
    uint32_t recordNum = 0;
    while (recordNum < mainMethodIndexSize) {
        // Get mainMethodIndex
        uint32_t mainMethodIndex = *reinterpret_cast<const uint32_t*>(readBuf);
        readBuf += sizeof(uint32_t);
        // Get recordName length
        uint32_t strLen = *reinterpret_cast<const uint32_t*>(readBuf);
        readBuf += sizeof(uint32_t);
        // Get recordName
        CString str(readBuf, strLen);
        readBuf += strLen;
        // set mainMethodIndex record and index
        auto info = jsPandaFile->jsRecordInfo_.find(str);
        if (info != jsPandaFile->jsRecordInfo_.end()) {
            info->second->mainMethodIndex = mainMethodIndex;
        }
        recordNum++;
    }
    LOG_ECMA(INFO) << "JSPandaFileSnapshot: success with: " << filename;
    return true;
}
} // namespace panda::ecmascript