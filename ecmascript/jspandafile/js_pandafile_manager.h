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

#include "ecmascript/mem/c_containers.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/panda_file_translator.h"
#include "ecmascript/jspandafile/debug_info_extractor.h"

#include "libpandafile/file.h"

namespace panda {
namespace ecmascript {
class Program;

class PUBLIC_API JSPandaFileManager {
public:
    static JSPandaFileManager *GetInstance();

    ~JSPandaFileManager();

    JSHandle<Program> GenerateProgram(EcmaVM *vm, const JSPandaFile *jsPandaFile, std::string_view entryPoint);

    const JSPandaFile *LoadJSPandaFile(JSThread *thread, const CString &filename, std::string_view entryPoint,
                                       bool needUpdate = false);

    const JSPandaFile *LoadJSPandaFile(JSThread *thread, const CString &filename, std::string_view entryPoint,
                                       const void *buffer, size_t size, bool needUpdate = false);

    JSPandaFile *OpenJSPandaFile(const CString &filename);

    JSPandaFile *NewJSPandaFile(const panda_file::File *pf, const CString &desc);

    DebugInfoExtractor *GetJSPtExtractor(const JSPandaFile *jsPandaFile);

    static void RemoveJSPandaFile(void *pointer);

    // for debugger
    template<typename Callback>
    void EnumerateJSPandaFiles(Callback cb)
    {
        // since there is a lock, so cannot mark function const
        os::memory::LockHolder lock(jsPandaFileLock_);
        for (const auto &iter : loadedJSPandaFiles_) {
            if (!cb(iter.second.first)) {
                return;
            }
        }
        for (const auto &iter : oldJSPandaFiles_) {
            if (!cb(iter.first)) {
                return;
            }
        }
    }

    void InsertJSPandaFile(const JSPandaFile *jsPandaFile);
    void ClearCache();

    const JSPandaFile *FindJSPandaFile(const CString &filename);
private:
    JSPandaFileManager() = default;

    class JSPandaFileAllocator {
    public:
        static void *AllocateBuffer(size_t size);
        static void FreeBuffer(void *mem);
    };

    const JSPandaFile *GenerateJSPandaFile(JSThread *thread, const panda_file::File *pf, const CString &desc,
                                           std::string_view entryPoint);
    void ReleaseJSPandaFile(const JSPandaFile *jsPandaFile);
    const JSPandaFile *GetJSPandaFile(const panda_file::File *pf);
    const JSPandaFile *FindJSPandaFileWithChecksum(const CString &filename, uint32_t checksum);
    const JSPandaFile *FindJSPandaFileUnlocked(const CString &filename);
    void IncreaseRefJSPandaFileUnlocked(const JSPandaFile *jsPandaFile);
    void DecreaseRefJSPandaFile(const JSPandaFile *jsPandaFile);
    void ObsoleteLoadedJSPandaFile(const CString &filename);

    static void *AllocateBuffer(size_t size);
    static void FreeBuffer(void *mem);

    os::memory::RecursiveMutex jsPandaFileLock_;
    std::unordered_map<const CString, std::pair<const JSPandaFile *, uint32_t>, CStringHash> loadedJSPandaFiles_;
    std::unordered_map<const JSPandaFile *, uint32_t> oldJSPandaFiles_;
    std::unordered_map<const JSPandaFile *, std::unique_ptr<DebugInfoExtractor>> extractors_;

    friend class JSPandaFile;
};
}  // namespace ecmascript
}  // namespace panda
#endif // ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_MANAGER_H
