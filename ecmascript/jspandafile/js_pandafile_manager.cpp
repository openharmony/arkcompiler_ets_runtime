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

#include "ecmascript/jspandafile/js_pandafile_manager.h"

#include "ecmascript/aot_file_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_file_path.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"

namespace panda::ecmascript {
static const size_t MALLOC_SIZE_LIMIT = 2147483648; // Max internal memory used by the VM declared in options

JSPandaFileManager *JSPandaFileManager::GetInstance()
{
    static JSPandaFileManager jsFileManager;
    return &jsFileManager;
}

JSPandaFileManager::~JSPandaFileManager()
{
    os::memory::LockHolder lock(jsPandaFileLock_);
    auto pos = extractors_.begin();
    while (pos != extractors_.end()) {
        pos = extractors_.erase(pos);
    }

    auto iterOld = oldJSPandaFiles_.begin();
    while (iterOld != oldJSPandaFiles_.end()) {
        const JSPandaFile *jsPandaFile = iterOld->first;
        ReleaseJSPandaFile(jsPandaFile);
        iterOld = oldJSPandaFiles_.erase(iterOld);
    }
    auto iter = loadedJSPandaFiles_.begin();
    while (iter != loadedJSPandaFiles_.end()) {
        const JSPandaFile *jsPandaFile = iter->second.first;
        ReleaseJSPandaFile(jsPandaFile);
        iter = loadedJSPandaFiles_.erase(iter);
    }
}

const JSPandaFile *JSPandaFileManager::LoadJSPandaFile(JSThread *thread, const CString &filename,
    std::string_view entryPoint, bool needUpdate)
{
    {
        os::memory::LockHolder lock(jsPandaFileLock_);
        const JSPandaFile *jsPandaFile = nullptr;
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
            if (!thread->GetEcmaVM()->HasCachedConstpool(jsPandaFile)) {
                IncreaseRefJSPandaFileUnlocked(jsPandaFile);
            }
            return jsPandaFile;
        }
    }
    auto pf = panda_file::OpenPandaFileOrZip(filename, panda_file::File::READ_WRITE);
    if (pf == nullptr) {
        LOG_ECMA(ERROR) << "open file " << filename << " error";
        return nullptr;
    }

    return GenerateJSPandaFile(thread, pf.release(), filename, entryPoint);
}

const JSPandaFile *JSPandaFileManager::LoadJSPandaFile(JSThread *thread, const CString &filename,
    std::string_view entryPoint, const void *buffer, size_t size, bool needUpdate)
{
    if (buffer == nullptr || size == 0) {
        return nullptr;
    }
    {
        os::memory::LockHolder lock(jsPandaFileLock_);
        const JSPandaFile *jsPandaFile = nullptr;
        if (needUpdate) {
            auto pf = panda_file::OpenPandaFileFromMemory(buffer, size);
            if (pf == nullptr) {
                LOG_ECMA(ERROR) << "open file " << filename << " error";
                return nullptr;
            }
            jsPandaFile = FindJSPandaFileWithChecksum(filename, pf->GetHeader()->checksum);
        } else {
            jsPandaFile = FindJSPandaFileUnlocked(filename);
        }
        if (jsPandaFile != nullptr) {
            if (!thread->GetEcmaVM()->HasCachedConstpool(jsPandaFile)) {
                IncreaseRefJSPandaFileUnlocked(jsPandaFile);
            }
            return jsPandaFile;
        }
    }

    auto pf = panda_file::OpenPandaFileFromMemory(buffer, size);
    if (pf == nullptr) {
        LOG_ECMA(ERROR) << "open file " << filename << " error";
        return nullptr;
    }
    return GenerateJSPandaFile(thread, pf.release(), filename, entryPoint);
}

JSHandle<Program> JSPandaFileManager::GenerateProgram(
    EcmaVM *vm, const JSPandaFile *jsPandaFile, std::string_view entryPoint)
{
    ASSERT(GetJSPandaFile(jsPandaFile->GetPandaFile()) != nullptr);
    if (AnFileDataManager::GetInstance()->IsEnable()) {
        vm->GetAOTFileManager()->LoadAiFile(jsPandaFile);
    }

    return PandaFileTranslator::GenerateProgram(vm, jsPandaFile, entryPoint);
}

