/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ECMA_GLOBAL_STORAGE_H
#define ECMASCRIPT_ECMA_GLOBAL_STORAGE_H

#include "ecmascript/cross_vm/ecma_global_storage_hybrid.h"

#include "ecmascript/js_thread.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/platform/backtrace.h"
#ifdef HOOK_ENABLE
#include "memory_trace.h"
#endif

namespace panda::ecmascript {
class Node {
public:
    JSTaggedType GetObject() const
    {
        return obj_;
    }

    void SetObject(JSTaggedType obj)
    {
        obj_ = obj;
    }

    Node *GetNext() const
    {
        return next_;
    }

    void SetNext(Node *node)
    {
        next_ = node;
    }

    Node *GetPrev() const
    {
        return prev_;
    }

    void SetPrev(Node *node)
    {
        prev_ = node;
    }

    uint32_t GetIndex() const
    {
        return index_;
    }

    void SetIndex(uint32_t index)
    {
        index_ = index;
    }

    void SetUsing(bool isUsing)
    {
        isUsing_ = isUsing;
    }

    void SetWeak(bool isWeak)
    {
        isWeak_ = isWeak;
    }

    bool IsUsing() const
    {
        return isUsing_;
    }

    bool IsWeak() const
    {
        return isWeak_;
    }

    uintptr_t GetObjectAddress() const
    {
        return reinterpret_cast<uintptr_t>(&obj_);
    }

    // If isUsing is true, it means that the node is being used, otherwise it means that node is be freed
    void Reset([[maybe_unused]] JSThread *thread, Node *next, JSTaggedType value, bool isUsing)
    {
        SetPrev(nullptr);
        SetNext(next);
        SetObject(value);
        SetUsing(isUsing);
#ifdef HOOK_ENABLE
        memtrace((void *)next, sizeof(Node), "ArkJsGlobalHandle", isUsing);
#endif
    }

private:
    JSTaggedType obj_;
    Node *next_ {nullptr};
    Node *prev_ {nullptr};
    uint32_t index_ {0};
    bool isUsing_ {false};
    bool isWeak_ {false};
};

class DebugNode : public Node {
public:
    void Reset(JSThread *thread, Node *next, JSTaggedType value, bool isUsing)
    {
        Node::Reset(thread, next, value, isUsing);
        ResetDebugInfo();
        if (isUsing && thread->IsStartGlobalLeakCheck()) {
            if (JSTaggedValue(value).IsHeapObject()) {
                if (thread->EnableGlobalObjectLeakCheck()) {
                    SaveBacktrace(thread, value);
                }
            } else {
                if (thread->EnableGlobalPrimitiveLeakCheck()) {
                    SaveBacktrace(thread, value);
                }
            }
        }
    }

    int32_t GetMarkCount() const
    {
        return markCount_;
    }

    void MarkCount()
    {
        markCount_++;
    }

    void ResetDebugInfo()
    {
        markCount_ = 0;
        globalNumber_ = -1;
    }

    int32_t GetGlobalNumber()
    {
        return globalNumber_;
    }

private:
    void SaveBacktrace(JSThread *thread, JSTaggedType value)
    {
        globalNumber_ = static_cast<int32_t>(thread->IncreaseGlobalNumberCount());
        std::ostringstream stack;
        stack << "Global Handle Number:[" << globalNumber_ << "], value:0x" <<
            std::hex << value << std::endl;
        Backtrace(stack, true);
        thread->WriteToStackTraceFd(stack);
    }

    int32_t markCount_ {0};
    // A number generated in the order of distribution.It Used to help locate global memory leaks.
    int32_t globalNumber_ {-1};
};

class WeakNode : public Node {
public:
    void SetReference(void *ref)
    {
        reference_ = ref;
    }

    void* GetReference() const
    {
        return reference_;
    }

    void SetFreeGlobalCallback(WeakClearCallback callback)
    {
        freeGlobalCallback_ = callback;
    }

    void SetNativeFinalizeCallback(WeakClearCallback callback)
    {
        nativeFinalizeCallback_ = callback;
    }

    WeakClearCallback GetNativeFinalizeCallback() const
    {
        return nativeFinalizeCallback_;
    }

    WeakClearCallback GetFreeGlobalCallback() const
    {
        return freeGlobalCallback_;
    }

    void CallFreeGlobalCallback()
    {
        if (freeGlobalCallback_ != nullptr) {
            freeGlobalCallback_(reference_);
        }
    }

