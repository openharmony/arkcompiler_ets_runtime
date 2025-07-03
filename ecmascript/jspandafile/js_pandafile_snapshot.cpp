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
    // check application white list & specific file
    if (IsJSPandaFileSnapshotFileExist(jsPandaFile->GetJSPandaFileDesc(), path)) {
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

bool JSPandaFileSnapshot::WriteDataToFile(JSThread *thread, JSPandaFile *jsPandaFile, const CString &path,
    const CString &version)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "JSPandaFileSnapshot::WriteDataToFile", "");
    CString filename = GetJSPandaFileFileName(jsPandaFile->GetJSPandaFileDesc(), path);
    if (FileExist(filename.c_str())) {
        LOG_ECMA(INFO) << "JSPandaFileSnapshot::WriteDataToFile file already exist";
        return false;
    }
    LOG_ECMA(INFO) << "JSPandaFileSnapshot::WriteDataToFile: " << filename;
    // calculate file size
    uint32_t checksumSize = sizeof(uint32_t);
    uint32_t fileSize = sizeof(uint32_t);
    uint32_t appVersionCodeSize = sizeof(uint32_t);
    uint32_t versionStrLenSize = sizeof(uint32_t);
    uint32_t versionStrLen = version.size();
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
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot::WriteDataToFile open file failed:" << filename;
        return false;
    }
    MemMapScope memMapScope(fileMapMem);
    FileMemMapWriter writer(fileMapMem, "JSPandaFileSnapshot::WriteDataToFile");
    // write versionCode
    uint32_t appVersionCode = thread->GetEcmaVM()->GetApplicationVersionCode();
    if (!writer.WriteSingleData(&appVersionCode, sizeof(appVersionCode), "appVersionCode")) {
        return false;
    }
    if (!writer.WriteSingleData(&versionStrLen, sizeof(versionStrLen), "versionStrLen")) {
        return false;
    }
    if (!writer.WriteSingleData(version.c_str(), versionStrLen, "versionStr")) {
        return false;
    }
    // write pandafile size
    uint32_t fsize = jsPandaFile->GetFileSize();
    if (!writer.WriteSingleData(&fsize, sizeof(fsize), "fsize")) {
        return false;
    }
    // write moduleName
    if (!writer.WriteSingleData(&moduleNameLen, sizeof(moduleNameLen), "moduleNameLen")) {
        return false;
    }
    if (!writer.WriteSingleData(moduleName.c_str(), moduleNameLen, "moduleName")) {
        return false;
    }
    // write numMethods
    if (!writer.WriteSingleData(&numMethods, sizeof(numMethods), "numMethods")) {
        return false;
    }
    size_t methodLiteralSize = sizeof(MethodLiteral) * numMethods;
    // write MethodLiterals
    if (!writer.WriteSingleData(jsPandaFile->GetMethodLiterals(), methodLiteralSize, "methodLiterals")) {
        return false;
    }
    MethodLiteral *methodLiterals = reinterpret_cast<MethodLiteral*>(writer.GetWritePtr() - methodLiteralSize);
    size_t methodIdx = 0;
    // get current pandafile base offset
    uintptr_t baseAddress = reinterpret_cast<uintptr_t>(jsPandaFile->GetBase());
    while (methodIdx < jsPandaFile->numMethods_) {
        MethodLiteral *methodLiteral = methodLiterals + (methodIdx++);
        uintptr_t nativePointerAddress = reinterpret_cast<uintptr_t>(methodLiteral->GetNativePointer());
        // calculate relative offset
        methodLiteral->SetNativePointer(reinterpret_cast<void*>(nativePointerAddress - baseAddress));
    }
    // write mainMethodIndexSize
    if (!writer.WriteSingleData(&mainMethodIndexSize, sizeof(mainMethodIndexSize), "mainMethodIndexSize")) {
        return false;
    }
    // write mainMethodIndex & recordName
    mainMethodIndex = JSPandaFile::DEFAULT_MAIN_METHOD_INDEX;
    for (auto &[recordName, recordInfo]: jsPandaFile->GetJSRecordInfo()) {
        mainMethodIndex = recordInfo->mainMethodIndex;
        if (mainMethodIndex != JSPandaFile::DEFAULT_MAIN_METHOD_INDEX) {
            if (!writer.WriteSingleData(&mainMethodIndex, sizeof(mainMethodIndex), "mainMethodIndex")) {
                return false;
            }
            uint32_t recordNameLen = recordName.size();
            if (!writer.WriteSingleData(&recordNameLen, sizeof(recordNameLen), "recordNameLen")) {
                return false;
            }
            if (!writer.WriteSingleData(recordName.data(), recordNameLen, "recordName")) {
                return false;
            }
        }
    }
    uint32_t contentSize = fileMapMem.GetSize() - checksumSize;
    uint32_t checksum = adler32(0, static_cast<const Bytef*>(fileMapMem.GetOriginAddr()), contentSize);
    if (!writer.WriteSingleData(&checksum, checksumSize, "checksum")) {
        return false;
    }
    FileSync(fileMapMem, FILE_MS_SYNC);
    return true;
}