const JSPandaFile *JSPandaFileManager::FindJSPandaFileWithChecksum(const CString &filename, uint32_t checksum)
{
    const JSPandaFile *jsPandaFile = FindJSPandaFileUnlocked(filename);
    if (jsPandaFile != nullptr) {
        if (checksum == jsPandaFile->GetChecksum()) {
            return jsPandaFile;
        } else {
            LOG_FULL(INFO) << "reload " << filename << " with new checksum";
            ObsoleteLoadedJSPandaFile(filename);
        }
    }
    return nullptr;
}

const JSPandaFile *JSPandaFileManager::FindJSPandaFileUnlocked(const CString &filename)
{
    if (filename.empty()) {
        return nullptr;
    }
    auto const iter = loadedJSPandaFiles_.find(filename);
    if (iter == loadedJSPandaFiles_.end()) {
        return nullptr;
    }
    return iter->second.first;
}

const JSPandaFile *JSPandaFileManager::FindJSPandaFile(const CString &filename)
{
    os::memory::LockHolder lock(jsPandaFileLock_);
    return FindJSPandaFileUnlocked(filename);
}

const JSPandaFile *JSPandaFileManager::GetJSPandaFile(const panda_file::File *pf)
{
    os::memory::LockHolder lock(jsPandaFileLock_);
    for (const auto &iter : loadedJSPandaFiles_) {
        const JSPandaFile *jsPandafile = iter.second.first;
        if (jsPandafile->GetPandaFile() == pf) {
            return jsPandafile;
        }
    }
    return nullptr;
}

void JSPandaFileManager::InsertJSPandaFile(const JSPandaFile *jsPandaFile)
{
    const auto &filename = jsPandaFile->GetJSPandaFileDesc();
    std::pair<const JSPandaFile *, uint32_t> pandaFileRecord = std::make_pair(jsPandaFile, 1);
    os::memory::LockHolder lock(jsPandaFileLock_);
    ASSERT(loadedJSPandaFiles_.find(filename) == loadedJSPandaFiles_.end());
    loadedJSPandaFiles_[filename] = pandaFileRecord;
}

void JSPandaFileManager::IncreaseRefJSPandaFileUnlocked(const JSPandaFile *jsPandaFile)
{
    auto const filename = jsPandaFile->GetJSPandaFileDesc();
    auto iter = loadedJSPandaFiles_.find(filename);
    ASSERT(iter != loadedJSPandaFiles_.end());
    iter->second.second++;
}

void JSPandaFileManager::DecreaseRefJSPandaFile(const JSPandaFile *jsPandaFile)
{
    os::memory::LockHolder lock(jsPandaFileLock_);
    auto iterOld = oldJSPandaFiles_.find(jsPandaFile);
    if (iterOld != oldJSPandaFiles_.end()) {
        if (iterOld->second > 1) {
            iterOld->second--;
            return;
        }
        oldJSPandaFiles_.erase(iterOld);
    } else {
        const auto &filename = jsPandaFile->GetJSPandaFileDesc();
        auto iter = loadedJSPandaFiles_.find(filename);
        if (iter != loadedJSPandaFiles_.end()) {
            if (iter->second.second > 1) {
                iter->second.second--;
                return;
            }
            loadedJSPandaFiles_.erase(iter);
        }
    }
    extractors_.erase(jsPandaFile);
    ReleaseJSPandaFile(jsPandaFile);
}

void JSPandaFileManager::ObsoleteLoadedJSPandaFile(const CString &filename)
{
    auto iter = loadedJSPandaFiles_.find(filename);
    ASSERT(iter != loadedJSPandaFiles_.end());
    const JSPandaFile *jsPandaFile = iter->second.first;
    if (oldJSPandaFiles_.find(jsPandaFile) == oldJSPandaFiles_.end()) {
        oldJSPandaFiles_.emplace(jsPandaFile, iter->second.second);
    } else {
        oldJSPandaFiles_[jsPandaFile] += iter->second.second;
    }
    loadedJSPandaFiles_.erase(iter);
}