    void CallNativeFinalizeCallback()
    {
        if (nativeFinalizeCallback_ != nullptr) {
            nativeFinalizeCallback_(reference_);
        }
    }
private:
    void *reference_ {nullptr};
    WeakClearCallback freeGlobalCallback_ {nullptr};
    WeakClearCallback nativeFinalizeCallback_ {nullptr};
};

template<typename T>
class NodeList {
public:
    NodeList()
    {
        bool isWeak = std::is_same<T, WeakNode>::value;
        for (uint32_t i = 0; i < GLOBAL_BLOCK_SIZE; i++) {
            nodeList_[i].SetIndex(i);
            nodeList_[i].SetWeak(isWeak);
        }
    }
    ~NodeList() = default;

    inline static NodeList<T> *NodeToNodeList(T *node)
    {
        uintptr_t ptr = ToUintPtr(node) - node->GetIndex() * sizeof(T);
        return reinterpret_cast<NodeList<T> *>(ptr);
    }

    inline T *NewNode(JSThread *thread, JSTaggedType value)
    {
        if (IsFull()) {
            return nullptr;
        }
        T *node = &nodeList_[index_++];
        node->Reset(thread, usedList_, value, true);
        if (usedList_ != nullptr) {
            usedList_->SetPrev(node);
        }

        usedList_ = node;
        return node;
    }

    inline T *GetFreeNode(JSThread *thread, JSTaggedType value)
    {
        T *node = freeList_;
        if (node != nullptr) {
            freeList_ = reinterpret_cast<T *>(node->GetNext());
            node->Reset(thread, usedList_, value, true);
            if (usedList_ != nullptr) {
                usedList_->SetPrev(node);
            }
            usedList_ = node;
        }
        return node;
    }

    inline void FreeNode(JSThread *thread, T *node)
    {
        if (node->GetPrev() != nullptr) {
            node->GetPrev()->SetNext(node->GetNext());
        }
        if (node->GetNext() != nullptr) {
            node->GetNext()->SetPrev(node->GetPrev());
        }
        if (node == usedList_) {
            usedList_ = reinterpret_cast<T *>(node->GetNext());
        }
        node->Reset(thread, freeList_, JSTaggedValue::Undefined().GetRawData(), false);
        if (node->IsWeak()) {
            reinterpret_cast<WeakNode *>(node)->SetReference(nullptr);
            reinterpret_cast<WeakNode *>(node)->SetFreeGlobalCallback(nullptr);
            reinterpret_cast<WeakNode *>(node)->SetNativeFinalizeCallback(nullptr);
        }
        if (freeList_ != nullptr) {
            freeList_->SetPrev(node);
        }
        freeList_ = node;
    }

    inline void LinkTo(NodeList<T> *prev)
    {
        next_ = nullptr;
        prev_ = prev;
        prev_->next_ = this;
    }
    inline void RemoveList()
    {
        if (next_ != nullptr) {
            next_->SetPrev(prev_);
        }
        if (prev_ != nullptr) {
            prev_->SetNext(next_);
        }
        if (freeNext_ != nullptr) {
            freeNext_->SetFreePrev(freePrev_);
        }
        if (freePrev_ != nullptr) {
            freePrev_->SetFreeNext(freeNext_);
        }
    }

    inline bool IsFull()
    {
        return index_ >= GLOBAL_BLOCK_SIZE;
    }

    inline bool HasFreeNode()
    {
        return freeList_ != nullptr;
    }

    inline bool HasUsagedNode()
    {
        return !IsFull() || usedList_ != nullptr;
    }

    inline void SetNext(NodeList *next)
    {
        next_ = next;
    }
    inline NodeList<T> *GetNext() const
    {
        return next_;
    }

    inline void SetPrev(NodeList<T> *prev)
    {
        prev_ = prev;
    }
    inline NodeList<T> *GetPrev() const
    {
        return prev_;
    }

    inline void SetFreeNext(NodeList<T> *next)
    {
        freeNext_ = next;
    }
    inline NodeList<T> *GetFreeNext() const
    {
        return freeNext_;
    }

    inline void SetFreePrev(NodeList<T> *prev)
    {
        freePrev_ = prev;
    }
    inline NodeList<T> *GetFreePrev() const
    {
        return freePrev_;
    }

    template<class Callback>
    inline void IterateUsageGlobal(Callback callback)
    {
        T *next = usedList_;
        T *current = nullptr;
        while (next != nullptr) {
            current = next;
            next = reinterpret_cast<T *>(current->GetNext());
            ASSERT(current != next);
            callback(current);
        }
    }

private:
    static const uint32_t GLOBAL_BLOCK_SIZE = 256;
    T nodeList_[GLOBAL_BLOCK_SIZE];  // all
    T *freeList_ {nullptr};  // dispose node
    T *usedList_ {nullptr};  // usage node
    uint32_t index_ {0};
    NodeList<T> *next_ {nullptr};
    NodeList<T> *prev_ {nullptr};
    NodeList<T> *freeNext_ {nullptr};
    NodeList<T> *freePrev_ {nullptr};
};

enum class NodeKind : uint8_t {
    NORMAL_NODE,
    UNIFIED_NODE,
};

template<typename T>
class EcmaGlobalStorage {
public:
    using WeakClearCallback = void (*)(void *);