bool JSPandaFileSnapshot::ReadDataFromFile(JSThread *thread, JSPandaFile *jsPandaFile, const CString &path,
    const CString &version)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "JSPandaFileSnapshot::ReadDataFromFile", "");
    CString filename = GetJSPandaFileFileName(jsPandaFile->GetJSPandaFileDesc(), path);
    MemMap fileMapMem = FileMap(filename.c_str(), FILE_RDONLY, PAGE_PROT_READ, 0);
    if (fileMapMem.GetOriginAddr() == nullptr) {
        RemoveSnapshotFiles(path);
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot::ReadDataFromFile open file failed:" << filename;
        return false;
    }
    LOG_ECMA(INFO) << "JSPandaFileSnapshot::ReadDataFromFile: " << filename;
    MemMapScope memMapScope(fileMapMem);
    FileMemMapReader reader(fileMapMem, std::bind(RemoveSnapshotFiles, path), "JSPandaFileSnapshot::ReadDataFromFile");

    size_t checksumSize = sizeof(uint32_t);
    uint32_t contentSize = fileMapMem.GetSize() - checksumSize;
    uint32_t readCheckSum = 0;
    if (!reader.ReadFromOffset(&readCheckSum, checksumSize, contentSize, "checksum")) {
        return false;
    }
    uint32_t checksum = adler32(0, static_cast<const Bytef*>(fileMapMem.GetOriginAddr()), contentSize);
    if (checksum != readCheckSum) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot::ReadDataFromFile checksum compare failed, checksum: " << checksum
            << ", readCheckSum: " << readCheckSum;
        RemoveSnapshotFiles(path);
        return false;
    }
    // verify version code
    uint32_t appVersionCode = thread->GetEcmaVM()->GetApplicationVersionCode();
    uint32_t readAppVersionCode = 0;
    if (!reader.ReadSingleData(&readAppVersionCode, sizeof(readAppVersionCode), "appVersionCode")) {
        return false;
    }
    if (appVersionCode != readAppVersionCode) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot::ReadDataFromFile version compare failed, appVersionCode: "
            << appVersionCode << ", readAppVersionCode: " << readAppVersionCode;
        RemoveSnapshotFiles(path);
        return false;
    }
    uint32_t readVersionStrLen = 0;
    if (!reader.ReadSingleData(&readVersionStrLen, sizeof(readVersionStrLen), "readVersionStrLen")) {
        return false;
    }
    CString readVersionStr;
    if (!reader.ReadString(readVersionStr, readVersionStrLen, "readVersionStr")) {
        return false;
    }
    if (version != readVersionStr) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot::ReadDataFromFile version compare failed, version: " << version
            << ", readVersion: " << readVersionStr;
        RemoveSnapshotFiles(path);
        return false;
    }
    // verify filesize
    uint32_t fsize = 0;
    if (!reader.ReadSingleData(&fsize, sizeof(fsize), "fsize")) {
        return false;
    }
    if (fsize != jsPandaFile->GetFileSize()) {
        LOG_COMPILER(ERROR) << "JSPandaFileSnapshot::ReadDataFromFile file size not equal, " << filename  << ", old = "
            << fsize << ", new = " << jsPandaFile->GetFileSize();
        RemoveSnapshotFiles(path);
        return false;
    }
    // verify moduleName
    // read moduleName len
    uint32_t moduleNameLen = 0;
    if (!reader.ReadSingleData(&moduleNameLen, sizeof(moduleNameLen), "moduleNameLen")) {
        return false;
    }
    // Get moduleName
    CString readModuleName;
    if (!reader.ReadString(readModuleName, moduleNameLen, "readModuleName")) {
        return false;
    }
    CString curModuleName = ModulePathHelper::GetModuleNameWithBaseFile(jsPandaFile->GetJSPandaFileDesc());
    if (readModuleName != curModuleName) {
        LOG_ECMA(ERROR) << "JSPandaFileSnapshot::ReadDataFromFile moduleName check failed, read moduleName is: "
            << readModuleName << ", current moduleName is:" << curModuleName;
        RemoveSnapshotFiles(path);
        return false;
    }
    // read numMethods
    uint32_t numMethods = 0;
    if (!reader.ReadSingleData(&numMethods, sizeof(numMethods), "numMethods")) {
        return false;
    }
    jsPandaFile->numMethods_ = numMethods;
    // read MethodLiterals
    MethodLiteral *methodLiterals = jsPandaFile->GetMethodLiterals();
    size_t methodLiteralSize = numMethods * sizeof(MethodLiteral);
    if (!reader.ReadSingleData(methodLiterals, methodLiteralSize, "methodLiteralSize")) {
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

    // read needUpdate size
    uint32_t mainMethodIndexSize = 0;
    if (!reader.ReadSingleData(&mainMethodIndexSize, sizeof(mainMethodIndexSize), "mainMethodIndexSize")) {
        return false;
    }
    // calculte mainMethodIndex and recordName
    uint32_t recordNum = 0;
    while (recordNum < mainMethodIndexSize) {
        // Get mainMethodIndex
        uint32_t mainMethodIndex = 0;
        if (!reader.ReadSingleData(&mainMethodIndex, sizeof(mainMethodIndex), "mainMethodIndex")) {
            return false;
        }
        // Get recordName length
        uint32_t recordNameLen = 0;
        if (!reader.ReadSingleData(&recordNameLen, sizeof(recordNameLen), "recordNameLen")) {
            return false;
        }
        // Get recordName
        CString recordName;
        if (!reader.ReadString(recordName, recordNameLen, "recordName")) {
            return false;
        }
        // set mainMethodIndex record and index
        auto info = jsPandaFile->jsRecordInfo_.find(recordName);
        if (info != jsPandaFile->jsRecordInfo_.end()) {
            info->second->mainMethodIndex = mainMethodIndex;
        }
        recordNum++;
    }
    LOG_ECMA(INFO) << "JSPandaFileSnapshot::ReadDataFromFile success with: " << filename;
    return true;
}
} // namespace panda::ecmascript