void JSPandaFileManager::ClearCache()
{
    loadedJSPandaFiles_.clear();
}

JSPandaFile *JSPandaFileManager::OpenJSPandaFile(const CString &filename)
{
    auto pf = panda_file::OpenPandaFileOrZip(filename, panda_file::File::READ_WRITE);
    if (pf == nullptr) {
        LOG_ECMA(ERROR) << "open file " << filename << " error";
        return nullptr;
    }

    JSPandaFile *jsPandaFile = NewJSPandaFile(pf.release(), filename);
    return jsPandaFile;
}

JSPandaFile *JSPandaFileManager::NewJSPandaFile(const panda_file::File *pf, const CString &desc)
{
    auto jsPandaFile = new JSPandaFile(pf, desc);
    PGOProfilerManager::GetInstance()->SamplePandaFileInfo(jsPandaFile->GetChecksum());
    return jsPandaFile;
}

void JSPandaFileManager::ReleaseJSPandaFile(const JSPandaFile *jsPandaFile)
{
    if (jsPandaFile == nullptr) {
        return;
    }
    LOG_ECMA(DEBUG) << "ReleaseJSPandaFile " << jsPandaFile->GetJSPandaFileDesc();
    delete jsPandaFile;
    jsPandaFile = nullptr;
}

DebugInfoExtractor *JSPandaFileManager::GetJSPtExtractor(const JSPandaFile *jsPandaFile)
{
    LOG_ECMA_IF(jsPandaFile == nullptr, FATAL) << "GetJSPtExtractor error, js pandafile is nullptr";

    os::memory::LockHolder lock(jsPandaFileLock_);
    [[maybe_unused]] auto const &filename = jsPandaFile->GetJSPandaFileDesc();
    ASSERT(loadedJSPandaFiles_.find(filename) != loadedJSPandaFiles_.end());

    auto iter = extractors_.find(jsPandaFile);
    if (iter == extractors_.end()) {
        auto extractorPtr = std::make_unique<DebugInfoExtractor>(jsPandaFile);
        DebugInfoExtractor *extractor = extractorPtr.get();
        extractors_[jsPandaFile] = std::move(extractorPtr);
        return extractor;
    }

    return iter->second.get();
}

const JSPandaFile *JSPandaFileManager::GenerateJSPandaFile(JSThread *thread, const panda_file::File *pf,
                                                           const CString &desc, std::string_view entryPoint)
{
    ASSERT(GetJSPandaFile(pf) == nullptr);
    JSPandaFile *newJsPandaFile = NewJSPandaFile(pf, desc);
    auto aotFM = thread->GetEcmaVM()->GetAOTFileManager();
    EcmaVM *vm = thread->GetEcmaVM();
    if (aotFM->IsLoad(newJsPandaFile)) {
        uint32_t index = aotFM->GetAnFileIndex(newJsPandaFile);
        newJsPandaFile->SetAOTFileInfoIndex(index);
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
    PandaFileTranslator::TranslateClasses(newJsPandaFile, methodName);
    {
        os::memory::LockHolder lock(jsPandaFileLock_);
        const JSPandaFile *jsPandaFile = FindJSPandaFileUnlocked(desc);
        if (jsPandaFile != nullptr) {
            if (!vm->HasCachedConstpool(jsPandaFile)) {
                IncreaseRefJSPandaFileUnlocked(jsPandaFile);
            }
            ReleaseJSPandaFile(newJsPandaFile);
            return jsPandaFile;
        }
        InsertJSPandaFile(newJsPandaFile);
    }

    return newJsPandaFile;
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

void JSPandaFileManager::RemoveJSPandaFile(void *pointer)
{
    if (pointer == nullptr) {
        return;
    }
    auto jsPandaFile = static_cast<JSPandaFile *>(pointer);
    // dec ref in filemanager
    JSPandaFileManager::GetInstance()->DecreaseRefJSPandaFile(jsPandaFile);
}
}  // namespace panda::ecmascript