    EcmaGlobalStorage(JSThread *thread, NativeAreaAllocator *allocator)
        : thread_(thread), allocator_(allocator)
    {
        topGlobalNodes_ = lastGlobalNodes_ = allocator_->New<NodeList<T>>();
        topXRefGlobalNodes_ = lastXRefGlobalNodes_ = allocator_->New<NodeList<T>>();
        topWeakGlobalNodes_ = lastWeakGlobalNodes_ = allocator_->New<NodeList<WeakNode>>();
    }

    ~EcmaGlobalStorage()
    {
        auto clearWeakNodeCallback = [] (WeakNode *node) {
            node->SetUsing(false);
            node->SetObject(JSTaggedValue::Undefined().GetRawData());
            node->CallFreeGlobalCallback();
            node->CallNativeFinalizeCallback();
        };
        auto clearNodeCallback = [] (T *node) {
            node->SetUsing(false);
            node->SetObject(JSTaggedValue::Undefined().GetRawData());
        };

        IterateNodeList<decltype(clearWeakNodeCallback), WeakNode>(clearWeakNodeCallback, topWeakGlobalNodes_, true);
        IterateNodeList<decltype(clearNodeCallback), T>(clearNodeCallback, topXRefGlobalNodes_, true);
        IterateNodeList<decltype(clearNodeCallback), T>(clearNodeCallback, topGlobalNodes_, true);
    }

    template<class Callback, class S>
    inline void IterateNodeList(Callback callback, NodeList<S> *nodeList, bool releaseNodeList)
    {
        auto *next = nodeList;
        NodeList<S> *current = nullptr;
        while (next != nullptr) {
            current = next;
            next = current->GetNext();
            ASSERT(current != next);
            current->IterateUsageGlobal(callback);
            if (releaseNodeList) {
                allocator_->Delete(current);
            }
        }
    }

    template<NodeKind nodeKind>
    inline uintptr_t NewGlobalHandle(JSTaggedType value)
    {
        if constexpr(nodeKind == NodeKind::NORMAL_NODE) {
            return NewGlobalHandleImplement(&lastGlobalNodes_, &freeListNodes_, value);
        } else if constexpr(nodeKind == NodeKind::UNIFIED_NODE) {
            return NewGlobalHandleImplement(&lastXRefGlobalNodes_, &xRefFreeListNodes_, value);
        }
    }

    template<NodeKind nodeKind>
    inline void DisposeGlobalHandle(uintptr_t nodeAddr)
    {
        T *node = reinterpret_cast<T *>(nodeAddr);
        if (!node->IsUsing()) {
            return;
        }
        if constexpr(nodeKind == NodeKind::UNIFIED_NODE) {
            DisposeGlobalHandleInner(node, &xRefFreeListNodes_, &topXRefGlobalNodes_,
                &lastXRefGlobalNodes_);
        } else if constexpr(nodeKind == NodeKind::NORMAL_NODE) {
            if (node->IsWeak()) {
                DisposeGlobalHandleInner(reinterpret_cast<WeakNode *>(node), &weakFreeListNodes_, &topWeakGlobalNodes_,
                    &lastWeakGlobalNodes_);
            } else {
                DisposeGlobalHandleInner(node, &freeListNodes_, &topGlobalNodes_, &lastGlobalNodes_);
            }
        }
    }

    inline uintptr_t SetWeak(uintptr_t nodeAddr, void *ref = nullptr, WeakClearCallback freeGlobalCallback = nullptr,
                             WeakClearCallback nativeFinalizeCallback = nullptr)
    {
        auto value = reinterpret_cast<T *>(nodeAddr)->GetObject();
        DisposeGlobalHandle<NodeKind::NORMAL_NODE>(nodeAddr);
        uintptr_t addr = NewGlobalHandleImplement(&lastWeakGlobalNodes_, &weakFreeListNodes_, value);
        WeakNode *node = reinterpret_cast<WeakNode *>(addr);
        node->SetReference(ref);
        node->SetFreeGlobalCallback(freeGlobalCallback);
        node->SetNativeFinalizeCallback(nativeFinalizeCallback);
        return addr;
    }

