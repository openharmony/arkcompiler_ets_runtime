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

#include "ecmascript/waiter_list.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class WaiterListTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

static WaiterListNode *CreateListNode(JSThread *thread, uint32_t bufferLength)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    WaiterListNode *listNode = new WaiterListNode();
    void *toBuffer = ecmaVm->GetNativeAreaAllocator()->AllocateBuffer(bufferLength);
    EXPECT_TRUE(toBuffer != nullptr && listNode != nullptr);
    listNode->date_ = toBuffer;
    listNode->index_ = bufferLength;
    listNode->waitPointer_ = reinterpret_cast<int8_t *>(toBuffer) + bufferLength;
    listNode->waiting_ = true;
    listNode->prev_ = nullptr;
    listNode->next_ = nullptr;
    return listNode;
}

static void DeleteListNode(JSThread *thread, WaiterListNode *listNode)
{
    if (listNode != nullptr) {
        thread->GetEcmaVM()->GetNativeAreaAllocator()->Delete(listNode->date_);
        delete listNode;
        listNode = nullptr;
    }
    return;
}

HWTEST_F_L0(WaiterListTest, CreateWaiterList)
{
    WaiterList *waitLists = Singleton<WaiterList>::GetInstance();
    EXPECT_TRUE(waitLists != nullptr);
}

HWTEST_F_L0(WaiterListTest, AddNode)
{
    uint32_t bufferLength = 5;
    WaiterList *waitLists = Singleton<WaiterList>::GetInstance();
    WaiterListNode *listNode = CreateListNode(thread, bufferLength);
    // add the listNode to waitlists
    waitLists->AddNode(listNode);
    auto indexOneIter = waitLists->locationListMap_.find(listNode->waitPointer_);
    EXPECT_EQ(indexOneIter->second.pTail, listNode);
    EXPECT_EQ(indexOneIter->second.pHead, listNode);
    // change listNode property and add
    listNode->waiting_ = false;
    waitLists->AddNode(listNode);
    auto indexOneIter1 = waitLists->locationListMap_.find(listNode->waitPointer_);
    EXPECT_EQ(indexOneIter1->second.pTail, listNode);
    EXPECT_EQ(indexOneIter1->second.pTail->next_, listNode);
    EXPECT_EQ(indexOneIter1->second.pHead, listNode);
    EXPECT_EQ(listNode->prev_, listNode);
    EXPECT_EQ(listNode->next_, listNode);
    EXPECT_EQ(listNode->prev_->waiting_, false);
    DeleteListNode(thread, listNode);
}

HWTEST_F_L0(WaiterListTest, DeleteNode)
{
    uint32_t bufferLength = 5;
    WaiterList *waitLists = Singleton<WaiterList>::GetInstance();

    WaiterListNode *listNode = CreateListNode(thread, bufferLength);
    WaiterListNode *listNode1 = CreateListNode(thread, bufferLength + 1);
    waitLists->AddNode(listNode);
    waitLists->AddNode(listNode1);
    // change listNode waiting
    listNode1->waiting_ = false;
    waitLists->AddNode(listNode1);
    // delete listNode1
    waitLists->DeleteNode(listNode1);
    auto indexOneIter = waitLists->locationListMap_.find(listNode1->waitPointer_);
    EXPECT_EQ(indexOneIter, waitLists->locationListMap_.end());
    // delete listNode
    waitLists->DeleteNode(listNode);
    indexOneIter = waitLists->locationListMap_.find(listNode->waitPointer_);
    EXPECT_EQ(indexOneIter, waitLists->locationListMap_.end());

    DeleteListNode(thread, listNode);
    DeleteListNode(thread, listNode1);
}
}  // namespace panda::test
