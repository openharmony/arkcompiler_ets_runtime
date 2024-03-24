/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/jspandafile/js_pandafile_manager.h"

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/compiler/aot_file/an_file_data_manager.h"
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/js_file_path.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "file.h"
#include "jsnapi.h"

namespace panda::ecmascript {
using PGOProfilerManager = pgo::PGOProfilerManager;
static const size_t MALLOC_SIZE_LIMIT = 2147483648; // Max internal memory used by the VM declared in options

JSPandaFileManager *JSPandaFileManager::GetInstance()
{
    static JSPandaFileManager *jsFileManager = new JSPandaFileManager();
    return jsFileManager;
}

JSPandaFileManager::~JSPandaFileManager()
{
    LockHolder lock(jsPandaFileLock_);
    extractors_.clear();
    oldJSPandaFiles_.clear();
    loadedJSPandaFiles_.clear();
}

std::shared_ptr<JSPandaFile> JSPandaFileManager::LoadJSPandaFile(JSThread *thread, const CString &filename,
    std::string_view entryPoint, bool needUpdate)
{
    {
        LockHolder lock(jsPandaFileLock_);
        std::shared_ptr<JSPandaFile> jsPandaFile;
        if (needUpdate) {
            auto pf = panda_file::OpenPandaFileOrZip(filename, panda_file::File::READ_WRITE);
            if (pf == nullptr) {
                LOG_ECMA(ERROR) << "open file " << filename << " error";
                return nullptr;
            }
            jsPandaFile = FindJSPandaFileWithChecksum(filename, pf->GetHeader()->checksum);
        } else {
            jsPandaFile = FindJSPandaFileUnlocked(filename);
        }
        if (jsPandaFile != nullptr) {
            InsertJSPandaFileVmUnlocked(thread->GetEcmaVM(), jsPandaFile);
            return jsPandaFile;
        }
    }

    EcmaVM *vm = thread->GetEcmaVM();
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    std::unique_ptr<const panda_file::File> pf;
    if (!vm->IsBundlePack() && moduleManager->GetExecuteMode()) {
        ResolveBufferCallback resolveBufferCallback = vm->GetResolveBufferCallback();
        if (resolveBufferCallback == nullptr) {
#if defined(PANDA_TARGET_WINDOWS) || defined(PANDA_TARGET_MACOS)
            if (vm->EnableReportModuleResolvingFailure()) {
                LOG_NO_TAG(ERROR) << "[ArkRuntime Log] Importing shared package is not supported in the Previewer.";
            }
#endif
            LOG_FULL(FATAL) << "resolveBufferCallback is nullptr";
            return nullptr;
        }
        uint8_t *data = nullptr;
        size_t dataSize = 0;
        bool getBuffer = resolveBufferCallback(ModulePathHelper::ParseHapPath(filename), &data, &dataSize);
        if (!getBuffer) {
#if defined(PANDA_TARGET_WINDOWS) || defined(PANDA_TARGET_MACOS)
            if (vm->EnableReportModuleResolvingFailure()) {
                LOG_NO_TAG(INFO) << "[ArkRuntime Log] Importing shared package in the Previewer.";
            }
#endif
            LOG_FULL(FATAL) << "resolveBufferCallback get buffer failed";
            return nullptr;
        }
        if (!JSNApi::CheckSecureMem(reinterpret_cast<uintptr_t>(data))) {
            LOG_FULL(FATAL) << "Hsp secure memory check failed, please execute in secure memory.";
            return nullptr;
        }
#if defined(PANDA_TARGET_ANDROID) || defined(PANDA_TARGET_IOS)
        pf = panda_file::OpenPandaFileFromMemory(data, dataSize);
#else
        pf = panda_file::OpenPandaFileFromSecureMemory(data, dataSize);
#endif
    } else {
        pf = panda_file::OpenPandaFileOrZip(filename, panda_file::File::READ_WRITE);
    }

    if (pf == nullptr) {
        LOG_ECMA(ERROR) << "open file " << filename << " error";
        return nullptr;
    }

    std::shared_ptr<JSPandaFile> jsPandaFile = GenerateJSPandaFile(thread, pf.release(), filename, entryPoint);
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    if (thread->GetIsProfiling()) {
        GetJSPtExtractorAndExtract(jsPandaFile.get());
    }
#endif
    return jsPandaFile;
}

// The security interface needs to be modified accordingly.
std::shared_ptr<JSPandaFile> JSPandaFileManager::LoadJSPandaFile(JSThread *thread, const CString &filename,
    std::string_view entryPoint, const void *buffer, size_t size, bool needUpdate)
{
    if (buffer == nullptr || size == 0) {
        LOG_FULL(ERROR) << "Input buffer is empty";
        return nullptr;
    }
    {
        LockHolder lock(jsPandaFileLock_);
        std::shared_ptr<JSPandaFile> jsPandaFile;
        if (needUpdate) {
            auto pf = panda_file::OpenPandaFileFromMemory(buffer, size);
            if (pf == nullptr) {
                LOG_ECMA(ERROR) << "open file buffer " << filename << " error";
                return nullptr;
            }
            jsPandaFile = FindJSPandaFileWithChecksum(filename, pf->GetHeader()->checksum);
        } else {
            jsPandaFile = FindJSPandaFileUnlocked(filename);
        }
        if (jsPandaFile != nullptr) {
            InsertJSPandaFileVmUnlocked(thread->GetEcmaVM(), jsPandaFile);
            return jsPandaFile;
        }
    }
#if defined(PANDA_TARGET_PREVIEW)
    auto pf = panda_file::OpenPandaFileFromMemory(buffer, size);
#else
    CString tag = ModulePathHelper::ParseFileNameToVMAName(filename);
    auto pf = panda_file::OpenPandaFileFromMemory(buffer, size, tag.c_str());
#endif
    if (pf == nullptr) {
        LOG_ECMA(ERROR) << "open file " << filename << " error";
        return nullptr;
    }

    // JSPandaFile desc cannot be empty, if buffer with empty filename, use pf filename as a descriptor.
    const CString &desc = filename.empty() ? pf->GetFilename().c_str() : filename;

    std::shared_ptr<JSPandaFile> jsPandaFile = GenerateJSPandaFile(thread, pf.release(), desc, entryPoint);
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    if (thread->GetIsProfiling()) {
        GetJSPtExtractorAndExtract(jsPandaFile.get());
    }
#endif
    return jsPandaFile;
}

std::shared_ptr<JSPandaFile> JSPandaFileManager::LoadJSPandaFileSecure(JSThread *thread, const CString &filename,
    std::string_view entryPoint, uint8_t *buffer, size_t size, bool needUpdate)
{
    if (buffer == nullptr || size == 0) {
        LOG_FULL(ERROR) << "Input buffer is empty";
        return nullptr;
    }
    {
        LockHolder lock(jsPandaFileLock_);
        std::shared_ptr<JSPandaFile> jsPandaFile;
        if (needUpdate) {
            auto pf = panda_file::OpenPandaFileFromSecureMemory(buffer, size);
            if (pf == nullptr) {
                LOG_ECMA(ERROR) << "open file buffer " << filename << " error";
                return nullptr;
            }
            jsPandaFile = FindJSPandaFileWithChecksum(filename, pf->GetHeader()->checksum);
        } else {
            jsPandaFile = FindJSPandaFileUnlocked(filename);
        }
        if (jsPandaFile != nullptr) {
            InsertJSPandaFileVmUnlocked(thread->GetEcmaVM(), jsPandaFile);
            return jsPandaFile;
        }
    }

    auto pf = panda_file::OpenPandaFileFromSecureMemory(buffer, size);
    if (pf == nullptr) {
        LOG_ECMA(ERROR) << "open file " << filename << " error";
        return nullptr;
    }

    // JSPandaFile desc cannot be empty, if buffer with empty filename, use pf filename as a descriptor.
    const CString &desc = filename.empty() ? pf->GetFilename().c_str() : filename;

    std::shared_ptr<JSPandaFile> jsPandaFile = GenerateJSPandaFile(thread, pf.release(), desc, entryPoint);
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    if (thread->GetIsProfiling()) {
        GetJSPtExtractorAndExtract(jsPandaFile.get());
    }
#endif
    return jsPandaFile;
}

JSHandle<Program> JSPandaFileManager::GenerateProgram(EcmaVM *vm, const JSPandaFile *jsPandaFile,
                                                      std::string_view entryPoint)
{
    ASSERT(GetJSPandaFile(jsPandaFile->GetPandaFile()) != nullptr);
    return PandaFileTranslator::GenerateProgram(vm, jsPandaFile, entryPoint);
}

std::shared_ptr<JSPandaFile> JSPandaFileManager::FindJSPandaFileWithChecksum(const CString &filename, uint32_t checksum)
{
    std::shared_ptr<JSPandaFile> jsPandaFile = FindJSPandaFileUnlocked(filename);
    if (jsPandaFile == nullptr) {
        return nullptr;
    }

    if (checksum == jsPandaFile->GetChecksum()) {
        return jsPandaFile;
    }

    LOG_FULL(INFO) << "reload " << filename << " with new checksum";
    ObsoleteLoadedJSPandaFile(filename);
    return nullptr;
}

std::shared_ptr<JSPandaFile> JSPandaFileManager::FindMergedJSPandaFile()
{
    LockHolder lock(jsPandaFileLock_);
    for (const auto &iter : loadedJSPandaFiles_) {
        const std::shared_ptr<JSPandaFile> &jsPandafile = iter.second.first;
        if (jsPandafile->IsFirstMergedAbc()) {
            return jsPandafile;
        }
    }
    return nullptr;
}

std::shared_ptr<JSPandaFile> JSPandaFileManager::FindJSPandaFileUnlocked(const CString &filename)
{
    if (filename.empty()) {
        return nullptr;
    }
    const auto iter = loadedJSPandaFiles_.find(filename);
    if (iter == loadedJSPandaFiles_.end()) {
        return nullptr;
    }
    return iter->second.first;
}

std::shared_ptr<JSPandaFile> JSPandaFileManager::FindJSPandaFileByNormalizedName(const CString &normalizedName)
{
    if (normalizedName.empty()) {
        return nullptr;
    }
    std::shared_ptr<JSPandaFile> result;
    EnumerateJSPandaFiles([&](const std::shared_ptr<JSPandaFile> &file) -> bool {
        // normalize path inside and outside sandbox
        if (file->GetNormalizedFileDesc() == normalizedName) {
            result = file;
            return false;
        }
        return true;
    });
    return result;
}

std::shared_ptr<JSPandaFile> JSPandaFileManager::FindJSPandaFile(const CString &filename)
{
    LockHolder lock(jsPandaFileLock_);
    return FindJSPandaFileUnlocked(filename);
}

std::shared_ptr<JSPandaFile> JSPandaFileManager::GetJSPandaFile(const panda_file::File *pf)
{
    LockHolder lock(jsPandaFileLock_);
    for (const auto &iter : loadedJSPandaFiles_) {
        const std::shared_ptr<JSPandaFile> &jsPandafile = iter.second.first;
        if (jsPandafile->GetPandaFile() == pf) {
            return jsPandafile;
        }
    }
    return nullptr;
}

void JSPandaFileManager::ClearNameMap()
{
    LockHolder lock(jsPandaFileLock_);
    for (const auto &iter : loadedJSPandaFiles_) {
        iter.second.first->ClearNameMap();
    }
}

void JSPandaFileManager::AddJSPandaFileVm(const EcmaVM *vm, const std::shared_ptr<JSPandaFile> &jsPandaFile)
{
    const auto &filename = jsPandaFile->GetJSPandaFileDesc();
    LockHolder lock(jsPandaFileLock_);
    if (loadedJSPandaFiles_.find(filename) != loadedJSPandaFiles_.end()) {
        LOG_ECMA(FATAL) << "add failed, file already exist: " << filename;
        UNREACHABLE();
    }

    std::set<const EcmaVM *> vmSet {vm};
    JSPandaFileVmsPair pandaFileRecord = std::make_pair(jsPandaFile, std::move(vmSet));
    loadedJSPandaFiles_[filename] = std::move(pandaFileRecord);
    LOG_ECMA(DEBUG) << "add file: " << filename;
}

void JSPandaFileManager::InsertJSPandaFileVmUnlocked(const EcmaVM *vm,
                                                     const std::shared_ptr<JSPandaFile> &jsPandaFile)
{
    const auto &filename = jsPandaFile->GetJSPandaFileDesc();
    auto iter = loadedJSPandaFiles_.find(filename);
    if (iter == loadedJSPandaFiles_.end()) {
        LOG_ECMA(FATAL) << "insert vm failed, file not exist: " << filename;
        UNREACHABLE();
    }

    auto &vmSet = iter->second.second;
    vmSet.emplace(vm);
}

void JSPandaFileManager::RemoveJSPandaFileVm(const EcmaVM *vm, const JSPandaFile *jsPandaFile)
{
    if (jsPandaFile == nullptr) {
        return;
    }

    LockHolder lock(jsPandaFileLock_);
    auto iterOld = oldJSPandaFiles_.begin();
    while (iterOld != oldJSPandaFiles_.end()) {
        if (iterOld->first.get() == jsPandaFile) {
            auto &vmSet = iterOld->second;
            vmSet.erase(vm);
            if (vmSet.empty()) {
                extractors_.erase(jsPandaFile);
                oldJSPandaFiles_.erase(iterOld);
            }
            return;
        }
        iterOld++;
    }
    const auto &filename = jsPandaFile->GetJSPandaFileDesc();
    auto iter = loadedJSPandaFiles_.find(filename);
    if (iter != loadedJSPandaFiles_.end()) {
        auto &vmSet = iter->second.second;
        vmSet.erase(vm);
        if (vmSet.empty()) {
            extractors_.erase(jsPandaFile);
            // erase shared_ptr from map, the ref count -1.
            loadedJSPandaFiles_.erase(iter);
        }
    }
}

void JSPandaFileManager::ObsoleteLoadedJSPandaFile(const CString &filename)
{
    auto iter = loadedJSPandaFiles_.find(filename);
    ASSERT(iter != loadedJSPandaFiles_.end());
    std::shared_ptr<JSPandaFile> &jsPandaFile = iter->second.first;
    if (oldJSPandaFiles_.find(jsPandaFile) == oldJSPandaFiles_.end()) {
        oldJSPandaFiles_.emplace(jsPandaFile, iter->second.second);
    } else {
        auto &oldVmSet = oldJSPandaFiles_[jsPandaFile];
        auto &vmSet = iter->second.second;
        oldVmSet.insert(vmSet.begin(), vmSet.end());
    }
    loadedJSPandaFiles_.erase(iter);
}

std::shared_ptr<JSPandaFile> JSPandaFileManager::OpenJSPandaFile(const CString &filename)
{
    return OpenJSPandaFile(filename, filename);
}

std::shared_ptr<JSPandaFile> JSPandaFileManager::OpenJSPandaFile(const CString &filename, const CString &desc)
{
    auto pf = panda_file::OpenPandaFileOrZip(filename, panda_file::File::READ_WRITE);
    if (pf == nullptr) {
        LOG_ECMA(ERROR) << "open file " << filename << " error";
        return nullptr;
    }

    return NewJSPandaFile(pf.release(), desc);
}

std::shared_ptr<JSPandaFile> JSPandaFileManager::OpenJSPandaFileFromBuffer(uint8_t *buffer,
                                                                           size_t size,
                                                                           const CString &filename)
{
    auto pf = panda_file::OpenPandaFileFromMemory(buffer, size);
    if (pf == nullptr) {
        LOG_ECMA(ERROR) << "open file " << filename << " error";
        return nullptr;
    }

    return NewJSPandaFile(pf.release(), filename);
}

std::shared_ptr<JSPandaFile> JSPandaFileManager::NewJSPandaFile(const panda_file::File *pf, const CString &desc)
{
    std::shared_ptr<JSPandaFile> jsPandaFile = std::make_shared<JSPandaFile>(pf, desc);
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(jsPandaFile->GetChecksum(),
                                                           jsPandaFile->GetJSPandaFileDesc());
    return jsPandaFile;
}

DebugInfoExtractor *JSPandaFileManager::GetJSPtExtractor(const JSPandaFile *jsPandaFile)
{
    LOG_ECMA_IF(jsPandaFile == nullptr, FATAL) << "GetJSPtExtractor error, js pandafile is nullptr";

    LockHolder lock(jsPandaFileLock_);
    const auto &filename = jsPandaFile->GetJSPandaFileDesc();
    if (loadedJSPandaFiles_.find(filename) == loadedJSPandaFiles_.end()) {
        LOG_ECMA(FATAL) << "get extractor failed, file not exist: " << filename;
        UNREACHABLE();
    }

    auto iter = extractors_.find(jsPandaFile);
    if (iter == extractors_.end()) {
        auto extractorPtr = std::make_unique<DebugInfoExtractor>(jsPandaFile);
        DebugInfoExtractor *extractor = extractorPtr.get();
        extractors_[jsPandaFile] = std::move(extractorPtr);
        return extractor;
    }

    return iter->second.get();
}

DebugInfoExtractor *JSPandaFileManager::GetJSPtExtractorAndExtract(const JSPandaFile *jsPandaFile)
{
    LOG_ECMA_IF(jsPandaFile == nullptr, FATAL) << "GetJSPtExtractor error, js pandafile is nullptr";

    LockHolder lock(jsPandaFileLock_);
    const auto &filename = jsPandaFile->GetJSPandaFileDesc();
    if (loadedJSPandaFiles_.find(filename) == loadedJSPandaFiles_.end()) {
        LOG_ECMA(FATAL) << "get extractor failed, file not exist: " << filename;
        UNREACHABLE();
    }

    auto iter = extractors_.find(jsPandaFile);
    if (iter == extractors_.end()) {
        auto extractorPtr = std::make_unique<DebugInfoExtractor>(jsPandaFile);
        DebugInfoExtractor *extractor = extractorPtr.get();
        extractor->Extract();
        extractors_[jsPandaFile] = std::move(extractorPtr);
        return extractor;
    }

    return iter->second.get();
}

DebugInfoExtractor *JSPandaFileManager::CpuProfilerGetJSPtExtractor(const JSPandaFile *jsPandaFile)
{
    LOG_ECMA_IF(jsPandaFile == nullptr, FATAL) << "GetJSPtExtractor error, js pandafile is nullptr";

    LockHolder lock(jsPandaFileLock_);
    const auto &filename = jsPandaFile->GetJSPandaFileDesc();
    if (loadedJSPandaFiles_.find(filename) == loadedJSPandaFiles_.end()) {
        LOG_ECMA(FATAL) << "get extractor failed, file not exist: " << filename;
        UNREACHABLE();
    }

    DebugInfoExtractor *extractor = nullptr;
    auto iter = extractors_.find(jsPandaFile);
    if (iter == extractors_.end()) {
        auto extractorPtr = std::make_unique<DebugInfoExtractor>(jsPandaFile);
        extractor = extractorPtr.get();
        extractors_[jsPandaFile] = std::move(extractorPtr);
    } else {
        extractor = iter->second.get();
    }

    extractor->Extract();
    return extractor;
}

std::string GetModuleNameFromDesc(const std::string &desc)
{
    /*
    handle desc like:
    case1: /data/storage/el1/bundle/entry/ets/modules.abc -> entry/ets/modules.abc
    case2: /data/storage/el1/bundle/entry/ets/widgets.abc -> entry/ets/widgets.abc
    case3: /data/app/el1/bundle/public/com.xx.xx/entry/ets/modules.abc -> entry/ets/modules.abc
    case4: /data/app/el1/bundle/public/com.xx.xx/entry/ets/widgets.abc -> entry/ets/widgets.abc
    */
    auto lastSlash = desc.rfind("/");
    if (lastSlash == std::string::npos) {
        LOG_ECMA(DEBUG) << "GetModuleNameFromDesc can't find fisrt /: " << desc;
        return "";
    }

    auto secondLastSlash = desc.rfind("/", lastSlash - 1);
    if (secondLastSlash == std::string::npos) {
        LOG_ECMA(DEBUG) << "GetModuleNameFromDesc can't find second /: " << desc;
        return "";
    }

    auto thirdLastSlash = desc.rfind("/", secondLastSlash - 1);
    if (thirdLastSlash == std::string::npos) {
        LOG_ECMA(DEBUG) << "GetModuleNameFromDesc can't find third /: " << desc;
        return "";
    }
    // get moduleName from thirdLastSlash to secondLastSlash
    return desc.substr(thirdLastSlash + 1, secondLastSlash - thirdLastSlash - 1);
}

std::shared_ptr<JSPandaFile> JSPandaFileManager::GenerateJSPandaFile(JSThread *thread, const panda_file::File *pf,
                                                                     const CString &desc, std::string_view entryPoint)
{
    ThreadNativeScope nativeScope(thread);
    ASSERT(GetJSPandaFile(pf) == nullptr);
    std::shared_ptr<JSPandaFile> newJsPandaFile = NewJSPandaFile(pf, desc);
    EcmaVM *vm = thread->GetEcmaVM();

    std::string moduleName = GetModuleNameFromDesc(desc.c_str());
    std::string hapPath;
    SearchHapPathCallBack callback = vm->GetSearchHapPathCallBack();
    if (callback) {
        callback(moduleName, hapPath);
        LOG_ECMA(DEBUG) << "SearchHapPathCallBack moduleName: " << moduleName
                        << ", fileName:" << desc << ", hapPath: " << hapPath;
        newJsPandaFile->SetHapPath(hapPath.c_str());
    }

    CString methodName = entryPoint.data();
    if (newJsPandaFile->IsBundlePack()) {
        // entryPoint maybe is _GLOBAL::func_main_watch to execute func_main_watch
        auto pos = entryPoint.find_last_of("::");
        if (pos != std::string_view::npos) {
            methodName = entryPoint.substr(pos + 1);
        } else {
            // default use func_main_0 as entryPoint
            methodName = JSPandaFile::ENTRY_FUNCTION_NAME;
        }
    }
    PandaFileTranslator::TranslateClasses(newJsPandaFile.get(), methodName);

    {
        // For worker, JSPandaFile may be created by another vm.
        LockHolder lock(jsPandaFileLock_);
        std::shared_ptr<JSPandaFile> jsPandaFile = FindJSPandaFileUnlocked(desc);
        if (jsPandaFile != nullptr) {
            InsertJSPandaFileVmUnlocked(vm, jsPandaFile);
            newJsPandaFile.reset();
            return jsPandaFile;
        } else {
            AddJSPandaFileVm(vm, newJsPandaFile);
            return newJsPandaFile;
        }
    }
}

void *JSPandaFileManager::AllocateBuffer(size_t size)
{
    return JSPandaFileAllocator::AllocateBuffer(size);
}

void *JSPandaFileManager::JSPandaFileAllocator::AllocateBuffer(size_t size)
{
    if (size == 0) {
        LOG_ECMA_MEM(FATAL) << "size must have a size bigger than 0";
        UNREACHABLE();
    }
    if (size >= MALLOC_SIZE_LIMIT) {
        LOG_ECMA_MEM(FATAL) << "size must be less than the maximum";
        UNREACHABLE();
    }
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    void *ptr = malloc(size);
    if (ptr == nullptr) {
        LOG_ECMA_MEM(FATAL) << "malloc failed";
        UNREACHABLE();
    }
#if ECMASCRIPT_ENABLE_ZAP_MEM
    if (memset_s(ptr, size, INVALID_VALUE, size) != EOK) {
        LOG_ECMA_MEM(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
#endif
    return ptr;
}

void JSPandaFileManager::FreeBuffer(void *mem)
{
    JSPandaFileAllocator::FreeBuffer(mem);
}

void JSPandaFileManager::JSPandaFileAllocator::FreeBuffer(void *mem)
{
    if (mem == nullptr) {
        return;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    free(mem);
}
}  // namespace panda::ecmascript
