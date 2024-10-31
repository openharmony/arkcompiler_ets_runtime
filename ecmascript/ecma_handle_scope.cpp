/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/ecma_handle_scope.h"

#include "ecmascript/ecma_context.h"

namespace panda::ecmascript {
EcmaHandleScope::EcmaHandleScope(JSThread *thread) : thread_(thread)
{
    auto context = thread_->GetCurrentEcmaContext();
    OpenHandleScope(context);
    OpenPrimitiveScope(context);
}

void EcmaHandleScope::OpenHandleScope(EcmaContext *context)
{
    prevNext_ = context->handleScopeStorageNext_;
    prevEnd_ = context->handleScopeStorageEnd_;
    prevHandleStorageIndex_ = context->currentHandleStorageIndex_;
#ifdef ECMASCRIPT_ENABLE_HANDLE_LEAK_CHECK
    context->HandleScopeCountAdd();
    prevHandleScope_ = context->GetLastHandleScope();
    context->SetLastHandleScope(this);
#endif
}

void EcmaHandleScope::OpenPrimitiveScope(EcmaContext *context)
{
    prevPrimitiveNext_ = context->primitiveScopeStorageNext_;
    prevPrimitiveEnd_ = context->primitiveScopeStorageEnd_;
    prevPrimitiveStorageIndex_ = context->currentPrimitiveStorageIndex_;
#ifdef ECMASCRIPT_ENABLE_HANDLE_LEAK_CHECK
    context->PrimitiveScopeCountAdd();
    prevPrimitiveScope_ = context->GetLastPrimitiveScope();
    context->SetLastPrimitiveScope(this);
#endif
}

EcmaHandleScope::~EcmaHandleScope()
{
    auto context = thread_->GetCurrentEcmaContext();
    CloseHandleScope(context);
    ClosePrimitiveScope(context);
}

void EcmaHandleScope::CloseHandleScope(EcmaContext *context)
{
#ifdef ECMASCRIPT_ENABLE_HANDLE_LEAK_CHECK
    context->HandleScopeCountDec();
    context->SetLastHandleScope(prevHandleScope_);
    prevHandleScope_ = nullptr;
#endif
    context->handleScopeStorageNext_ = prevNext_;
    if (context->handleScopeStorageEnd_ != prevEnd_) {
        context->handleScopeStorageEnd_ = prevEnd_;
        context->ShrinkHandleStorage(prevHandleStorageIndex_);
    }
}

void EcmaHandleScope::ClosePrimitiveScope(EcmaContext *context)
{
#ifdef ECMASCRIPT_ENABLE_HANDLE_LEAK_CHECK
    context->PrimitiveScopeCountDec();
    context->SetLastPrimitiveScope(prevPrimitiveScope_);
    prevPrimitiveScope_ = nullptr;
#endif
    context->primitiveScopeStorageNext_ = prevPrimitiveNext_;
    if (context->primitiveScopeStorageEnd_ != prevPrimitiveEnd_) {
        context->primitiveScopeStorageEnd_ = prevPrimitiveEnd_;
        context->ShrinkPrimitiveStorage(prevPrimitiveStorageIndex_);
    }
}

uintptr_t EcmaHandleScope::NewHandle(JSThread *thread, JSTaggedType value)
{
    CHECK_NO_HANDLE_ALLOC;
#if ECMASCRIPT_ENABLE_THREAD_STATE_CHECK
    if (UNLIKELY(!thread->IsInRunningStateOrProfiling())) {
        LOG_ECMA(FATAL) << "New handle must be in jsthread running state";
        UNREACHABLE();
    }
#endif
    // Handle is a kind of GC_ROOT, and should only directly hold Obejct or Primitive, not a weak reference.
    ASSERT(!JSTaggedValue(value).IsWeak());
    auto context = thread->GetCurrentEcmaContext();
#ifdef ECMASCRIPT_ENABLE_HANDLE_LEAK_CHECK
    // Each Handle must be managed by HandleScope, otherwise it may cause Handle leakage.
    if (context->handleScopeCount_ <= 0) {
        LOG_ECMA(ERROR) << "New handle must be in handlescope" << context->handleScopeCount_;
    }
    static const long MAYBE_HANDLE_LEAK_TIME_MS = 5000;
    if (context->GetLastHandleScope() != nullptr) {
        float totalSpentTime = context->GetLastHandleScope()->scope_.TotalSpentTime();
        if (totalSpentTime >= MAYBE_HANDLE_LEAK_TIME_MS) {
            LOG_ECMA(INFO) << "New handle in scope count:" << context->handleScopeCount_
                            << ", time:" << totalSpentTime << "ms";
            std::ostringstream stack;
            Backtrace(stack);
            LOG_ECMA(INFO) << stack.str();
        }
    }
#endif
    auto result = context->handleScopeStorageNext_;
    if (result == context->handleScopeStorageEnd_) {
        result = reinterpret_cast<JSTaggedType *>(context->ExpandHandleStorage());
    }
#if ECMASCRIPT_ENABLE_NEW_HANDLE_CHECK
    thread->CheckJSTaggedType(value);
    if (result == nullptr) {
        LOG_ECMA(ERROR) << "result is nullptr, New handle fail!";
        return 0U;
    }
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    context->handleScopeStorageNext_ = result + 1;
    *result = value;
    return reinterpret_cast<uintptr_t>(result);
}

uintptr_t EcmaHandleScope::NewPrimitiveHandle(JSThread *thread, JSTaggedType value)
{
    CHECK_NO_HANDLE_ALLOC;
    auto context = thread->GetCurrentEcmaContext();
#ifdef ECMASCRIPT_ENABLE_HANDLE_LEAK_CHECK
    // Each PrimitiveHandle must be managed by PrimitiveScope, otherwise it may cause Handle leakage.
    if (context->primitiveScopeCount_ <= 0) {
        LOG_ECMA(ERROR) << "New primitive must be in primitivescope" << context->primitiveScopeCount_;
    }
    static const long MAYBE_PRIMITIVE_LEAK_TIME_MS = 5000;
    if (context->GetLastPrimitiveScope() != nullptr) {
        float totalSpentTime = context->GetLastPrimitiveScope()->scope_.TotalSpentTime();
        if (totalSpentTime >= MAYBE_PRIMITIVE_LEAK_TIME_MS) {
            LOG_ECMA(INFO) << "New primitiveHandle in scope count:" << context->primitiveScopeCount_
                            << ", time:" << totalSpentTime << "ms";
            std::ostringstream stack;
            Backtrace(stack);
            LOG_ECMA(INFO) << stack.str();
        }
    }
#endif
    auto result = context->primitiveScopeStorageNext_;
    if (result == context->primitiveScopeStorageEnd_) {
        result = reinterpret_cast<JSTaggedType *>(context->ExpandPrimitiveStorage());
    }
#if ECMASCRIPT_ENABLE_NEW_HANDLE_CHECK
    thread->CheckJSTaggedType(value);
    if (result == nullptr) {
        LOG_ECMA(ERROR) << "result is nullptr, New primitiveHandle fail!";
        return 0U;
    }
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    context->primitiveScopeStorageNext_ = result + 1;
    *result = value;
    return reinterpret_cast<uintptr_t>(result);
}
}  // namespace panda::ecmascript