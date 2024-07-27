/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ecmascript/debugger/js_debugger.h"
#include <memory>

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/interpreter/interpreter-inl.h"

namespace panda::ecmascript::tooling {
using panda::ecmascript::base::BuiltinsBase;

std::shared_mutex JSDebugger::listMutex_;
CUnorderedMap<CString, CUnorderedMap<std::string,
    CUnorderedSet<JSBreakpoint, HashJSBreakpoint>>> JSDebugger::globalBpList_ {};
bool JSDebugger::SetBreakpoint(const JSPtLocation &location, Local<FunctionRef> condFuncRef)
{
    // acquire write lock of global list
    std::unique_lock<std::shared_mutex> globalListLock(listMutex_);
    Global<FunctionRef> funcRef = Global<FunctionRef>(ecmaVm_, condFuncRef);
    return SetBreakpointWithMatchedUrl(location, funcRef);
}

bool JSDebugger::SetUrlNotMatchedBreakpoint(const JSPtLocation &location)
{
    // acquire write lock of global list
    std::unique_lock<std::shared_mutex> globalListLock(listMutex_);
    return SetBreakpointWithoutMatchedUrl(location);
}

std::vector<bool> JSDebugger::SetBreakpointUsingList(std::vector<JSPtLocation> &list)
{
    // acquire write lock of global list
    std::unique_lock<std::shared_mutex> globalListLock(listMutex_);
    return SetBreakpointByList(list);
}

std::vector<bool> JSDebugger::SetBreakpointByList(std::vector<JSPtLocation> &list)
{
    std::vector<bool> result;
    for (const auto &location : list) {
        if (location.GetJsPandaFile() != nullptr) {
            // URL matched breakpoint
            auto funcRef = location.GetCondFuncRef();
            result.emplace_back(SetBreakpointWithMatchedUrl(location, funcRef));
        } else if (location.GetSourceFile() == "invalid") {
            // Invalid breakpoint (condition check failed or match location failed)
            result.emplace_back(false);
        } else {
            // URL not matched breakpoint
            result.emplace_back(SetBreakpointWithoutMatchedUrl(location));
        }
    }
    DumpBreakpoints();
    DumpGlobalBreakpoints();
    ASSERT(list.size() == result.size());
    return result;
}

bool JSDebugger::SetBreakpointWithMatchedUrl(const JSPtLocation &location, Global<FunctionRef> condFuncRef)
{
    std::unique_ptr<PtMethod> ptMethod = FindMethod(location);
    if (ptMethod == nullptr) {
        LOG_DEBUGGER(ERROR) << "SetBreakpointWithMatchedUrl: Cannot find MethodLiteral";
        return false;
    }

    if (location.GetBytecodeOffset() >= ptMethod->GetCodeSize()) {
        LOG_DEBUGGER(ERROR) << "SetBreakpointWithMatchedUrl: Invalid breakpoint location";
        return false;
    }
    auto ptMethodPtr = ptMethod.get();
    JSBreakpoint jsBreakpoint{location.GetSourceFile(), ptMethod.release(), location.GetBytecodeOffset(),
        Global<FunctionRef>(ecmaVm_, condFuncRef), location.GetSourceFile(), location.GetLine(), location.GetColumn()};

    bool success = PushBreakpointToLocal(jsBreakpoint, location);
    if (!success) {
        LOG_DEBUGGER(WARN) << "SetBreakpointWithMatchedUrl: Breakpoint already exists in localList";
    }

    auto breakpoint = SearchBreakpointInGlobalList(location, ptMethodPtr);
    if (!breakpoint.has_value() && ecmaVm_->GetJsDebuggerManager()->IsBreakpointSyncEnabled()) {
        // global list does not have this breakpoint, push it to the global list
        PushBreakpointToGlobal(jsBreakpoint, location);
    }
    return true;
}

bool JSDebugger::SetBreakpointWithoutMatchedUrl(const JSPtLocation &location)
{
    // try to find this breakpoint in local list
    auto breakpointList = SearchNoMatchBreakpointInLocalList(location);
    if (!breakpointList.empty()) {
        // found this breakpoint in local list, return success
        return true;
    }
    // if not found in local, try to find it in global list
    breakpointList = SearchNoMatchBreakpointInGlobalList(location);
    if (breakpointList.empty()) {
        return false;
    }
    // found this breakpoint in global list, return success
    for (const auto &breakpoint : breakpointList) {
        PullBreakpointFromGlobal(breakpoint);
    }
    return true;
}

bool JSDebugger::PushBreakpointToLocal(const JSBreakpoint &breakpoint, const JSPtLocation &location)
{
    auto pandaFileKey = location.GetJsPandaFile()->GetJSPandaFileDesc();
    auto urlKey = location.GetSourceFile();
    if (breakpoints_.find(pandaFileKey) == breakpoints_.end()) {
        CUnorderedMap<std::string, CUnorderedSet<JSBreakpoint, HashJSBreakpoint>> urlHashMap;
        CUnorderedSet<JSBreakpoint, HashJSBreakpoint> breakpointSet;
        breakpointSet.emplace(breakpoint);
        urlHashMap[urlKey] = breakpointSet;
        breakpoints_[pandaFileKey] = urlHashMap;
        return true;
    }
    auto urlHashMap = breakpoints_[pandaFileKey];
    if (urlHashMap.find(urlKey) == urlHashMap.end()) {
        CUnorderedSet<JSBreakpoint, HashJSBreakpoint> breakpointSet;
        breakpointSet.emplace(breakpoint);
        urlHashMap[urlKey] = breakpointSet;
        breakpoints_[pandaFileKey] = urlHashMap;
        return true;
    }
    auto [_, success] = breakpoints_[pandaFileKey][urlKey].emplace(breakpoint);
    return success;
}

void JSDebugger::PushBreakpointToGlobal(const JSBreakpoint &breakpoint, const JSPtLocation &location)
{
    auto pandaFileKey = location.GetJsPandaFile()->GetJSPandaFileDesc();
    auto urlKey = location.GetSourceFile();
    if (globalBpList_.find(pandaFileKey) == globalBpList_.end()) {
        CUnorderedMap<std::string, CUnorderedSet<JSBreakpoint, HashJSBreakpoint>> urlHashMap;
        CUnorderedSet<JSBreakpoint, HashJSBreakpoint> breakpointSet;
        breakpointSet.emplace(breakpoint);
        urlHashMap[urlKey] = breakpointSet;
        globalBpList_[pandaFileKey] = urlHashMap;
        return;
    }
    auto urlHashMap = breakpoints_[pandaFileKey];
    if (urlHashMap.find(urlKey) == urlHashMap.end()) {
        CUnorderedSet<JSBreakpoint, HashJSBreakpoint> breakpointSet;
        breakpointSet.emplace(breakpoint);
        urlHashMap[urlKey] = breakpointSet;
        globalBpList_[pandaFileKey] = urlHashMap;
        return;
    }
    globalBpList_[pandaFileKey][urlKey].emplace(breakpoint);
}

bool JSDebugger::PullBreakpointFromGlobal(const JSBreakpoint &breakpoint)
{
    auto pandaFileKey = breakpoint.GetPtMethod()->GetJSPandaFile()->GetJSPandaFileDesc();
    auto urlKey = breakpoint.GetUrl();
    if (breakpoints_.find(pandaFileKey) == breakpoints_.end()) {
        CUnorderedMap<std::string, CUnorderedSet<JSBreakpoint, HashJSBreakpoint>> urlHashMap;
        CUnorderedSet<JSBreakpoint, HashJSBreakpoint> breakpointSet;
        breakpointSet.emplace(breakpoint);
        urlHashMap[urlKey] = breakpointSet;
        breakpoints_[pandaFileKey] = urlHashMap;
        return true;
    }
    auto urlHashMap = breakpoints_[pandaFileKey];
    if (urlHashMap.find(urlKey) == urlHashMap.end()) {
        CUnorderedSet<JSBreakpoint, HashJSBreakpoint> breakpointSet;
        breakpointSet.emplace(breakpoint);
        urlHashMap[urlKey] = breakpointSet;
        breakpoints_[pandaFileKey] = urlHashMap;
        return true;
    }
    auto [_, success] = breakpoints_[pandaFileKey][urlKey].emplace(breakpoint);
    return success;
}

std::vector<JSBreakpoint> JSDebugger::SearchNoMatchBreakpointInLocalList(const JSPtLocation &location)
{
    std::vector<JSBreakpoint> result{};
    if (breakpoints_.empty()) {
        return result;
    }
    for (const auto &entry : breakpoints_) {
        auto urlHashMap = breakpoints_[entry.first];
        auto urlKey = location.GetSourceFile();
        if (urlHashMap.find(urlKey) != urlHashMap.end()) {
            auto breakpointSet = urlHashMap[urlKey];
            for (const auto &bp : breakpointSet) {
                if ((bp.GetLine() == location.GetLine()) &&
                    (bp.GetColumn() == location.GetColumn())) {
                    result.emplace_back(bp);
                }
            }
        }
    }
    return result;
}

std::vector<JSBreakpoint> JSDebugger::SearchNoMatchBreakpointInGlobalList(const JSPtLocation &location)
{
    std::vector<JSBreakpoint> result{};
    if (breakpoints_.empty()) {
        return result;
    }
    for (const auto &entry : globalBpList_) {
        auto urlHashMap = globalBpList_[entry.first];
        auto urlKey = location.GetSourceFile();
        if (urlHashMap.find(urlKey) != urlHashMap.end()) {
            auto breakpointSet = urlHashMap[urlKey];
            for (const auto &bp : breakpointSet) {
                if ((bp.GetLine() == location.GetLine()) &&
                    (bp.GetColumn() == location.GetColumn())) {
                    result.emplace_back(bp);
                }
            }
        }
    }
    return result;
}

std::optional<JSBreakpoint> JSDebugger::SearchBreakpointInGlobalList(const JSPtLocation &location, PtMethod *ptMethod)
{
    auto pandaFileKey = ptMethod->GetJSPandaFile()->GetJSPandaFileDesc();
    if (globalBpList_.empty() || globalBpList_.find(pandaFileKey) == globalBpList_.end()) {
        return {};
    }
    auto urlHashMap = globalBpList_[pandaFileKey];
    if (urlHashMap.empty() || urlHashMap.find(location.GetSourceFile()) == urlHashMap.end()) {
        return {};
    }
    for (const auto &bp : urlHashMap[location.GetSourceFile()]) {
        if ((bp.GetBytecodeOffset() == location.GetBytecodeOffset()) &&
            (bp.GetPtMethod()->GetJSPandaFile() == ptMethod->GetJSPandaFile()) &&
            (bp.GetPtMethod()->GetMethodId() == ptMethod->GetMethodId())) {
            return bp;
        }
    }
    return {};
}

std::optional<JSBreakpoint> JSDebugger::SearchBreakpointInGlobalList(JSHandle<Method> method, uint32_t bcOffset)
{
    auto pandaFileKey = method->GetJSPandaFile()->GetJSPandaFileDesc();
    if (globalBpList_.empty() || globalBpList_.find(pandaFileKey) == globalBpList_.end()) {
        return {};
    }
    auto urlHashMap = globalBpList_.at(pandaFileKey);
    for (const auto &entry : urlHashMap) {
        for (const auto &bp : urlHashMap[entry.first]) {
            if ((bp.GetBytecodeOffset() == bcOffset) &&
                (bp.GetPtMethod()->GetJSPandaFile() == method->GetJSPandaFile()) &&
                (bp.GetPtMethod()->GetMethodId() == method->GetMethodId())) {
                return bp;
            }
        }
    }
    return {};
}

bool JSDebugger::RemoveUrlNotMatchedBreakpoint(const JSPtLocation &location)
{
    for (const auto &pandafileEntry : breakpoints_) {
        for (const auto &urlHashMapEntry : breakpoints_[pandafileEntry.first]) {
            if (location.GetSourceFile() == urlHashMapEntry.first) {
                for (auto it = breakpoints_[pandafileEntry.first][urlHashMapEntry.first].begin();
                     it != breakpoints_[pandafileEntry.first][urlHashMapEntry.first].end();) {
                    const auto &bp = *it;
                    if ((bp.GetLine() == location.GetLine()) && (bp.GetColumn() == location.GetColumn())) {
                        it = breakpoints_[pandafileEntry.first][urlHashMapEntry.first].erase(it);
                    } else {
                        it++;
                    }
                }
            }
        }
    }
    {
        std::unique_lock<std::shared_mutex> lock(listMutex_);
        RemoveGlobalBreakpoint(location);
    }
    return true;
}

bool JSDebugger::RemoveGlobalBreakpoint(const std::unique_ptr<PtMethod> &ptMethod, const JSPtLocation &location)
{
    auto pandaFileKey = location.GetJsPandaFile()->GetJSPandaFileDesc();
    if (globalBpList_.empty() || globalBpList_.find(pandaFileKey) == globalBpList_.end()) {
        return false;
    }
    if (globalBpList_[pandaFileKey].empty() ||
        globalBpList_[pandaFileKey].find(location.GetSourceFile()) == globalBpList_[pandaFileKey].end()) {
        return false;
    }
    for (auto it = globalBpList_[pandaFileKey][location.GetSourceFile()].begin();
         it != globalBpList_[pandaFileKey][location.GetSourceFile()].end();) {
        const auto &bp = *it;
        if ((bp.GetBytecodeOffset() == location.GetBytecodeOffset()) &&
            (bp.GetPtMethod()->GetJSPandaFile() == ptMethod->GetJSPandaFile()) &&
            (bp.GetPtMethod()->GetMethodId() == ptMethod->GetMethodId())) {
            it = globalBpList_[pandaFileKey][location.GetSourceFile()].erase(it);
        } else {
            it++;
        }
    }
    return true;
}

bool JSDebugger::RemoveGlobalBreakpoint(const JSPtLocation &location)
{
    for (const auto &entry : globalBpList_) {
        if (globalBpList_[entry.first].find(location.GetSourceFile()) != globalBpList_[entry.first].end()) {
            for (auto it = globalBpList_[entry.first][location.GetSourceFile()].begin();
                 it != globalBpList_[entry.first][location.GetSourceFile()].end();) {
                const auto &bp = *it;
                if ((bp.GetLine() == location.GetLine()) && (bp.GetColumn() == location.GetColumn())) {
                    it = globalBpList_[entry.first][location.GetSourceFile()].erase(it);
                } else {
                    it++;
                }
            }
        }
    }
    return true;
}

bool JSDebugger::RemoveGlobalBreakpoint(const std::string &url)
{
    for (const auto &entry : globalBpList_) {
        if (globalBpList_[entry.first].find(url) != globalBpList_[entry.first].end()) {
            globalBpList_[entry.first].erase(url);
        }
    }
    return true;
}

bool JSDebugger::SetSmartBreakpoint(const JSPtLocation &location)
{
    std::unique_ptr<PtMethod> ptMethod = FindMethod(location);
    if (ptMethod == nullptr) {
        LOG_DEBUGGER(ERROR) << "SetSmartBreakpoint: Cannot find MethodLiteral";
        return false;
    }

    if (location.GetBytecodeOffset() >= ptMethod->GetCodeSize()) {
        LOG_DEBUGGER(ERROR) << "SetSmartBreakpoint: Invalid breakpoint location";
        return false;
    }

    auto [_, success] = smartBreakpoints_.emplace(location.GetSourceFile(), ptMethod.release(),
        location.GetBytecodeOffset(), Global<FunctionRef>(ecmaVm_, FunctionRef::Undefined(ecmaVm_)),
        location.GetSourceFile(), location.GetLine(), location.GetColumn());
    if (!success) {
        // also return true
        LOG_DEBUGGER(WARN) << "SetSmartBreakpoint: Breakpoint already exists";
    }

    DumpBreakpoints();
    return true;
}

bool JSDebugger::RemoveBreakpoint(const JSPtLocation &location)
{
    std::unique_ptr<PtMethod> ptMethod = FindMethod(location);
    if (ptMethod == nullptr) {
        LOG_DEBUGGER(ERROR) << "RemoveBreakpoint: Cannot find MethodLiteral";
        return false;
    }

    if (!RemoveLocalBreakpoint(ptMethod, location.GetBytecodeOffset())) {
        LOG_DEBUGGER(ERROR) << "RemoveBreakpoint: Breakpoint not found";
        return false;
    }

    {
        std::unique_lock<std::shared_mutex> lock(listMutex_);
        RemoveGlobalBreakpoint(ptMethod, location);
    }
    return true;
}

void JSDebugger::RemoveAllBreakpoints()
{
    breakpoints_.clear();
    if (!globalBpList_.empty()) {
        {
            // acquire write lock of global list
            std::unique_lock<std::shared_mutex> globalListLock(listMutex_);
            globalBpList_.clear();
        }
    }
}

bool JSDebugger::RemoveBreakpointsByUrl(const std::string &url)
{
    for (const auto &entry : breakpoints_) {
        if (breakpoints_[entry.first].find(url) != breakpoints_[entry.first].end()) {
            breakpoints_[entry.first].erase(url);
        }
    }
    return true;
}

bool JSDebugger::RemoveAllBreakpointsByUrl(const std::string &url, bool skipGlobal)
{
    RemoveBreakpointsByUrl(url);

    if (!skipGlobal && !globalBpList_.empty()) {
        {
            // acquire write lock of global list
            std::unique_lock<std::shared_mutex> globalListLock(listMutex_);
            RemoveGlobalBreakpoint(url);
        }
    }
    return true;
}

void JSDebugger::BytecodePcChanged(JSThread *thread, JSHandle<Method> method, uint32_t bcOffset)
{
    ASSERT(bcOffset < method->GetCodeSize() && "code size of current Method less then bcOffset");
    HandleExceptionThrowEvent(thread, method, bcOffset);
    // clear singlestep flag
    singleStepOnDebuggerStmt_ = false;
    if (ecmaVm_->GetJsDebuggerManager()->IsMixedStackEnabled()) {
        if (!HandleBreakpoint(method, bcOffset)) {
            HandleNativeOut();
            HandleStep(method, bcOffset);
        }
    } else  {
        if (!HandleStep(method, bcOffset)) {
            HandleBreakpoint(method, bcOffset);
        }
    }
}

bool JSDebugger::HandleNativeOut()
{
    if (hooks_ == nullptr) {
        return false;
    }

    return hooks_->NativeOut();
}

bool JSDebugger::HandleBreakpoint(JSHandle<Method> method, uint32_t bcOffset)
{
    if (hooks_ == nullptr) {
        return false;
    }
    Global<FunctionRef> funcRef = Global<FunctionRef>(ecmaVm_, FunctionRef::Undefined(ecmaVm_));
    auto smartBreakpoint = FindSmartBreakpoint(method, bcOffset);
    if (smartBreakpoint.has_value()) {
        JSPtLocation smartLocation {method->GetJSPandaFile(), method->GetMethodId(), bcOffset, funcRef,
            smartBreakpoint.value().GetLine(),
            smartBreakpoint.value().GetColumn(), smartBreakpoint.value().GetSourceFile()};
        std::unique_ptr<PtMethod> ptMethod = FindMethod(smartLocation);
        RemoveSmartBreakpoint(ptMethod, bcOffset);
        hooks_->Breakpoint(smartLocation);
        return true;
    }

    auto breakpoint = FindLocalBreakpoint(method, bcOffset);
    if (breakpoint.has_value()) {
        // find breakpoint in Local list
        if (!IsBreakpointCondSatisfied(breakpoint)) {
            return false;
        }
        JSPtLocation location {method->GetJSPandaFile(), method->GetMethodId(), bcOffset, funcRef,
        breakpoint.value().GetLine(), breakpoint.value().GetColumn(),
        breakpoint.value().GetSourceFile(), false, method->GetRecordNameStr()};
        hooks_->Breakpoint(location);
    } else {
        // if not found in local, try to find it in global list
        {
            // acquire read lock of global list
            std::shared_lock<std::shared_mutex> globalListLock(listMutex_);
            breakpoint = SearchBreakpointInGlobalList(method, bcOffset);
        }
        if (!breakpoint.has_value()) {
            return false;
        }
        if (!IsBreakpointCondSatisfied(breakpoint)) {
            return false;
        }
        JSPtLocation location {method->GetJSPandaFile(), method->GetMethodId(), bcOffset, funcRef,
        breakpoint.value().GetLine(), breakpoint.value().GetColumn(),
        breakpoint.value().GetSourceFile(), true, method->GetRecordNameStr()};
        hooks_->Breakpoint(location);
        
        if (ecmaVm_->GetJsDebuggerManager()->IsBreakpointSyncEnabled()) {
            PullBreakpointFromGlobal(breakpoint.value());
        }
    }
    
    return true;
}

bool JSDebugger::HandleDebuggerStmt(JSHandle<Method> method, uint32_t bcOffset)
{
    if (hooks_ == nullptr || !ecmaVm_->GetJsDebuggerManager()->IsDebugMode()) {
        return false;
    }
    // if debugger stmt is met by single stepping, disable debugger
    // stmt to prevent pausing on this line twice
    if (singleStepOnDebuggerStmt_) {
        return false;
    }
    auto breakpointAtDebugger = FindLocalBreakpoint(method, bcOffset);
    // if a breakpoint is set on the same line as debugger stmt,
    // the debugger stmt is ineffective
    if (breakpointAtDebugger.has_value()) {
        return false;
    }
    {
        // acquire read lock of global list
        std::shared_lock<std::shared_mutex> globalListLock(listMutex_);
        breakpointAtDebugger = SearchBreakpointInGlobalList(method, bcOffset);
    }
    if (breakpointAtDebugger.has_value()) {
        return false;
    }
    Global<FunctionRef> funcRef = Global<FunctionRef>(ecmaVm_, FunctionRef::Undefined(ecmaVm_));
    
    JSPtLocation location {method->GetJSPandaFile(), method->GetMethodId(), bcOffset};
    hooks_->DebuggerStmt(location);

    return true;
}

void JSDebugger::HandleExceptionThrowEvent(const JSThread *thread, JSHandle<Method> method, uint32_t bcOffset)
{
    if (hooks_ == nullptr || !thread->HasPendingException()) {
        return;
    }
    JSPtLocation throwLocation {method->GetJSPandaFile(), method->GetMethodId(), bcOffset};

    hooks_->Exception(throwLocation);
}

bool JSDebugger::HandleStep(JSHandle<Method> method, uint32_t bcOffset)
{
    if (hooks_ == nullptr) {
        return false;
    }
    JSPtLocation location {method->GetJSPandaFile(), method->GetMethodId(), bcOffset};

    return hooks_->SingleStep(location);
}

std::optional<JSBreakpoint> JSDebugger::FindLocalBreakpoint(JSHandle<Method> method, uint32_t bcOffset)
{
    for (const auto &pandafileEntry : breakpoints_) {
        for (const auto &urlHashMapEntry : breakpoints_[pandafileEntry.first]) {
            auto breakpointSet = breakpoints_[pandafileEntry.first][urlHashMapEntry.first];
            for (const auto &bp : breakpointSet) {
                if ((bp.GetBytecodeOffset() == bcOffset) &&
                    (bp.GetPtMethod()->GetJSPandaFile() == method->GetJSPandaFile()) &&
                    (bp.GetPtMethod()->GetMethodId() == method->GetMethodId())) {
                    return bp;
                }
            }
        }
    }
    return {};
}

std::optional<JSBreakpoint> JSDebugger::FindSmartBreakpoint(JSHandle<Method> method, uint32_t bcOffset) const
{
    for (const auto &bp : smartBreakpoints_) {
        if ((bp.GetBytecodeOffset() == bcOffset) &&
            (bp.GetPtMethod()->GetJSPandaFile() == method->GetJSPandaFile()) &&
            (bp.GetPtMethod()->GetMethodId() == method->GetMethodId())) {
            return bp;
        }
    }
    return {};
}

bool JSDebugger::RemoveLocalBreakpoint(const std::unique_ptr<PtMethod> &ptMethod, uint32_t bcOffset)
{
    for (const auto &pandafileEntry : breakpoints_) {
        for (const auto &urlHashMapEntry : breakpoints_[pandafileEntry.first]) {
            for (auto it = breakpoints_[pandafileEntry.first][urlHashMapEntry.first].begin();
                 it != breakpoints_[pandafileEntry.first][urlHashMapEntry.first].end();) {
                const auto &bp = *it;
                if ((bp.GetBytecodeOffset() == bcOffset) &&
                    (bp.GetPtMethod()->GetJSPandaFile() == ptMethod->GetJSPandaFile()) &&
                    (bp.GetPtMethod()->GetMethodId() == ptMethod->GetMethodId())) {
                    it = breakpoints_[pandafileEntry.first][urlHashMapEntry.first].erase(it);
                } else {
                    it++;
                }
            }
        }
    }
    return true;
}

bool JSDebugger::RemoveSmartBreakpoint(const std::unique_ptr<PtMethod> &ptMethod, uint32_t bcOffset)
{
    for (auto it = smartBreakpoints_.begin(); it != smartBreakpoints_.end(); ++it) {
        const auto &bp = *it;
        if ((bp.GetBytecodeOffset() == bcOffset) &&
            (bp.GetPtMethod()->GetJSPandaFile() == ptMethod->GetJSPandaFile()) &&
            (bp.GetPtMethod()->GetMethodId() == ptMethod->GetMethodId())) {
            it = smartBreakpoints_.erase(it);
            return true;
        }
    }

    return false;
}

std::unique_ptr<PtMethod> JSDebugger::FindMethod(const JSPtLocation &location) const
{
    std::unique_ptr<PtMethod> ptMethod {nullptr};
    ::panda::ecmascript::JSPandaFileManager::GetInstance()->EnumerateJSPandaFiles([&ptMethod, location](
        const std::shared_ptr<JSPandaFile> &file) {
        if (file->GetJSPandaFileDesc() == location.GetJsPandaFile()->GetJSPandaFileDesc()) {
            MethodLiteral *methodsData = file->GetMethodLiterals();
            uint32_t numberMethods = file->GetNumMethods();
            for (uint32_t i = 0; i < numberMethods; ++i) {
                if (methodsData[i].GetMethodId() == location.GetMethodId()) {
                    MethodLiteral *methodLiteral = methodsData + i;
                    ptMethod = std::make_unique<PtMethod>(file.get(),
                        methodLiteral->GetMethodId(), methodLiteral->IsNativeWithCallField());
                    return false;
                }
            }
        }
        return true;
    });
    return ptMethod;
}

void JSDebugger::DumpBreakpoints()
{
    int32_t size = 0;
    LOG_DEBUGGER(DEBUG) << "Dumping breakpoints in local list:";
    for (const auto &entry : breakpoints_) {
        auto urlHashMap = breakpoints_[entry.first];
        for (const auto &pair : urlHashMap) {
            LOG_DEBUGGER(DEBUG) << "URL: " << pair.first;
            for (const auto &bp : urlHashMap[pair.first]) {
                size++;
                LOG_DEBUGGER(DEBUG) << "Local #" << size << ": " << bp.ToString();
            }
        }
    }
    LOG_DEBUGGER(INFO) << "Dumpped breakpoints in local list with size " << size;
}

void JSDebugger::DumpGlobalBreakpoints()
{
    // Should call this function after acquiring the lock of global list
    int32_t size = 0;
    LOG_DEBUGGER(DEBUG) << "Dumping breakpoints in global list:";
    for (const auto &entry : globalBpList_) {
        auto urlHashMap = globalBpList_[entry.first];
        for (const auto &pair : urlHashMap) {
            LOG_DEBUGGER(DEBUG) << "URL: " << pair.first;
            for (const auto &bp : urlHashMap[pair.first]) {
                size++;
                LOG_DEBUGGER(DEBUG) << "Global #" << size << ": " << bp.ToString();
            }
        }
    }
    LOG_DEBUGGER(INFO) << "Dumpped breakpoints in global list with size " << size;
}

bool JSDebugger::IsBreakpointCondSatisfied(std::optional<JSBreakpoint> breakpoint) const
{
    if (!breakpoint.has_value()) {
        return false;
    }
    JSThread *thread = ecmaVm_->GetJSThread();
    auto condFuncRef = breakpoint.value().GetConditionFunction();
    if (!condFuncRef->IsHole() && condFuncRef->IsFunction(ecmaVm_)) {
        LOG_DEBUGGER(INFO) << "BreakpointCondition: evaluating condition";
        auto handlerPtr = std::make_shared<FrameHandler>(ecmaVm_->GetJSThread());
        auto evalResult = DebuggerApi::EvaluateViaFuncCall(const_cast<EcmaVM *>(ecmaVm_),
            condFuncRef.ToLocal(ecmaVm_), handlerPtr);
        if (thread->HasPendingException()) {
            LOG_DEBUGGER(ERROR) << "BreakpointCondition: has pending exception";
            thread->ClearException();
            return false;
        }
        bool satisfied = evalResult->ToBoolean(ecmaVm_)->Value();
        if (!satisfied) {
            LOG_DEBUGGER(INFO) << "BreakpointCondition: condition not meet";
            return false;
        }
    }
    return true;
}

void JSDebugger::MethodEntry(JSHandle<Method> method, JSHandle<JSTaggedValue> envHandle)
{
    if (hooks_ == nullptr || !ecmaVm_->GetJsDebuggerManager()->IsDebugMode()) {
        return;
    }
    FrameHandler frameHandler(ecmaVm_->GetJSThread());
    if (frameHandler.IsEntryFrame() || frameHandler.IsBuiltinFrame()) {
        return;
    }
    auto *debuggerMgr = ecmaVm_->GetJsDebuggerManager();
    debuggerMgr->MethodEntry(method, envHandle);
}

void JSDebugger::MethodExit([[maybe_unused]] JSHandle<Method> method)
{
    if (hooks_ == nullptr || !ecmaVm_->GetJsDebuggerManager()->IsDebugMode()) {
        return;
    }
    FrameHandler frameHandler(ecmaVm_->GetJSThread());
    if (frameHandler.IsEntryFrame() || frameHandler.IsBuiltinFrame()) {
        return;
    }
    auto *debuggerMgr = ecmaVm_->GetJsDebuggerManager();
    debuggerMgr->MethodExit(method);
}
}  // namespace panda::tooling::ecmascript