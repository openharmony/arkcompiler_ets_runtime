/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DFX_HPROF_HEAP_SNAPSHOT_H
#define ECMASCRIPT_DFX_HPROF_HEAP_SNAPSHOT_H

#include <atomic>
#include <cstdint>
#include <fstream>
#include <sys/time.h>

#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/hprof/heap_root_visitor.h"
#include "ecmascript/dfx/hprof/string_hashmap.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/dfx/hprof/file_stream.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/object_fast_operator.h"

namespace panda::ecmascript {
class EntryIdMap;
// Define the Object Graphic
using Address = uintptr_t;

enum class NodeType {
    HIDDEN,
    ARRAY,
    STRING,
    OBJECT,
    CODE,
    CLOSURE,
    REGEXP,
    HEAPNUMBER,
    NATIVE,
    SYNTHETIC,
    CONSSTRING,
    SLICEDSTRING,
    SYMBOL,
    BIGINT,
    DEFAULT = NATIVE,
};

enum class EdgeType { CONTEXT, ELEMENT, PROPERTY, INTERNAL, HIDDEN, SHORTCUT, WEAK, DEFAULT = PROPERTY };

class Node {
public:
    Node(NodeId id, uint32_t index, const CString *name, NodeType type, size_t size, size_t nativeSize,
         uint32_t traceId, JSTaggedType address, bool isLive = true)
        : id_(id),
          index_(index),
          name_(name),
          type_(type),
          size_(size),
          nativeSize_(nativeSize),
          traceId_(traceId),
          address_(address),
          isLive_(isLive)
    {
    }
    NodeId GetId() const
    {
        return id_;
    }
    void SetIndex(uint32_t index)
    {
        index_ = index;
    }
    uint32_t GetIndex() const
    {
        return index_;
    }

    const CString *GetName() const
    {
        return name_;
    }

    void SetName(CString *name)
    {
        name_ = name;
    }

    NodeType GetType() const
    {
        return type_;
    }
    size_t GetSelfSize() const
    {
        return size_;
    }
    void SetSelfSize(size_t size)
    {
        size_ = size;
    }
    size_t GetNativeSize() const
    {
        return nativeSize_;
    }
    void SetNativeSize(size_t size)
    {
        nativeSize_ = size;
    }
    size_t GetEdgeCount() const
    {
        return edgeCount_;
    }
    void IncEdgeCount()
    {
        edgeCount_++;
    }
    uint32_t GetStackTraceId() const
    {
        return traceId_;
    }
    JSTaggedType GetAddress() const
    {
        return address_;
    }
    void SetAddress(JSTaggedType address)
    {
        address_ = address;
    }
    bool IsLive() const
    {
        return isLive_;
    }
    void SetLive(bool isLive)
    {
        isLive_ = isLive;
    }
    void SetTraceId(uint32_t traceId)
    {
        traceId_ = traceId;
    }
    static Node *NewNode(Chunk *chunk, NodeId id, size_t index, const CString *name, NodeType type, size_t size,
                         size_t nativeSize, JSTaggedType entry, bool isLive = true);
    template<typename T>
    static JSTaggedType NewAddress(T *addr)
    {
        return reinterpret_cast<JSTaggedType>(addr);
    }
    static constexpr int NODE_FIELD_COUNT = 8;
    ~Node() = default;

private:
    NodeId id_ {0};  // Range from 1
    uint32_t index_ {0};
    const CString *name_ {nullptr};
    NodeType type_ {NodeType::DEFAULT};
    size_t size_ {0};
    size_t nativeSize_ {0};
    size_t edgeCount_ {0};
    uint32_t traceId_ {0};
    JSTaggedType address_ {0};
    bool isLive_ {true};
};

class Edge {
public:
    Edge(EdgeType type, Node *from, Node *to, CString *name)
        : edgeType_(type), from_(from), to_(to), name_(name) {}
    Edge(EdgeType type, Node *from, Node *to, uint32_t index)
        : edgeType_(type), from_(from), to_(to), index_(index) {}
    EdgeType GetType() const
    {
        return edgeType_;
    }
    const Node *GetFrom() const
    {
        return from_;
    }
    const Node *GetTo() const
    {
        return to_;
    }
    const CString *GetName() const
    {
        ASSERT(GetType() != EdgeType::ELEMENT);
        return name_;
    }
    uint32_t GetIndex() const
    {
        ASSERT(GetType() == EdgeType::ELEMENT);
        return index_;
    }
    void SetName(CString *name)
    {
        ASSERT(GetType() != EdgeType::ELEMENT);
        name_ = name;
    }
    void UpdateFrom(Node *node)
    {
        from_ = node;
    }
    void UpdateTo(Node *node)
    {
        to_ = node;
    }
    static Edge *NewEdge(Chunk *chunk, EdgeType type, Node *from, Node *to, CString *name);
    static Edge *NewEdge(Chunk *chunk, EdgeType type, Node *from, Node *to, uint32_t index);
    static constexpr int EDGE_FIELD_COUNT = 3;
    ~Edge() = default;

private:
    EdgeType edgeType_ {EdgeType::DEFAULT};
    Node *from_ {nullptr};
    Node *to_ {nullptr};
    union {
        CString *name_;
        uint32_t index_;
    };
};

class TimeStamp {
public:
    explicit TimeStamp(int sequenceId) : lastSequenceId_(sequenceId), timeStampUs_(TimeStamp::Now()) {}
    ~TimeStamp() = default;

