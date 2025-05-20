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
#include "ecmascript/frames.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/dfx/dump_code/jit_dump_elf.h"
#if defined(PANDA_TARGET_OHOS)
#include "ecmascript/extractortool/src/zip_file.h"
#endif

namespace panda::ecmascript {
typedef bool (*ReadMemFunc)(void *ctx, uintptr_t addr, uintptr_t *val);
bool ArkFrameCheck(uintptr_t frameType);
bool IsJsFunctionFrame(uintptr_t frameType);
bool IsNativeFunctionFrame(uintptr_t frameType);
bool IsAotFunctionFrame(uintptr_t frameType);
bool IsFastJitFunctionFrame(uintptr_t frameType);
bool IsFastJitFunctionFrame(const FrameType frameType);
template<typename T>
void ParseJsFrameInfo(JSPandaFile *jsPandaFile, DebugInfoExtractor *debugExtractor,
    EntityId methodId, uintptr_t offset, T &jsFrame, SourceMap *sourceMap = nullptr);


static constexpr uint16_t URL_MAX = 1024;
static constexpr uint16_t FUNCTIONNAME_MAX = 1024;
static constexpr uint16_t PACKAGENAME_MAX = 1024;
static constexpr uint32_t JS_STACK_TRACE_DEPTH_MAX = 64;

struct ArkStepParam {
    uintptr_t *fp;
    uintptr_t *sp;
    uintptr_t *pc;
    bool *isJsFrame;
};

struct JsFrameInfo {
    std::string functionName;
    std::string fileName;
    std::string packageName;
    std::string pos;
};

struct JsFunction {
    char functionName[FUNCTIONNAME_MAX];
    char packageName[PACKAGENAME_MAX];
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
    char packageName[PACKAGENAME_MAX];
    char url[URL_MAX];
    int32_t line;
    int32_t column;
};

class JSStackTrace {
public:
    JSStackTrace() = default;
    ~JSStackTrace();
    static JSStackTrace *GetInstance()
    {
        return trace_;
    }
    static std::optional<MethodInfo> ReadMethodInfo(panda_file::MethodDataAccessor &mda);
    static CVector<MethodInfo> ReadAllMethodInfos(std::shared_ptr<JSPandaFile> jsPandaFile);
    static std::optional<CodeInfo> TranslateByteCodePc(uintptr_t realPc, const CVector<MethodInfo> &vec);
    bool GetJsFrameInfo(uintptr_t byteCodePc, uintptr_t mapBase, uintptr_t loadOffset, JsFunction *jsFunction);
    static void AddReference();
    static void ReleaseReference();
private:
    std::shared_ptr<JSPandaFile> FindJSpandaFile(uintptr_t mapBase);
    const CVector<MethodInfo> &FindMethodInfos(uintptr_t mapBase);
    void SetJSpandaFile(uintptr_t mapBase, std::shared_ptr<JSPandaFile> pandafile);
    void SetMethodInfos(uintptr_t mapBase, CVector<MethodInfo> &infos);
    bool InitializeMethodInfo(uintptr_t mapBase);

    std::shared_mutex pfMutex_;
    std::shared_mutex infosMutex_;
    CVector<MethodInfo> methodInfo_; // empty vector
    std::unordered_map<uintptr_t, std::shared_ptr<JSPandaFile>> jsPandaFiles_;
    std::unordered_map<uintptr_t, CVector<MethodInfo>> methodInfos_;
    static JSStackTrace *trace_;
    static std::mutex mutex_;
    static size_t count_;
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
    uint8_t* data_ {nullptr};
    std::shared_ptr<JSPandaFile> jsPandaFile_ {nullptr};
    std::unique_ptr<DebugInfoExtractor> debugExtractor_ {nullptr};
    std::shared_ptr<SourceMap> sourceMap_ {nullptr};
};

class JsStackInfo {
private:
    struct LastBuilderCache {
        const JSPandaFile *pf{nullptr};
        DebugInfoExtractor *extractor{nullptr};
    };
public:
    static inline std::string BuildJsStackTrace(JSThread *thread, bool needNative,
                                                bool needNativeStack = true, uint32_t depth = UINT32_MAX)
    {
        // If jsErrorObj not be pass in, MachineCode object of its stack frame while not be keep alive
        JSHandle<JSObject> jsErrorObj;
        return BuildJsStackTrace(thread, needNative, jsErrorObj, needNativeStack, depth);
    }
    static std::string BuildJsStackTrace(JSThread *thread, bool needNative, const JSHandle<JSObject> &jsErrorObj,
                                         bool needNativeStack, uint32_t depth = UINT32_MAX);
    static std::vector<JsFrameInfo> BuildJsStackInfo(JSThread *thread, bool currentStack = false);
    static std::string BuildMethodTrace(Method *method, uint32_t pcOffset, LastBuilderCache &lastCache,
                                        bool enableStackSourceFile = true);
    static AOTFileManager *loader;
    static JSRuntimeOptions *options;
    static void BuildCrashInfo(bool isJsCrash, uintptr_t pc = 0, JSThread *thread = nullptr);
    static inline void BuildCrashInfo(JSThread *thread)
    {
        BuildCrashInfo(true, 0, thread); // pc is useless for JsCrash, pass 0 as placeholder
    }
    static std::unordered_map<EntityId, std::string> nameMap;
    static std::unordered_map<EntityId, std::vector<uint8>> machineCodeMap;
    static void DumpJitCode(JSThread *thread);

private:
    static std::string BuildJsStackTraceInfo(JSThread *thread, Method *method, FrameIterator &it,
                                             const JSHandle<JSObject> &jsErrorObj,
                                             LastBuilderCache &lastCache,
                                             bool needBaselineSpecialHandling,
                                             uint32_t pcOffset);
    static constexpr int32_t InitialLength = 50;
    static constexpr int32_t InitialDeeps = 5;
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_STACKINFO_JS_STACKINFO_H
extern "C" int ark_parse_js_frame_info(
    uintptr_t byteCodePc, uintptr_t mapBase, uintptr_t loadOffset, uint8_t *data,
    uint64_t dataSize, uintptr_t extractorptr, panda::ecmascript::JsFunction *jsFunction);
extern "C" int step_ark_with_record_jit(panda::ecmascript::ArkUnwindParam *arkUnwindParam);
extern "C" int ark_write_jit_code(
    void *ctx, panda::ecmascript::ReadMemFunc readMem, int fd, const uintptr_t *const jitCodeArray,
    const size_t jitSize);
extern "C" int step_ark(
    void *ctx, panda::ecmascript::ReadMemFunc readMem, panda::ecmascript::ArkStepParam *arkStepParam);
extern "C" int ark_create_js_symbol_extractor(uintptr_t *extractorptr);
extern "C" int ark_destory_js_symbol_extractor(uintptr_t extractorptr);
extern "C" int ark_parse_js_file_info(
    uintptr_t byteCodePc, uintptr_t mapBase, const char* filePath, uintptr_t extractorptr,
    panda::ecmascript::JsFunction *jsFunction);
extern "C" int ark_parse_js_frame_info_local(uintptr_t byteCodePc, uintptr_t mapBase,
    uintptr_t loadOffset, panda::ecmascript::JsFunction *jsFunction);
extern "C" int ark_create_local();
extern "C" int ark_destroy_local();
// define in dfx_signal_handler.h
typedef void(*ThreadInfoCallback)(char *buf, size_t len, void *ucontext);
extern "C" void SetThreadInfoCallback(ThreadInfoCallback func) __attribute__((weak));