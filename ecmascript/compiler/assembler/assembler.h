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
#ifndef ECMASCRIPT_COMPILER_ASSEMBLER_H
#define ECMASCRIPT_COMPILER_ASSEMBLER_H

#include "ecmascript/mem/dyn_chunk.h"
#ifdef JIT_ENABLE_CODE_SIGN
#include "ecmascript/compiler/jit_signcode.h"
#include "jit_buffer_integrity.h"
#endif

namespace panda::ecmascript {
#ifdef JIT_ENABLE_CODE_SIGN
using namespace OHOS::Security::CodeSign;
#endif
enum class Triple {
    TRIPLE_AMD64,
    TRIPLE_AARCH64,
    TRIPLE_ARM32,
};

#define TARGET_X64 "x86_64-unknown-linux-gnu"
#define TARGET_AARCH64 "aarch64-unknown-linux-gnu"
#define TARGET_ARM32 "arm-unknown-linux-gnu"

class GCStackMapRegisters {
public:
#if defined(PANDA_TARGET_AMD64)
    static constexpr int SP = 7;  /* x7 */
    static constexpr int FP = 6;  /* x6 */
#elif defined(PANDA_TARGET_ARM64)
    static constexpr int SP = 31;  /* x31 */
    static constexpr int FP = 29;  /* x29 */
#elif defined(PANDA_TARGET_ARM32)
    static constexpr int SP = 13;  /* x13 */
    static constexpr int FP = 11;  /* x11 */
#else
    static constexpr int SP = -1;
    static constexpr int FP = -1;
#endif

static int GetFpRegByTriple(Triple triple)
{
    int fp = -1;
    switch (triple) {
        case Triple::TRIPLE_AMD64:
            fp = 6;  /* x6 */
            break;
        case Triple::TRIPLE_ARM32:
            fp = 11;  /* x11 */
            break;
        case Triple::TRIPLE_AARCH64:
            fp = 29;  /* x29 */
            break;
        default:
            UNREACHABLE();
            break;
    }
    return fp;
}

static int GetSpRegByTriple(Triple triple)
{
    int sp = -1;
    switch (triple) {
        case Triple::TRIPLE_AMD64:
            sp = 7;  /* x7 */
            break;
        case Triple::TRIPLE_ARM32:
            sp = 13;  /* x13 */
            break;
        case Triple::TRIPLE_AARCH64:
            sp = 31;  /* x31 */
            break;
        default:
            UNREACHABLE();
            break;
    }
    return sp;
}
};

enum Distance {
    Near,
    Far
};

// When run from cpp to assembly code, there are some insts before the assembly frame is ready.
// When return from assembly code to cpp, there are some insts after the assembly frame is broken.
// And here are the numbers of insts. Only AsmInterpreterEntryFrame is dealt here, and there is no need
// for OptimizedEntryFrame because insts for OptimizedEntryFrame are negligible.
enum FrameCompletionPos : uint64_t {
    // X64
    X64CppToAsmInterp = 28,
    X64AsmInterpToCpp = 9,
    X64EntryFrameDuration = 70,
    // ARM64
    ARM64CppToAsmInterp = 56,
    ARM64AsmInterpToCpp = 40,
    ARM64EntryFrameDuration = 116,
};

class Label {
public:
    Label() = default;

    ~Label() = default;

    bool IsBound() const
    {
        return pos_ > 0;
    }

    bool IsLinked() const
    {
        return pos_ < 0;
    }

    bool IsLinkedNear() const
    {
        return nearPos_ > 0;
    }

    uint32_t GetPos() const
    {
        return static_cast<uint32_t>(pos_ - 1);
    }

    uint32_t GetLinkedPos() const
    {
        ASSERT(!IsBound());
        return static_cast<uint32_t>(-pos_ - 1);
    }

    void BindTo(int32_t pos)
    {
        // +1 skip offset 0
        pos_ = pos + 1;
    }

    void LinkTo(int32_t pos)
    {
        // +1 skip offset 0
        pos_ = - (pos + 1);
    }

    void UnlinkNearPos()
    {
        nearPos_ = 0;
    }

    void LinkNearPos(uint32_t pos)
    {
        // +1 skip offset 0
        nearPos_ = pos + 1;
    }