    DEFAULT_MOVE_SEMANTIC(TimeStamp);
    DEFAULT_COPY_SEMANTIC(TimeStamp);

    int GetLastSequenceId() const
    {
        return lastSequenceId_;
    }

    int64_t GetTimeStamp() const
    {
        return timeStampUs_;
    }

    uint32_t GetSize() const
    {
        return size_;
    }

    void SetSize(uint32_t size)
    {
        size_ = size;
    }

    uint32_t GetCount() const
    {
        return count_;
    }

    void SetCount(uint32_t count)
    {
        count_ = count;
    }

private:
    static int64_t Now()
    {
        struct timeval tv = {0, 0};
        gettimeofday(&tv, nullptr);
        const int THOUSAND = 1000;
        return tv.tv_usec + tv.tv_sec * THOUSAND * THOUSAND;
    }

    int lastSequenceId_ {0};
    int64_t timeStampUs_ {0};
    uint32_t size_ {0};
    uint32_t count_ {0};
};

class HeapEntryMap {
public:
    HeapEntryMap() = default;
    ~HeapEntryMap() = default;
    NO_MOVE_SEMANTIC(HeapEntryMap);
    NO_COPY_SEMANTIC(HeapEntryMap);
    Node *FindOrInsertNode(Node *node);
    Node *FindAndEraseNode(JSTaggedType addr);
    Node *FindEntry(JSTaggedType addr);
    void InsertEntry(Node *node);

private:
    CUnorderedMap<JSTaggedType, Node *> nodesMap_ {};
};

struct FunctionInfo {
    int functionId = 0;
    std::string functionName = "";
    std::string scriptName = "";
    int scriptId = 0;
    int columnNumber = 0;
    int lineNumber = 0;
};

class TraceTree;
class TraceNode {
public:
    TraceNode(TraceTree* tree, uint32_t nodeIndex);
    ~TraceNode();

    TraceNode(const TraceNode&) = delete;
    TraceNode& operator=(const TraceNode&) = delete;
    TraceNode* FindChild(uint32_t nodeIndex);
    TraceNode* FindOrAddChild(uint32_t nodeIndex);
    uint32_t GetNodeIndex() const
    {
        return nodeIndex_;
    }
    uint32_t GetTotalSize() const
    {
        return totalSize_;
    }
    uint32_t GetTotalCount() const
    {
        return totalCount_;
    }
    uint32_t GetId() const
    {
        return id_;
    }
    const std::vector<TraceNode*>& GetChildren() const
    {
        return children_;
    }
    TraceNode &SetTotalSize(uint32_t totalSize)
    {
        totalSize_ = totalSize;
        return *this;
    }
    TraceNode &SetTotalCount(uint32_t totalCount)
    {
        totalCount_ = totalCount;
        return *this;
    }

private:
    TraceTree* tree_ {nullptr};
    uint32_t nodeIndex_ {0};
    uint32_t totalSize_ {0};
    uint32_t totalCount_ {0};
    uint32_t id_ {0};
    std::vector<TraceNode*> children_ {};
};

class TraceTree {
public:
    TraceTree() : nextNodeId_(1), root_(this, 0)
    {
    }
    ~TraceTree() = default;
    TraceTree(const TraceTree&) = delete;
    TraceTree& operator=(const TraceTree&) = delete;
    TraceNode* AddNodeToTree(CVector<uint32_t> traceNodeIndex);
    TraceNode* GetRoot()
    {
        return &root_;
    }
    uint32_t GetNextNodeId()
    {
        return nextNodeId_++;
    }

private:
    uint32_t nextNodeId_ {0};
    TraceNode root_;
};

struct Reference {
    enum class ReferenceType { CONTEXT, ELEMENT, PROPERTY, INTERNAL, HIDDEN, SHORTCUT, WEAK, DEFAULT = PROPERTY };

