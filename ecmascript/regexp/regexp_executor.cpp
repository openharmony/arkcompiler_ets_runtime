/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/regexp/regexp_executor.h"

namespace panda::ecmascript {
using RegExpState = RegExpExecutor::RegExpState;
using RegExpGlobalResult = builtins::RegExpGlobalResult;
bool RegExpExecutor::Execute(const uint8_t *input, uint32_t lastIndex, uint32_t length, uint8_t *buf, bool isWideChar)
{
    DynChunk buffer(buf, chunk_);
    input_ = const_cast<uint8_t *>(input);
    inputEnd_ = const_cast<uint8_t *>(input + length * (isWideChar ? WIDE_CHAR_SIZE : CHAR_SIZE));
    uint32_t size = buffer.GetU32(0);
    nCapture_ = buffer.GetU32(RegExpParser::NUM_CAPTURE__OFFSET);
    nStack_ = buffer.GetU32(RegExpParser::NUM_STACK_OFFSET);
    flags_ = buffer.GetU32(RegExpParser::FLAGS_OFFSET);
    prefilter_ = buffer.GetU32(RegExpParser::PREFILTER_OFFSET);
    isWideChar_ = isWideChar;

    uint32_t captureResultSize = sizeof(CaptureState) * nCapture_;
    uint32_t stackSize = sizeof(uintptr_t) * nStack_;
    stateStackLen_ = 0;
    currentStack_ = 0;

    if (captureResultSize != 0) {
        if (captureResultList_ == nullptr) {
            captureResultList_ = chunk_->NewArray<CaptureState>(nCapture_);
        }
        if (memset_s(captureResultList_, captureResultSize, 0, captureResultSize) != EOK) {
            LOG_FULL(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
    }
    if (stackSize != 0 && stack_ == nullptr) {
        stack_ = chunk_->NewArray<uintptr_t>(nStack_);
        if (memset_s(stack_, stackSize, 0, stackSize) != EOK) {
            LOG_FULL(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    SetCurrentPtr(input + lastIndex * (isWideChar ? WIDE_CHAR_SIZE : CHAR_SIZE));
    SetCurrentPC(RegExpParser::OP_START_OFFSET);

    // first split
    if ((flags_ & RegExpParser::FLAG_STICKY) == 0) {
        PushRegExpState(STATE_SPLIT, RegExpParser::OP_START_OFFSET);
    }
    return ExecuteInternal(buffer, size);
}

bool RegExpExecutor::MatchFailed(bool isMatched)
{
    if (isMatched) {
        stateStackLen_ = 0;
        return true;
    }
    while (stateStackLen_ > 0) {
        // StateType::STATE_SPLIT or STATE_NEGATIVE_MATCH_AHEAD
        if (PopRegExpState() <= StateType::STATE_NEGATIVE_MATCH_AHEAD) {
            return false;
        }
    }
    return true;
}

// NOLINTNEXTLINE(readability-function-size)
bool RegExpExecutor::ExecuteInternal(const DynChunk &byteCode, uint32_t pcEnd)
{
    while (GetCurrentPC() < pcEnd) {
        // first split
        if (!HandleFirstSplit()) {
            return false;
        }
        uint8_t opCode = byteCode.GetU8(GetCurrentPC());
        switch (opCode) {
            case RegExpOpCode::OP_DOTS:
            case RegExpOpCode::OP_ALL: {
                if (!HandleOpAll(opCode)) {
                    return false;
                }
                break;
            }
            case RegExpOpCode::OP_CHAR32:
            case RegExpOpCode::OP_CHAR: {
                if (!HandleOpChar(byteCode, opCode)) {
                    return false;
                }
                break;
            }
            case RegExpOpCode::OP_NOT_WORD_BOUNDARY:
            case RegExpOpCode::OP_WORD_BOUNDARY: {
                if (!HandleOpWordBoundary(opCode)) {
                    return false;
                }
                break;
            }
            case RegExpOpCode::OP_LINE_START: {
                if (!HandleOpLineStart(opCode)) {
                    return false;
                }
                break;
            }
            case RegExpOpCode::OP_LINE_END: {
                if (!HandleOpLineEnd(opCode)) {
                    return false;
                }
                break;
            }
            case RegExpOpCode::OP_SAVE_START:
                HandleOpSaveStart(byteCode, opCode);
                break;
            case RegExpOpCode::OP_SAVE_END:
                HandleOpSaveEnd(byteCode, opCode);
                break;
            case RegExpOpCode::OP_GOTO: {
                uint32_t offset = byteCode.GetU32(GetCurrentPC() + 1);
                Advance(opCode, offset);
                break;
            }
            case RegExpOpCode::OP_MATCH: {
                ASSERT(stateStackLen_ > 0);
                // jump to match ahead
                uint32_t ahead = stateStackLen_ - 1;
                auto stateStack = reinterpret_cast<RegExpState *>(stateStack_);
                while (ahead != 0 && stateStack[ahead].type_ != StateType::STATE_MATCH_AHEAD &&
                    stateStack[ahead].type_ != StateType::STATE_NEGATIVE_MATCH_AHEAD) {
                    --ahead;
                }
                bool isNegative = stateStack[ahead].type_ == StateType::STATE_NEGATIVE_MATCH_AHEAD;
                while (stateStackLen_ > ahead) {
                    PopRegExpState(isNegative);
                }
                if (isNegative && MatchFailed(false)) {
                    return false;
                }
                break;
            }
            case RegExpOpCode::OP_MATCH_END:
                return true;
            case RegExpOpCode::OP_SAVE_RESET:
                HandleOpSaveReset(byteCode, opCode);
                break;
            case RegExpOpCode::OP_SPLIT_NEXT:
            case RegExpOpCode::OP_MATCH_AHEAD:
            case RegExpOpCode::OP_NEGATIVE_MATCH_AHEAD:
                HandleOpMatch(byteCode, opCode);
                break;
            case RegExpOpCode::OP_SPLIT_FIRST:
                HandleOpSplitFirst(byteCode, opCode);
                break;
            case RegExpOpCode::OP_PREV: {
                if (!HandleOpPrev(opCode)) {
                    return false;
                }
                break;
            }
            case RegExpOpCode::OP_LOOP_GREEDY:
            case RegExpOpCode::OP_LOOP:
                HandleOpLoop(byteCode, opCode);
                break;
            case RegExpOpCode::OP_PUSH_CHAR: {
                PushRegExpState(StateType::STATE_PUSH, 0, 0);
                PushStack(reinterpret_cast<uintptr_t>(GetCurrentPtr()));
                Advance(opCode);
                break;
            }
            case RegExpOpCode::OP_CHECK_CHAR: {
                if (stateStackLen_ > 0 && PeekRegExpState()->type_ == StateType::STATE_PUSH) {
                    DropRegExpState();
                } else {
                    ASSERT(currentStack_ > 0);
                    PushRegExpState(StateType::STATE_POP, 0, stack_[currentStack_ - 1]);
                }
                if (PopStack() != reinterpret_cast<uintptr_t>(GetCurrentPtr())) {
                    Advance(opCode);
                } else {
                    uint32_t offset = byteCode.GetU32(GetCurrentPC() + 1);
                    Advance(opCode, offset);
                }
                break;
            }
            case RegExpOpCode::OP_PUSH: {
                PushRegExpState(StateType::STATE_PUSH, 0, 0);
                PushStack(0);
                Advance(opCode);
                break;
            }
            case RegExpOpCode::OP_POP: {
                ASSERT(currentStack_ > 0);
                PushRegExpState(StateType::STATE_POP, 0, stack_[currentStack_ - 1]);
                PopStack();
                Advance(opCode);
                break;
            }
            case RegExpOpCode::OP_RANGE32: {
                if (!HandleOpRange32(byteCode)) {
                    return false;
                }
                break;
            }
            case RegExpOpCode::OP_RANGE: {
                if (!HandleOpRange(byteCode)) {
                    return false;
                }
                break;
            }
            case RegExpOpCode::OP_SPARSE: {
                if (!HandleOpSparse(byteCode)) {
                    return false;
                }
                break;
            }
            case RegExpOpCode::OP_BACKREFERENCE:
            case RegExpOpCode::OP_BACKWARD_BACKREFERENCE: {
                if (!HandleOpBackReference(byteCode, opCode)) {
                    return false;
                }
                break;
            }
            default:
                UNREACHABLE();
        }
    }
    // for loop match
    return true;
}

void RegExpExecutor::DumpResult(std::ostream &out) const
{
    out << "captures:" << std::endl;
    for (uint32_t i = 0; i < nCapture_; i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        CaptureState *captureState = &captureResultList_[i];
        int32_t len = captureState->captureEnd - captureState->captureStart;
        if ((captureState->captureStart != nullptr && captureState->captureEnd != nullptr) && (len >= 0)) {
            out << i << ":\t" << CString(reinterpret_cast<const char *>(captureState->captureStart), len) << std::endl;
        } else {
            out << i << ":\t"
                << "undefined" << std::endl;
        }
    }
}

void RegExpExecutor::GetResult(JSThread *thread)
{
    JSHandle<RegExpGlobalResult> matchResult(thread->GetCurrentEcmaContext()->GetRegExpGlobalResult());
    matchResult->SetTotalCaptureCounts(thread, JSTaggedValue(nCapture_));
    uint32_t firstIndex = RegExpGlobalResult::FIRST_CAPTURE_INDEX;
    uint32_t availableCaptureSlot = matchResult->GetLength() - firstIndex;
    uint32_t requiredLength =  nCapture_ * 2;
    if (requiredLength > availableCaptureSlot) {
        matchResult = RegExpGlobalResult::GrowCapturesCapacity(thread, matchResult, requiredLength + firstIndex);
    }
    for (uint32_t i = 0; i < nCapture_; i++) {
        CaptureState *captureState = &captureResultList_[i];
        int32_t len = captureState->captureEnd - captureState->captureStart;
        if ((captureState->captureStart != nullptr && captureState->captureEnd != nullptr) && (len >= 0)) {
            if (isWideChar_) {
                matchResult->SetStartOfCaptureIndex(thread, i, JSTaggedValue(
                    static_cast<int32_t>((captureState->captureStart - input_) / WIDE_CHAR_SIZE)));
                matchResult->SetEndOfCaptureIndex(thread, i, JSTaggedValue(
                    static_cast<int32_t>((captureState->captureEnd - input_) / WIDE_CHAR_SIZE)));
            } else {
                matchResult->SetStartOfCaptureIndex(thread, i, JSTaggedValue(
                    static_cast<int32_t>(captureState->captureStart - input_)));
                matchResult->SetEndOfCaptureIndex(thread, i, JSTaggedValue(
                    static_cast<int32_t>(captureState->captureEnd - input_)));
            }
        } else {
            // undefined
            matchResult->SetStartOfCaptureIndex(thread, i, JSTaggedValue(0));
            matchResult->SetEndOfCaptureIndex(thread, i, JSTaggedValue(-1));
        }
    }
    uint32_t endIndex = currentPtr_ - input_;
    if (isWideChar_) {
        endIndex /= WIDE_CHAR_SIZE;
    }
    matchResult->SetEndIndex(thread, JSTaggedValue(endIndex));
}

void RegExpExecutor::PushRegExpState(StateType type, uint32_t pc)
{
    ReAllocStack(stateStackLen_ + 1);
    auto state = reinterpret_cast<RegExpState *>(
        stateStack_ +  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        stateStackLen_ * sizeof(RegExpState));
    state->type_ = type;
    state->currentPc_ = pc;
    state->currentPtr_ = GetCurrentPtr();
    stateStackLen_++;
}

void RegExpExecutor::PushRegExpState(StateType type, uint32_t pc, uintptr_t ptr)
{
    ReAllocStack(stateStackLen_ + 1);
    auto state = reinterpret_cast<RegExpState *>(
        stateStack_ + // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        stateStackLen_ * sizeof(RegExpState));
    state->type_ = type;
    state->currentPc_ = pc;
    state->currentPtr_ = reinterpret_cast<const uint8_t *>(ptr);
    stateStackLen_++;
}

RegExpExecutor::StateType RegExpExecutor::PopRegExpState(bool copyCapture)
{
    if (stateStackLen_ != 0) {
        auto state = PeekRegExpState();
        stateStackLen_--;
        switch (state->type_) {
            case StateType::STATE_SPLIT:
            case StateType::STATE_NEGATIVE_MATCH_AHEAD:
            case StateType::STATE_MATCH_AHEAD:
                SetCurrentPC(state->currentPc_);
                SetCurrentPtr(state->currentPtr_);
                break;
            case StateType::STATE_SAVE:
                if (copyCapture) {
                    *(reinterpret_cast<const uint8_t **>(GetCaptureResultList()) + state->currentPc_) =
                        state->currentPtr_;
                }
                break;
            case StateType::STATE_PUSH:
                PopStack();
                break;
            case StateType::STATE_POP:
                PushStack((uintptr_t)state->currentPtr_);
                break;
            case StateType::STATE_SET:
                SetStackValue((uintptr_t)state->currentPtr_);
                break;
            default:
                UNREACHABLE();
                break;
        }
        return state->type_;
    }
    return StateType::STATE_INVALID;
}

void RegExpExecutor::ReAllocStack(uint32_t stackLen)
{
    if (stackLen > stateStackSize_) {
        ASSERT((static_cast<size_t>(stateStackSize_) * 2) <= static_cast<size_t>(UINT32_MAX)); // 2: double the size
        uint32_t newStackSize = std::max(stateStackSize_ * 2, MIN_STACK_SIZE);  // 2: double the size
        ASSERT((static_cast<size_t>(newStackSize) * static_cast<size_t>(sizeof(RegExpState))) <=
            static_cast<size_t>(UINT32_MAX));
        uint32_t stackByteSize = newStackSize * sizeof(RegExpState);
        auto newStack = chunk_->NewArray<uint8_t>(stackByteSize);
        if (memset_s(newStack, stackByteSize, 0, stackByteSize) != EOK) {
            LOG_FULL(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
        if (stateStack_ != nullptr) {
            auto stackSize = stateStackSize_ * sizeof(RegExpState);
            if (memcpy_s(newStack, stackSize, stateStack_, stackSize) != EOK) {
                return;
            }
        }
        stateStack_ = newStack;
        stateStackSize_ = newStackSize;
    }
}
}  // namespace panda::ecmascript