    uint32_t GetLinkedNearPos() const
    {
        ASSERT(!IsBound());
        return static_cast<uint32_t>(nearPos_ - 1);
    }

private:
    int32_t pos_ = 0;
    uint32_t nearPos_ = 0;
};

class Assembler {
public:
    explicit Assembler(Chunk *chunk)
        : buffer_(chunk) {}
    ~Assembler() = default;

    void EmitU8(uint8_t v)
    {
        buffer_.EmitChar(v);
    }

    void EmitI8(int8_t v)
    {
        buffer_.EmitChar(static_cast<uint8_t>(v));
    }

    void EmitU16(uint16_t v)
    {
        buffer_.EmitU16(v);
    }

    ARK_INLINE void EmitU32(uint32_t v)
    {
        buffer_.EmitU32(v);
#ifdef JIT_ENABLE_CODE_SIGN
        if (doCodeSign && kungfu::JitSignCode::GetInstance()->GetCodeSigner() != nullptr) {
            int err = AppendData(kungfu::JitSignCode::GetInstance()->GetCodeSigner(),
                                 reinterpret_cast<const uint8_t *>(&v), sizeof(v));
            if (err != 0) {
                LOG_BASELINEJIT(ERROR) << "Baseline AppendData failed, err = " << std::hex << err;
            }
        }
#endif
    }

    void EmitI32(int32_t v)
    {
        buffer_.EmitU32(static_cast<uint32_t>(v));
    }

    void EmitU64(uint64_t v)
    {
        buffer_.EmitU64(v);
    }

    void PutI8(size_t offset, int8_t data)
    {
        buffer_.PutU8(offset, static_cast<int8_t>(data));
    }

    ARK_INLINE void PutI32(size_t offset, int32_t data)
    {
        buffer_.PutU32(offset, static_cast<int32_t>(data));
#ifdef JIT_ENABLE_CODE_SIGN
        if (doCodeSign && kungfu::JitSignCode::GetInstance()->GetCodeSigner() != nullptr) {
            int err = PatchData(kungfu::JitSignCode::GetInstance()->GetCodeSigner(),
                                offset, reinterpret_cast<const uint8_t *>(&data), sizeof(data));
            if (err != 0) {
                LOG_BASELINEJIT(ERROR) << "Baseline PatchData failed, err = " << std::hex << err;
            }
        }
#endif
    }

    uint32_t GetU32(size_t offset) const
    {
        return buffer_.GetU32(offset);
    }

    int8_t GetI8(size_t offset) const
    {
        return static_cast<int8_t>(buffer_.GetU8(offset));
    }

    uint8_t GetU8(size_t offset) const
    {
        return buffer_.GetU8(offset);
    }

    size_t GetCurrentPosition() const
    {
        return buffer_.GetSize();
    }

    uint8_t *GetBegin() const
    {
        return buffer_.GetBegin();
    }

    static bool InRangeN(int32_t x, uint32_t n)
    {
        int32_t limit = 1 << (n - 1);
        return (x >= -limit) && (x < limit);
    }

    static bool InRange8(int32_t x)
    {
        // 8: range8
        return InRangeN(x, 8);
    }

    static void GetFrameCompletionPos(uint64_t &headerSize, uint64_t &tailSize, uint64_t &entryDuration)
    {
#if defined(PANDA_TARGET_AMD64)
        headerSize = FrameCompletionPos::X64CppToAsmInterp;
        tailSize = FrameCompletionPos::X64AsmInterpToCpp;
        entryDuration = FrameCompletionPos::X64EntryFrameDuration;
#elif defined(PANDA_TARGET_ARM64)
        headerSize = FrameCompletionPos::ARM64CppToAsmInterp;
        tailSize = FrameCompletionPos::ARM64AsmInterpToCpp;
        entryDuration = FrameCompletionPos::ARM64EntryFrameDuration;
#else
        headerSize = 0;
        tailSize = 0;
        entryDuration = 0;
        LOG_ECMA(FATAL) << "Assembler does not currently support other platforms, please run on x64 and arm64";
#endif
    }

    void SetDoCodeSign()
    {
        doCodeSign = true;
    }

private:
    DynChunk buffer_;
    bool doCodeSign = false;
};
}  // panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_ASSEMBLER_H
