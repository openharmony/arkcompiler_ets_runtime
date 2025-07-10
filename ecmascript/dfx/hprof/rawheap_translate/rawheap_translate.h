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

#ifndef RAWHEAP_TRANSLATE_H
#define RAWHEAP_TRANSLATE_H

#include "ecmascript/dfx/hprof/rawheap_translate/common.h"
#include "ecmascript/dfx/hprof/rawheap_translate/metadata_parse.h"
#include "ecmascript/dfx/hprof/rawheap_translate/string_hashmap.h"

namespace panda::test {
class HeapDumpTestHelper;
};

namespace rawheap_translate {
class RawHeap {
public:
    RawHeap() : strTable_(new StringHashMap())
    {
        LOG_INFO_ << "start to translate rawheap!";
    }

    virtual ~RawHeap();

    virtual bool Parse(FileReader &file, uint32_t rawheapFileSize) = 0;
    virtual void Translate() = 0;

    static bool TranslateRawheap(const std::string &inputPath, const std::string &outputPath);
    static bool ParseMetaData(FileReader &file, MetaParser *parser);
    static RawHeap *ParseRawheap(FileReader &file, MetaParser *metaParser);
    static std::string ReadVersion(FileReader &file);

    std::vector<Node *>* GetNodes();
    std::vector<Edge *>* GetEdges();
    size_t GetNodeCount();
    size_t GetEdgeCount();
    StringHashMap* GetStringTable();
    std::string GetVersion();

protected:
    Node *CreateNode();
    void InsertEdge(Node *toNode, uint32_t indexOrStrId, EdgeType type);
    StringId InsertAndGetStringId(const std::string &str);
    void SetVersion(const std::string &version);

    static bool ReadSectionInfo(FileReader &file, uint32_t offset, std::vector<uint32_t> &section);

private:
    StringHashMap *strTable_ {nullptr};
    std::vector<Node *> nodes_ {};
    std::vector<Edge *> edges_ {};
    std::string version_;
    uint32_t nodeIndex_ {0};
};

class RawHeapTranslateV1 : public RawHeap {
public:
    RawHeapTranslateV1(MetaParser *meta) : metaParser_(meta) {}
    ~RawHeapTranslateV1();

    bool Parse(FileReader &file, uint32_t rawheapFileSize) override;
    void Translate() override;

private:
    struct AddrTableItem {
        uint64_t addr = 0;
        uint64_t id = 0;
        uint32_t objSize = 0;
        uint32_t offset = 0;
    };

    bool ReadRootTable(FileReader &file);
    bool ReadStringTable(FileReader &file);
    bool ReadObjectTable(FileReader &file, uint32_t offset, uint32_t totalSize);
    bool ParseStringTable(FileReader &file);
    void AddSyntheticRootNode(std::vector<uint64_t> &roots);
    void SetNodeStringId(const std::vector<uint64_t> &objects, StringId strId);
    Node* FindOrCreateNode(uint64_t addr);
    Node* FindNode(uint64_t addr);

    void FillNodes(Node *node, JSType type);
    void BuildEdges(Node *node, JSType type);
    void BuildGlobalEnvEdges(Node *node, JSType type);
    void BuildArrayEdges(Node *node, JSType type);
    void BuildFieldEdges(Node *node, JSType type);
    void BuildJSObjectEdges(Node *node, JSType type);
    void CreateEdge(Node *node, uint64_t addr, uint32_t nameOrIndex, EdgeType type);
    void CreateHClassEdge(Node *node, Node *hclass);
    EdgeType GenerateEdgeTypeAndRemoveWeak(Node *node, JSType type, uint64_t &addr);

    static bool IsHeapObject(uint64_t addr);
    static bool IsWeak(uint64_t addr);
    static void RemoveWeak(uint64_t &addr);
    static constexpr uint64_t TAG_WEAK = 0x01ULL;
    static constexpr uint64_t TAG_WEAK_MASK = 0x01ULL;
    static constexpr uint64_t TAG_HEAPOBJECT_MASK = (0xFFFFULL << 48) | 0x02ULL | 0x04ULL;  // 48 means 6 byte shift

    MetaParser *metaParser_ {nullptr};
    std::vector<char *> mem_ {};
    std::vector<uint32_t> sections_ {};
    std::unordered_map<uint64_t, Node *> nodesMap_ {};
    friend class panda::test::HeapDumpTestHelper;
};

class RawHeapTranslateV2 : public RawHeap {
public:
    RawHeapTranslateV2(MetaParser *meta) : metaParser_(meta) {}
    ~RawHeapTranslateV2();

    bool Parse(FileReader &file, uint32_t rawheapFileSize) override;
    void Translate() override;

private:
    struct AddrTableItemV2 {
        uint32_t syntheticAddr;
        uint32_t size;
        uint64_t nodeId;
        uint32_t nativeSize;
        uint32_t type;
    };

    bool ReadRootTable(FileReader &file);
    bool ReadStringTable(FileReader &file);
    bool ReadObjectTable(FileReader &file);
    bool ParseStringTable(FileReader &file);
    void AddSyntheticRootNode(std::vector<uint32_t> &roots);
    Node* FindNode(uint32_t addr);

    void FillNodes();
    void BuildEdges(Node *node);
    void BuildArrayEdges(Node *node);
    void BuildFieldEdges(Node *node, std::vector<Node *> &refs);
    void BuildJSObjectEdges(Node *node, std::vector<Node *> &refs, uint32_t endOffset);
    void CreateEdge(Node *node, Node *to, uint32_t nameOrIndex, EdgeType type);
    Node* GetNextEdgeTo();
    EdgeType GenerateEdgeType(Node *node);

    MetaParser *metaParser_ {nullptr};
    char *mem_ {};
    uint32_t memSize_ {0};
    uint32_t memPos_ {0};
    std::vector<uint32_t> sections_ {};
    std::unordered_map<uint32_t, Node *> nodesMap_ {};
    Node *syntheticRoot_ {nullptr};
    friend class panda::test::HeapDumpTestHelper;
};
}  // namespace rawheap_translate
#endif  // RAWHEAP_TRANSLATE_H