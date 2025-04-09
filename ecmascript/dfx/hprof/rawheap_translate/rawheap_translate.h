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

#include <optional>
#include <utility>
#include "ecmascript/dfx/hprof/rawheap_translate/metadata_parse.h"
#include "ecmascript/dfx/hprof/rawheap_translate/string_hashmap.h"

namespace panda::test {
class HeapDumpTestHelper;
};

namespace rawheap_translate {
struct AddrTableItem {
    uint64_t addr;
    uint64_t id;
    uint32_t objSize;
    uint32_t offset; // offset to the file
};

struct Node {
    uint64_t nodeId;  // Range from 1
    uint32_t index;
    StringId strId;
    uint8_t type;
    uint32_t size;
    uint32_t nativeSize;
    size_t edgeCount;
    char *data;

    Node(uint32_t nodeIndex)
        : nodeId(0),
          index(nodeIndex),
          strId(1),  // 1: for empty string
          type(8),  // 8: default node type
          size(0),
          nativeSize(0),
          edgeCount(0),
          data(nullptr)
    {
    }
};

enum class EdgeType { CONTEXT, ELEMENT, PROPERTY, INTERNAL, HIDDEN, SHORTCUT, WEAK, DEFAULT = PROPERTY };

struct Edge {
    EdgeType type;
    std::shared_ptr<Node> from;
    std::shared_ptr<Node> to;
    uint32_t nameOrIndex;

    Edge(EdgeType edgeType, std::shared_ptr<Node> nodeFrom, std::shared_ptr<Node> nodeTo, uint32_t index)
        : type(edgeType),
          from(nodeFrom),
          to(nodeTo),
          nameOrIndex(index)
    {
    }
};

class RawHeapTranslate {
public:
    RawHeapTranslate()
        : meta_(std::make_unique<Meta>(Meta())),
          strTable_(std::make_shared<StringHashMap>(StringHashMap())) {}

    ~RawHeapTranslate()
    {
        for (auto mem : memBuf_) {
            delete mem;
        }
        memBuf_.clear();
        nodes_.clear();
        edges_.clear();
        nodesMap_.clear();
    }

    bool Translate(const std::string &rawheapPath);

    std::vector<std::shared_ptr<Node>> GetNodes()
    {
        return nodes_;
    }

    std::vector<std::shared_ptr<Edge>> GetEdges()
    {
        return edges_;
    }

    std::shared_ptr<StringHashMap> GetEcmaStringTable()
    {
        return strTable_;
    }

    size_t GetNodeCount()
    {
        return nodes_.size();
    }

    size_t GetEdgeCount()
    {
        return edges_.size();
    }

private:
    bool ParseMetaData(std::ifstream &file, uint32_t &offset);
    bool ReadMetaDataJson(std::ifstream &file, uint32_t &offset, cJSON **json);
    void DelMetaDataJson(cJSON *json);

    bool ReadSectionInfo(std::ifstream &file, uint32_t endOffset, std::vector<uint32_t> &sections);
    bool ReadVersion(std::ifstream &file, uint32_t offset, uint32_t size);
    bool ReadObjTableBySection(std::ifstream &file, const std::vector<uint32_t> &sections);
    bool ReadObjTable(std::ifstream &file, uint32_t offset, uint32_t size);
    bool ReadStringTable(std::ifstream &file, uint32_t offset, uint32_t size);
    bool ReadRootTable(std::ifstream &file, uint32_t offset, uint32_t size);
    bool ReadFileAtOffset(std::ifstream &file, uint32_t offset, uint32_t size, char *buf);

    void CreateNode(AddrTableItem &item, char *data);
    void FillNodesAndBuildEdges();
    void FillNodes(const std::shared_ptr<Node> &node, char *hclass);
    void BuildEdges(const std::shared_ptr<Node> &from, char *hclass);
    void BuildFieldsEdges(
        const std::shared_ptr<Node> &from, const std::shared_ptr<MetaData> &metadata, uint32_t offset);
    void BuildGlobalEnvEdges(const std::shared_ptr<Node> &from);
    void BuildArrayEdges(const std::shared_ptr<Node> &from, const std::shared_ptr<MetaData> &metadata, uint32_t offset);
    void BuildDictionaryEdges(
        const std::shared_ptr<Node> &from, const std::shared_ptr<MetaData> &metadata, uint32_t offset);
    void BuildJSObjectInlEdges(const std::shared_ptr<Node> &from, char *hclass, uint32_t offset);
    void CreateEdge(const std::shared_ptr<Node> &from, const std::shared_ptr<Node> &to, EdgeType type, uint32_t index);
    void SetNodeStringId(char *addr, uint32_t size, StringId strId);
    bool ByteToAddrTableItem(std::ifstream &file, uint32_t offset, uint32_t objNum, std::vector<AddrTableItem> &table);
    std::optional<std::shared_ptr<Node>> FindNodeFromAddr(uint64_t addr, EdgeType *type);
    std::optional<std::pair<uint32_t, uint32_t>> CheckAndGetHead(
        std::ifstream &file, uint32_t offset, uint32_t assertNum);

    static std::shared_ptr<Field> FindFieldInMetaData(
        const std::shared_ptr<MetaData> &metadata, const std::string &name);
    static void CheckAndRemoveWeak(uint64_t &addr, EdgeType *type);
    static bool IsHeapObject(uint64_t addr);
    static constexpr uint64_t TAG_WEAK = 0x01ULL;
    static constexpr uint64_t TAG_WEAK_MASK = 0x01ULL;
    static constexpr uint64_t TAG_HEAPOBJECT_MASK = (0xFFFFULL << 48) | 0x02ULL | 0x04ULL;  // 48 means 6 byte shift

    std::vector<char *> memBuf_ {};
    std::unique_ptr<Meta> meta_ {nullptr};
    std::shared_ptr<StringHashMap> strTable_ {nullptr};
    std::vector<std::shared_ptr<Node>> nodes_ {};
    std::vector<std::shared_ptr<Edge>> edges_ {};
    std::unordered_map<uint64_t, std::shared_ptr<Node>> nodesMap_ {};

    friend class panda::test::HeapDumpTestHelper;
};

}  // namespace rawheap_translate
#endif