    Reference(const CString &name, JSTaggedValue value) : name_(name), value_(value) {}
    Reference(const CString &name, JSTaggedValue value, ReferenceType type) : name_(name), value_(value), type_(type) {}
    Reference(uint32_t index, JSTaggedValue value, ReferenceType type) : index_(index), value_(value), type_(type) {}

    CString name_;
    uint32_t index_ {-1U};
    JSTaggedValue value_;
    ReferenceType type_ {ReferenceType::DEFAULT};
};

class EntryVisitor {
public:
    NO_MOVE_SEMANTIC(EntryVisitor);
    NO_COPY_SEMANTIC(EntryVisitor);
    EntryVisitor() = default;
    ~EntryVisitor() = default;
    static CString ConvertKey(JSTaggedValue key);
};

class HeapSnapshot {
public:
    static constexpr int SEQ_STEP = 2;
    NO_MOVE_SEMANTIC(HeapSnapshot);
    NO_COPY_SEMANTIC(HeapSnapshot);
    HeapSnapshot(const EcmaVM *vm, StringHashMap *stringTable, const DumpSnapShotOption &dumpOption,
                 const bool trackAllocations, EntryIdMap *entryIdMap, Chunk *chunk)
        : vm_(vm), stringTable_(stringTable), isVmMode_(dumpOption.isVmMode), isPrivate_(dumpOption.isPrivate),
          captureNumericValue_(dumpOption.captureNumericValue), trackAllocations_(trackAllocations),
          entryIdMap_(entryIdMap), chunk_(chunk) {}
    ~HeapSnapshot();
    bool BuildUp(bool isSimplify = false);
    bool Verify();

    void PrepareSnapshot();
    void UpdateNodes(bool isInFinish = false);
    Node *AddNode(TaggedObject *address, size_t size);
    void MoveNode(uintptr_t address, TaggedObject *forwardAddress, size_t size);
    void RecordSampleTime();
    bool FinishSnapshot();
    void PushHeapStat(Stream* stream);
    int AddTraceNode(int sequenceId, int size);
    bool AddMethodInfo(MethodLiteral *methodLiteral,
                       const JSPandaFile *jsPandaFile, int sequenceId);
    void AddTraceNodeId(MethodLiteral *methodLiteral);

    const CVector<TimeStamp> &GetTimeStamps() const
    {
        return timeStamps_;
    }

    size_t GetNodeCount() const
    {
        return nodeCount_;
    }
    size_t GetEdgeCount() const
    {
        return edgeCount_;
    }
    size_t GetTotalNodeSize() const
    {
        return totalNodesSize_;
    }
    void AccumulateNodeSize(size_t size)
    {
        totalNodesSize_ += static_cast<int>(size);
    }
    void DecreaseNodeSize(size_t size)
    {
        totalNodesSize_ -= static_cast<int>(size);
    }
    CString *GenerateNodeName(TaggedObject *entry);
    NodeType GenerateNodeType(TaggedObject *entry);
    const CList<Node *> *GetNodes() const
    {
        return &nodes_;
    }
    const CList<Edge *> *GetEdges() const
    {
        return &edges_;
    }

    CString *GetString(const CString &as);
    CString *GetArrayString(TaggedArray *array, const CString &as);

    bool IsInVmMode() const
    {
        return isVmMode_;
    }

    bool IsPrivate() const
    {
        return isPrivate_;
    }

    bool trackAllocations() const
    {
        return trackAllocations_;
    }

    const CVector<FunctionInfo> &GetTrackAllocationsStack() const
    {
        return traceInfoStack_;
    }

