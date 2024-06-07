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

#ifndef ECMASCRIPT_DFX_STACKINFO_JS_STACKINFO_H
#define ECMASCRIPT_DFX_STACKINFO_JS_STACKINFO_H

#include <csignal>
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/extractortool/src/source_map.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/dfx/dump_code/jit_dump_elf.h"
#if defined(PANDA_TARGET_OHOS)
#include "ecmascript/extractortool/src/zip_file.h"
#endif

namespace panda::ecmascript {
typedef bool (*ReadMemFunc)(void *ctx, uintptr_t addr, uintptr_t *val);

static constexpr uint16_t URL_MAX = 1024;
static constexpr uint16_t FUNCTIONNAME_MAX = 1024;

struct JsFrameInfo {
    std::string functionName;
    std::string fileName;
    std::string pos;
    uintptr_t *nativePointer = nullptr;
};

struct JsFunction {
    char functionName[FUNCTIONNAME_MAX];
    char url[URL_MAX];
    int32_t line;
    int32_t column;
    uintptr_t codeBegin;
    uintptr_t codeSize;
};

struct MethodInfo {
    uintptr_t methodId;
    uintptr_t codeBegin;
    uint32_t codeSize;
    MethodInfo(uintptr_t methodId, uintptr_t codeBegin, uint32_t codeSize)
        : methodId(methodId), codeBegin(codeBegin), codeSize(codeSize) {}
    friend bool operator<(const MethodInfo &lhs, const MethodInfo &rhs)
    {
        return lhs.codeBegin < rhs.codeBegin;
    }
};

struct CodeInfo {
    uintptr_t offset;
    uintptr_t methodId;
    uint32_t codeSize;
    CodeInfo(uintptr_t offset, uintptr_t methodId, uint32_t codeSize)
        : offset(offset), methodId(methodId), codeSize(codeSize) {}
};

struct JsFrameDebugInfo {
    EntityId methodId;
    uint32_t offset;
    std::string hapPath;
    std::string filePath;
    JsFrameDebugInfo(EntityId methodId, uint32_t offset, std::string &hapPath, std::string &filePath)
        : methodId(methodId), offset(offset), hapPath(hapPath), filePath(filePath) {}
};

struct ArkUnwindParam {
    void *ctx;
    ReadMemFunc readMem;
    uintptr_t *fp;
    uintptr_t *sp;
    uintptr_t *pc;
    uintptr_t *methodId;
    bool *isJsFrame;
    std::vector<uintptr_t> &jitCache;
    ArkUnwindParam(void *ctx, ReadMemFunc readMem, uintptr_t *fp, uintptr_t *sp, uintptr_t *pc, uintptr_t *methodId,
                   bool *isJsFrame, std::vector<uintptr_t> &jitCache)
        : ctx(ctx), readMem(readMem), fp(fp), sp(sp), pc(pc), methodId(methodId),
          isJsFrame(isJsFrame), jitCache(jitCache) {}
};

struct JsFrame {
    char functionName[FUNCTIONNAME_MAX];
    char url[URL_MAX];
    int32_t line;
    int32_t column;
};

class JSStackTrace {
public:
    JSStackTrace() = default;
    ~JSStackTrace();

    static JSStackTrace *GetInstance();
    static std::optional<MethodInfo> ReadMethodInfo(panda_file::MethodDataAccessor &mda);
    static CVector<MethodInfo> ReadAllMethodInfos(std::shared_ptr<JSPandaFile> jsPandaFile);
    static std::optional<CodeInfo> TranslateByteCodePc(uintptr_t realPc, const CVector<MethodInfo> &vec);
    bool GetJsFrameInfo(uintptr_t byteCodePc, uintptr_t methodId, uintptr_t mapBase,
                        uintptr_t loadOffset, JsFunction *jsFunction);
    static void Destory(JSStackTrace* trace);
private:
    bool AddMethodInfos(uintptr_t mapBase);

    CVector<MethodInfo> methodInfo_;
    std::unordered_map<uintptr_t, std::shared_ptr<JSPandaFile>> jsPandaFiles_;
    std::unordered_map<uintptr_t, CVector<MethodInfo>> methodInfos_;
};

class JSSymbolExtractor {
public:
    JSSymbolExtractor() = default;
    ~JSSymbolExtractor();

    static JSSymbolExtractor* Create();
    static bool Destory(JSSymbolExtractor* extractor);

    void CreateJSPandaFile();
    void CreateJSPandaFile(uint8_t *data, size_t dataSize);
    void CreateSourceMap(const std::string &hapPath);
    void CreateSourceMap(uint8_t *data, size_t dataSize);
    void CreateDebugExtractor();
    bool ParseHapFileData(std::string& hapName);

