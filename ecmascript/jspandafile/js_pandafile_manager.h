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

#ifndef ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_MANAGER_H
#define ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_MANAGER_H

#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/panda_file_translator.h"
#include "ecmascript/jspandafile/debug_info_extractor.h"
#include "ecmascript/platform/mutex.h"

namespace panda {
namespace ecmascript {
class Program;

class PUBLIC_API JSPandaFileManager {
public:
    static JSPandaFileManager *GetInstance();

    ~JSPandaFileManager();

    JSHandle<Program> GenerateProgram(EcmaVM *vm, const JSPandaFile *jsPandaFile, std::string_view entryPoint);

    std::shared_ptr<JSPandaFile> LoadJSPandaFile(JSThread *thread, const CString &filename,
                                                 std::string_view entryPoint, bool needUpdate = false,
                                                 const ExecuteTypes &executeType = ExecuteTypes::STATIC);

    std::shared_ptr<JSPandaFile> LoadJSPandaFile(JSThread *thread, const CString &filename, std::string_view entryPoint,
                                                 const void *buffer, size_t size, bool needUpdate = false);

    // load pandafile from secure mem
    std::shared_ptr<JSPandaFile> LoadJSPandaFileSecure(JSThread *thread, const CString &filename,
                                                       std::string_view entryPoint, uint8_t *buffer, size_t size,
                                                       bool needUpdate = false);

    std::shared_ptr<JSPandaFile> OpenJSPandaFile(const CString &filename);

    std::shared_ptr<JSPandaFile> OpenJSPandaFile(const CString &filename, const CString &desc);

    std::shared_ptr<JSPandaFile> OpenJSPandaFileFromBuffer(uint8_t *buffer, size_t size, const CString &filename);

    std::shared_ptr<JSPandaFile> NewJSPandaFile(const panda_file::File *pf, const CString &desc);

    DebugInfoExtractor *GetJSPtExtractor(const JSPandaFile *jsPandaFile);

    DebugInfoExtractor *GetJSPtExtractorAndExtract(const JSPandaFile *jsPandaFile);

    DebugInfoExtractor *CpuProfilerGetJSPtExtractor(const JSPandaFile *jsPandaFile);

    bool CheckFilePath(JSThread *thread, const CString &fileName);

    // for debugger
    template<typename Callback>
    void EnumerateJSPandaFiles(Callback cb)
    {
        LockHolder lock(jsPandaFileLock_);
        for (const auto &item : loadedJSPandaFiles_) {
            if (!cb(item.second)) {
                return;
            }
        }
        for (const auto &item : oldJSPandaFiles_) {
            if (!cb(item)) {
                return;
            }
        }
    }

    template<typename Callback>
    void EnumerateNonVirtualJSPandaFiles(Callback cb)
    {
        LockHolder lock(jsPandaFileLock_);
        for (const auto &item : loadedJSPandaFiles_) {
            if (!cb(item.second)) {
                return;
            }
        }
        for (const auto &item : oldJSPandaFiles_) {
            if (!cb(item)) {
                return;
            }
        }
    }
    std::shared_ptr<JSPandaFile> FindJSPandaFileByNormalizedName(const CString &normalizedName);
    std::shared_ptr<JSPandaFile> FindJSPandaFileByMapBase(uintptr_t mapBase);
    std::shared_ptr<JSPandaFile> FindJSPandaFile(const CString &filename);
    std::shared_ptr<JSPandaFile> FindMergedJSPandaFile();
    void AddJSPandaFile(const std::shared_ptr<JSPandaFile> &jsPandaFile);
    void RemoveJSPandaFile(const JSPandaFile *jsPandaFile);
    void ClearNameMap();
private:
    JSPandaFileManager() = default;

    class JSPandaFileAllocator {
    public:
        static void *AllocateBuffer(size_t size);
        static void FreeBuffer(void *mem);
    };

    std::shared_ptr<JSPandaFile> GenerateJSPandaFile(JSThread *thread, const panda_file::File *pf, const CString &desc,
                                                     std::string_view entryPoint);
    std::shared_ptr<JSPandaFile> GetJSPandaFile(const panda_file::File *pf);
    std::shared_ptr<JSPandaFile> FindJSPandaFileWithChecksum(const CString &filename, uint32_t checksum);
    std::shared_ptr<JSPandaFile> FindJSPandaFileUnlocked(const CString &filename);
    std::shared_ptr<JSPandaFile> GenerateJSPandafileFromBufferCache(JSThread *thread,
                                                                    const CString &filename,
                                                                    std::string_view entryPoint);
    void ObsoleteLoadedJSPandaFile(const CString &filename);

    static void *AllocateBuffer(size_t size);
    static void FreeBuffer(void *mem);

    RecursiveMutex jsPandaFileLock_;
    // JSPandaFile was shared by all vm.
    std::unordered_map<const CString, std::shared_ptr<JSPandaFile>, CStringHash> loadedJSPandaFiles_;
    // for plugin update.
    std::set<std::shared_ptr<JSPandaFile>> oldJSPandaFiles_;
    std::unordered_map<const JSPandaFile *, std::unique_ptr<DebugInfoExtractor>> extractors_;

    friend class JSPandaFile;
};
}  // namespace ecmascript
}  // namespace panda
#endif // ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_MANAGER_H
