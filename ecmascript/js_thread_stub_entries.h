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

#ifndef ECMASCRIPT_JS_THREAD_STUB_ENTRIES_H
#define ECMASCRIPT_JS_THREAD_STUB_ENTRIES_H

#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/compiler/common_stub_csigns.h"
#include "ecmascript/compiler/baseline/baseline_stub_csigns.h"
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/rt_call_signature.h"

namespace panda::ecmascript {
struct BCStubEntries {
    static constexpr size_t EXISTING_BC_HANDLER_STUB_ENTRIES_COUNT =
        kungfu::BytecodeStubCSigns::NUM_OF_ALL_NORMAL_STUBS;
    // The number of bytecodes.
    static constexpr size_t BC_HANDLER_COUNT = kungfu::BytecodeStubCSigns::LAST_VALID_OPCODE + 1;
    static constexpr size_t COUNT = kungfu::BytecodeStubCSigns::NUM_OF_STUBS;
    static_assert(EXISTING_BC_HANDLER_STUB_ENTRIES_COUNT <= COUNT);
    Address stubEntries_[COUNT] = {0};

    static constexpr size_t SizeArch32 = sizeof(uint32_t) * COUNT;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * COUNT;

    void Set(size_t index, Address addr)
    {
        ASSERT(index < COUNT);
        stubEntries_[index] = addr;
    }

    Address* GetAddr()
    {
        return reinterpret_cast<Address*>(stubEntries_);
    }

    Address Get(size_t index) const
    {
        ASSERT(index < COUNT);
        return stubEntries_[index];
    }
};
STATIC_ASSERT_EQ_ARCH(sizeof(BCStubEntries), BCStubEntries::SizeArch32, BCStubEntries::SizeArch64);

struct RTStubEntries {
    static constexpr size_t COUNT = kungfu::RuntimeStubCSigns::NUM_OF_STUBS;
    Address stubEntries_[COUNT];

    static constexpr size_t SizeArch32 = sizeof(uint32_t) * COUNT;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * COUNT;

    void Set(size_t index, Address addr)
    {
        ASSERT(index < COUNT);
        stubEntries_[index] = addr;
    }

    Address Get(size_t index) const
    {
        ASSERT(index < COUNT);
        return stubEntries_[index];
    }
};
STATIC_ASSERT_EQ_ARCH(sizeof(RTStubEntries), RTStubEntries::SizeArch32, RTStubEntries::SizeArch64);

struct COStubEntries {
    static constexpr size_t COUNT = kungfu::CommonStubCSigns::NUM_OF_STUBS;
    Address stubEntries_[COUNT];

    static constexpr size_t SizeArch32 = sizeof(uint32_t) * COUNT;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * COUNT;

    void Set(size_t index, Address addr)
    {
        ASSERT(index < COUNT);
        stubEntries_[index] = addr;
    }

    Address Get(size_t index) const
    {
        ASSERT(index < COUNT);
        return stubEntries_[index];
    }
};

struct BCDebuggerStubEntries {
    static constexpr size_t EXISTING_BC_HANDLER_STUB_ENTRIES_COUNT =
        kungfu::BytecodeStubCSigns::NUM_OF_ALL_NORMAL_STUBS;
    static constexpr size_t COUNT = kungfu::BytecodeStubCSigns::LAST_VALID_OPCODE + 1;
    Address stubEntries_[COUNT];

    static constexpr size_t SizeArch32 = sizeof(uint32_t) * COUNT;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * COUNT;

    void Set(size_t index, Address addr)
    {
        ASSERT(index < COUNT);
        stubEntries_[index] = addr;
    }

    Address Get(size_t index) const
    {
        ASSERT(index < COUNT);
        return stubEntries_[index];
    }

    void SetNonexistentBCHandlerStubEntries(Address addr)
    {
        for (size_t i = EXISTING_BC_HANDLER_STUB_ENTRIES_COUNT; i < COUNT; i++) {
            if (stubEntries_[i] == 0) {
                stubEntries_[i] = addr;
            }
        }
    }
};

struct BuiltinStubEntries {
    static constexpr size_t COUNT = kungfu::BuiltinsStubCSigns::NUM_OF_BUILTINS_STUBS;
    Address stubEntries_[COUNT];

    static constexpr size_t SizeArch32 = sizeof(uint32_t) * COUNT;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * COUNT;

    void Set(size_t index, Address addr)
    {
        ASSERT(index < COUNT);
        stubEntries_[index] = addr;
    }

    Address Get(size_t index) const
    {
        ASSERT(index < COUNT);
        return stubEntries_[index];
    }
};

struct BaselineStubEntries {
    static constexpr size_t COUNT = kungfu::BaselineStubCSigns::NUM_OF_STUBS;
    Address stubEntries_[COUNT];

    static constexpr size_t SizeArch32 = sizeof(uint32_t) * COUNT;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * COUNT;

    void Set(size_t index, Address addr)
    {
        ASSERT(index < COUNT);
        stubEntries_[index] = addr;
    }

    Address Get(size_t index) const
    {
        ASSERT(index < COUNT);
        return stubEntries_[index];
    }
};

STATIC_ASSERT_EQ_ARCH(sizeof(COStubEntries), COStubEntries::SizeArch32, COStubEntries::SizeArch64);
} // namespace panda::ecmascript

#endif // ECMASCRIPT_JS_THREAD_STUB_ENTRIES_H
