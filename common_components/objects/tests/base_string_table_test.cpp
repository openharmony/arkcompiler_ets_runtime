/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "common_components/tests/test_helper.h"
#include "common_interfaces/objects/base_string_table.h"
#include "common_components/objects/string_table_internal.h"
#include "common_interfaces/thread/mutator_base.h"
#include "common_interfaces/objects/base_string.h"
#include "common_interfaces/thread/thread_holder.h"
#include "common_interfaces/base_runtime.h"
#include "common_interfaces/heap/heap_allocator.h"
#include "common_interfaces/objects/string/base_string-inl2.h"
   

namespace common {

struct DummyMutator : public MutatorBase {
    explicit DummyMutator(LanguageType lang) : lang_(lang) {}
    LanguageType lang_;
};

class BaseStringTableTest : public common::test::BaseTestWithScope {
protected:
    using TableType = BaseStringTableInternal<false>;
    BaseRuntime* runtime_;
    std::unique_ptr<MutatorBase> mutator_;
    std::unique_ptr<TableType> table_;
    ThreadHolder* threadHolder_;

    void SetUp() override
    {
        mutator_ = std::make_unique<DummyMutator>(LanguageType::DYNAMIC);
        threadHolder_ = new ThreadHolder(mutator_.get());

        runtime_ = BaseRuntime::GetInstance();
        ASSERT_TRUE(runtime_ != nullptr);

        runtime_->Init();

        table_ = std::make_unique<TableType>();
    }

    void TearDown() override
    {
        table_.reset();

        if (runtime_) {
            runtime_->Fini();
        }

        mutator_.reset();
        threadHolder_ = nullptr;
    }

    BaseString* CreateUtf8String(const char* utf8Data, uint32_t length, bool canBeCompress)
    {
        auto allocator = [](size_t size, CommonType type) -> BaseString* {
            void* mem = reinterpret_cast<void*>(HeapAllocator::AllocateInOldOrHuge(size, LanguageType::DYNAMIC));
            if (mem == nullptr) {
                return nullptr;
            }
            return reinterpret_cast<BaseString*>(mem);
        };
       
        BaseString* str = BaseString::CreateFromUtf8(allocator,
            reinterpret_cast<const uint8_t*>(utf8Data), length, canBeCompress);

        if (str == nullptr) {
            return nullptr;
        }
        return str;
    }

    static ReadOnlyHandle<BaseString> MockHandleCreator(ThreadHolder* holder, BaseString* str)
    {
        uintptr_t handleValue = reinterpret_cast<uintptr_t>(str);
        return ReadOnlyHandle<BaseString>(handleValue);
    }
};

HWTEST_F_L0(BaseStringTableTest, SweepWeakRef)
{
    WeakRefFieldVisitor mockVisitor = [](RefField<false>& field) {
        return true;
    };

    table_->GetHashTrieMap().StartSweeping();
    table_->SweepWeakRef(mockVisitor);
    table_->GetHashTrieMap().FinishSweeping();

    EXPECT_TRUE(true);
}

HWTEST_F_L0(BaseStringTableTest, CleanUp)
{
    table_->GetHashTrieMap().Clear();
    EXPECT_TRUE(true);
}

}