    uint8_t* GetData();
    uintptr_t GetLoadOffset();
    uintptr_t GetDataSize();

    JSPandaFile* GetJSPandaFile(uint8_t *data = nullptr, size_t dataSize = 0);
    DebugInfoExtractor* GetDebugExtractor();
    SourceMap* GetSourceMap(uint8_t *data = nullptr, size_t dataSize = 0);
    CVector<MethodInfo> GetMethodInfos();

private:
    CVector<MethodInfo> methodInfo_;
    uintptr_t loadOffset_ {0};
    uintptr_t dataSize_ {0};
    std::unique_ptr<uint8_t[]> data_ {nullptr};
    std::shared_ptr<JSPandaFile> jsPandaFile_ {nullptr};
    std::unique_ptr<DebugInfoExtractor> debugExtractor_ {nullptr};
    std::shared_ptr<SourceMap> sourceMap_ {nullptr};
};

class JsStackInfo {
public:
    static std::string BuildInlinedMethodTrace(const JSPandaFile *pf, std::map<uint32_t, uint32_t> &methodOffsets,
        const FrameType frameType);
    static std::string BuildJsStackTrace(JSThread *thread, bool needNative);
    static std::vector<JsFrameInfo> BuildJsStackInfo(JSThread *thread, bool currentStack = false);
    static std::string BuildMethodTrace(Method *method, uint32_t pcOffset, const FrameType frameType,
        bool enableStackSourceFile = true);
    static AOTFileManager *loader;
    static JSRuntimeOptions *options;
    static void BuildCrashInfo(bool isJsCrash, uintptr_t pc = 0);
    static std::unordered_map<EntityId, std::string> nameMap;
    static std::unordered_map<EntityId, std::vector<uint8>> machineCodeMap;

private:
    static void BuildJsStackTraceInfo(JSThread *thread, Method *method, std::string &data,
                                      FrameIterator &it, uint32_t pcOffset);
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_STACKINFO_JS_STACKINFO_H
extern "C" int step_ark_managed_native_frame(
    int pid, uintptr_t *pc, uintptr_t *fp, uintptr_t *sp, char *buf, size_t buf_sz);
extern "C" int get_ark_js_heap_crash_info(
    int pid, uintptr_t *x20, uintptr_t *fp, int out_js_info, char *buf, size_t buf_sz);
extern "C" int ark_parse_js_frame_info(
    uintptr_t byteCodePc, uintptr_t methodId, uintptr_t mapBase, uintptr_t loadOffset, uint8_t *data,
    uint64_t dataSize, uintptr_t extractorptr, panda::ecmascript::JsFunction *jsFunction);
extern "C" int ark_translate_js_frame_info(
    uint8_t *data, size_t dataSize, panda::ecmascript::JsFunction *jsFunction);
extern "C" int step_ark_with_record_jit(panda::ecmascript::ArkUnwindParam *arkUnwindParam);
extern "C" int ark_write_jit_code(
    void *ctx, panda::ecmascript::ReadMemFunc readMem, int fd, const uintptr_t *const jitCodeArray,
    const size_t jitSize);
extern "C" int step_ark(
    void *ctx, panda::ecmascript::ReadMemFunc readMem, uintptr_t *fp, uintptr_t *sp,
    uintptr_t *pc, uintptr_t *methodId, bool *isJsFrame);
extern "C" int ark_create_js_symbol_extractor(uintptr_t *extractorptr);
extern "C" int ark_destory_js_symbol_extractor(uintptr_t extractorptr);
extern "C" int ark_parse_js_file_info(
    uintptr_t byteCodePc, uintptr_t methodId, uintptr_t mapBase, const char* filePath, uintptr_t extractorptr,
    panda::ecmascript::JsFunction *jsFunction);
extern "C" int get_ark_native_frame_info(
    int pid, uintptr_t *pc, uintptr_t *fp, uintptr_t *sp, panda::ecmascript::JsFrame *jsFrame, size_t &size);
extern "C" int ark_parse_js_frame_info_local(uintptr_t byteCodePc, uintptr_t methodId, uintptr_t mapBase,
    uintptr_t loadOffset, panda::ecmascript::JsFunction *jsFunction);
extern "C" int ark_destory_local();
// define in dfx_signal_handler.h
typedef void(*ThreadInfoCallback)(char *buf, size_t len, void *ucontext);
extern "C" void SetThreadInfoCallback(ThreadInfoCallback func) __attribute__((weak));