    inline uintptr_t ClearWeak(uintptr_t nodeAddr)
    {
        auto value = reinterpret_cast<T *>(nodeAddr)->GetObject();
        DisposeGlobalHandle<NodeKind::NORMAL_NODE>(nodeAddr);
        uintptr_t ret = NewGlobalHandleImplement(&lastGlobalNodes_, &freeListNodes_, value);
        return ret;
    }

    inline bool IsWeak(uintptr_t addr) const
    {
        T *node = reinterpret_cast<T *>(addr);
        return node->IsWeak();
    }

    template<class Callback>
    void IterateUsageGlobal(Callback &&callback)
    {
        IterateNodeList<Callback, T>(callback, topGlobalNodes_, false);
        if (nodeKind_ == NodeKind::UNIFIED_NODE) {
            return;
        }
        IterateNodeList<Callback, T>(callback, topXRefGlobalNodes_, false);
    }

    template<class Callback>
    void IterateWeakUsageGlobal(Callback callback)
    {
        IterateNodeList<Callback, WeakNode>(callback, topWeakGlobalNodes_, false);
    }

    ECMAGLOBALSTORAGE_PUBLIC_HYBRID_EXTENSION()
private:
    NO_COPY_SEMANTIC(EcmaGlobalStorage);
    NO_MOVE_SEMANTIC(EcmaGlobalStorage);
    template<typename S>
    inline void DisposeGlobalHandleInner(S *node, NodeList<S> **freeList, NodeList<S> **topNodes,
                                    NodeList<S> **lastNodes)
    {
        NodeList<S> *list = NodeList<S>::NodeToNodeList(node);
        list->FreeNode(thread_, node);

        // If NodeList has no usage node, then delete NodeList
        if (!list->HasUsagedNode() && (*topNodes != *lastNodes)) {
            list->RemoveList();
            if (*freeList == list) {
                *freeList = list->GetFreeNext();
            }
            if (*topNodes == list) {
                *topNodes = list->GetNext();
            }
            if (*lastNodes == list) {
                *lastNodes = list->GetPrev();
            }
            allocator_->Delete(list);
        } else {
            // Add to freeList
            if (list != *freeList && list->GetFreeNext() == nullptr && list->GetFreePrev() == nullptr) {
                list->SetFreeNext(*freeList);
                if (*freeList != nullptr) {
                    (*freeList)->SetFreePrev(list);
                }
                *freeList = list;
            }
        }
    }

    template<typename S>
    inline uintptr_t NewGlobalHandleImplement(NodeList<S> **storage, NodeList<S> **freeList, JSTaggedType value)
    {
#if ECMASCRIPT_ENABLE_NEW_HANDLE_CHECK
        thread_->CheckJSTaggedType(value);
#endif
        if (!(*storage)->IsFull()) {
            // alloc new block
            S *node = (*storage)->NewNode(thread_, value);
            ASSERT(node != nullptr);
            return node->GetObjectAddress();
        }
        if (*freeList != nullptr) {
            // use free_list node
            S *node = (*freeList)->GetFreeNode(thread_, value);
            ASSERT(node != nullptr);
            if (!(*freeList)->HasFreeNode()) {
                auto next = (*freeList)->GetFreeNext();
                (*freeList)->SetFreeNext(nullptr);
                (*freeList)->SetFreePrev(nullptr);
                if (next != nullptr) {
                    next->SetFreePrev(nullptr);
                }
                *freeList = next;
            }
            return node->GetObjectAddress();
        }
        auto block = allocator_->New<NodeList<S>>();
        if (block == nullptr) {
            LOG_ECMA(FATAL) << "NewGlobalHandleImplement:block is nullptr";
        }
        block->LinkTo(*storage);
        *storage = block;

        // use node in block finally
        S *node = (*storage)->NewNode(thread_, value);
        ASSERT(node != nullptr);
        return node->GetObjectAddress();
    }

    [[maybe_unused]] JSThread *thread_ {nullptr};
    NativeAreaAllocator *allocator_ {nullptr};
    NodeList<T> *topGlobalNodes_ {nullptr};
    NodeList<T> *lastGlobalNodes_ {nullptr};
    NodeList<T> *freeListNodes_ {nullptr};

    NodeList<WeakNode> *topWeakGlobalNodes_ {nullptr};
    NodeList<WeakNode> *lastWeakGlobalNodes_ {nullptr};
    NodeList<WeakNode> *weakFreeListNodes_ {nullptr};
    ECMAGLOBALSTORAGE_PRIVATE_HYBRID_EXTENSION();
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_ECMA_GLOBAL_STORAGE_H