    TraceTree* GetTraceTree()
    {
        return &traceTree_;
    }

    void PrepareTraceInfo()
    {
        struct FunctionInfo info;
        info.functionName = "(root)";
        GetString(info.functionName.c_str());
        traceInfoStack_.push_back(info);
    }
    const StringHashMap *GetEcmaStringTable() const
    {
        return stringTable_;
    }

    bool BuildSnapshotForBinMod(CUnorderedMap<uint64_t, NewAddr *> &objMap,
                                CUnorderedMap<uint64_t, CUnorderedSet<uint64_t>> &refSetMap);
    void GenerateNodeForBinMod(CUnorderedMap<uint64_t, NewAddr *> &objMap, CUnorderedSet<uint64_t> &rootSetMap,
                               CUnorderedMap<uint64_t, CString *> &strTableIdMap);

    StringId GenerateStringId(TaggedObject *obj)
    {
        JSTaggedValue entry(obj);
        if (entry.IsOnlyJSObject()) {
            return stringTable_->InsertStrAndGetStringId(ParseObjectName(obj));
        }
        if (entry.IsJSFunction()) {
            return stringTable_->InsertStrAndGetStringId(ParseFunctionName(obj));
        }
        return 1; // 1 : invalid id
    }

private:
    void FillNodes(bool isInFinish = false, bool isSimplify = false);
    Node *GenerateNode(JSTaggedValue entry, size_t size = 0,
                       bool isInFinish = false, bool isSimplify = false, bool isBinMod = false);
    Node *HandleStringNode(JSTaggedValue &entry, size_t &size, bool &isInFinish, bool isBinMod);
    Node *HandleFunctionNode(JSTaggedValue &entry, size_t &size, bool &isInFinish);
    Node *HandleObjectNode(JSTaggedValue &entry, size_t &size, bool &isInFinish);
    Node *HandleBaseClassNode(size_t size, bool idExist, NodeId &sequenceId,
                              TaggedObject* obj, JSTaggedType &addr);
    CString GeneratePrimitiveNameString(JSTaggedValue &entry);
    Node *GeneratePrivateStringNode(size_t size);
    Node *GenerateStringNode(JSTaggedValue entry, size_t size, bool isInFinish = false, bool isBinMod = false);
    Node *GenerateFunctionNode(JSTaggedValue entry, size_t size, bool isInFinish = false);
    Node *GenerateObjectNode(JSTaggedValue entry, size_t size, bool isInFinish = false);
    void FillEdges(bool isSimplify = false);
    void RenameFunction(const CString &edgeName, Node *entryFrom, Node *entryTo);
    CString ParseFunctionName(TaggedObject *obj);
    const CString ParseObjectName(TaggedObject *obj);

    Node *InsertNodeUnique(Node *node);
    void EraseNodeUnique(Node *node);
    Edge *InsertEdgeUnique(Edge *edge);
    void AddSyntheticRoot();
    void FillEdgesForBinMod(char *newAddr, CUnorderedSet<uint64_t> *refSet);
    Node *InsertNodeAt(size_t pos, Node *node);
    Edge *InsertEdgeAt(size_t pos, Edge *edge);

    CList<Node *> nodes_ {};
    CList<Edge *> edges_ {};
    CVector<TimeStamp> timeStamps_ {};
    int nodeCount_ {0};
    int edgeCount_ {0};
    int totalNodesSize_ {0};
    HeapEntryMap entryMap_;
    panda::ecmascript::HeapRootVisitor rootVisitor_;
    const EcmaVM *vm_;
    StringHashMap *stringTable_ {nullptr};
    bool isVmMode_ {true};
    bool isPrivate_ {false};
    bool captureNumericValue_ {false};
    Node* privateStringNode_ {nullptr};
    bool trackAllocations_ {false};
    CVector<FunctionInfo> traceInfoStack_ {};
    CMap<MethodLiteral *, struct FunctionInfo> stackInfo_;
    CMap<std::string, int> scriptIdMap_;
    TraceTree traceTree_;
    CMap<MethodLiteral *, uint32_t> methodToTraceNodeId_;
    CVector<uint32_t> traceNodeIndex_;
    EntryIdMap* entryIdMap_;
    Chunk *chunk_ {nullptr};
    friend class HeapSnapShotFriendTest;
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_HPROF_HEAP_SNAPSHOT_H
