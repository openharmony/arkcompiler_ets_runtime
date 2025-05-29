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

#include "ecmascript/dfx/hprof/rawheap_translate/rawheap_translate.h"

namespace rawheap_translate {
RawHeap::~RawHeap()
{
    delete strTable_;
    nodes_.clear();
    edges_.clear();
}

std::vector<Node *>* RawHeap::GetNodes()
{
    return &nodes_;
}

std::vector<Edge *>* RawHeap::GetEdges()
{
    return &edges_;
}

size_t RawHeap::GetNodeCount()
{
    return nodes_.size();
}

size_t RawHeap::GetEdgeCount()
{
    return edges_.size();
}

StringHashMap *RawHeap::GetStringTable()
{
    return strTable_;
}

std::string RawHeap::GetVersion()
{
    return version_;
}

Node *RawHeap::CreateNode()
{
    Node *node = new Node(nodeIndex_++);
    nodes_.push_back(node);
    return node;
}

void RawHeap::InsertEdge(Node *toNode, uint32_t indexOrStrId, EdgeType type)
{
    Edge *edge = new Edge(toNode, indexOrStrId, type);
    edges_.push_back(edge);
}

StringId RawHeap::InsertAndGetStringId(const std::string &str)
{
    return strTable_->InsertStrAndGetStringId(str);
}

void RawHeap::SetVersion(const std::string &version)
{
    version_ = version;
}

bool RawHeap::ReadSectionInfo(FileReader &file, uint32_t offset, std::vector<uint32_t> &section)
{
    if (!file.CheckAndGetHeaderAt(offset - sizeof(uint64_t), sizeof(uint32_t))) {
        LOG_ERROR_ << "sections header error!";
        return false;
    }

    uint32_t sectionHeaderOffset = offset - (file.GetHeaderLeft() + 2) * sizeof(uint32_t);
    section.resize(file.GetHeaderLeft());
    if (!file.Seek(sectionHeaderOffset) || !file.ReadArray(section, file.GetHeaderLeft())) {
        LOG_ERROR_ << "read sections error!";
        return false;
    }

    return true;
}

RawHeapTranslateV1::~RawHeapTranslateV1()
{
    for (auto &mem : mem_) {
        delete[] mem;
    }
    mem_.clear();
    sections_.clear();
    nodesMap_.clear();
}

bool RawHeapTranslateV1::Parse(FileReader &file, uint32_t rawheapFileSize)
{
    if (!ReadVersion(file) || !ReadSectionInfo(file, rawheapFileSize, sections_) ||
        !ReadRootTable(file) || !ReadStringTable(file)) {
        return false;
    }

    // 4: object table section start from 4, step is 2
    for (int i = 4; i < sections_.size(); i += 2) {
        if (!ReadObjectTable(file, sections_[i], sections_[i + 1])) {
            return false;
        }
    }
    return true;
}

void RawHeapTranslateV1::Translate()
{
    uint32_t missNodeCnt = 0;
    auto nodes = GetNodes();
    for (auto it = nodes->begin() + 1; it != nodes->end(); ++it) {
        Node *node = *it;
        Node *hclass = FindNode(ByteToU64(node->data));
        if (hclass == nullptr) {
            ++missNodeCnt;
            continue;
        }

        JSType type = metaParser_->GetJSTypeFromHClass(hclass);
        FillNodes(node, type);
        CreateHClassEdge(node, hclass);
        if (!metaParser_->IsString(type)) {
            BuildEdges(node, type);
        }
    }

    if (missNodeCnt > 0) {
        LOG_ERROR_ << missNodeCnt << " nodes missed hclass!";
    } else {
        LOG_INFO_ << "success!";
    }
}

bool RawHeapTranslateV1::ReadVersion(FileReader &file)
{
    uint32_t size = 8;  // 8: version size
    std::vector<char> version(size);
    if (!file.Seek(0) || !file.Read(version.data(), size)) {
        return false;
    }
    SetVersion(std::string(version.data()));
    LOG_INFO_ << "current rawheap version is " << GetVersion();
    return true;
}

bool RawHeapTranslateV1::ReadRootTable(FileReader &file)
{
    if (!file.CheckAndGetHeaderAt(sections_[0], sizeof(uint64_t))) {
        LOG_ERROR_ << "root table header error!";
        return false;
    }

    std::vector<uint64_t> roots(file.GetHeaderLeft());
    if (!file.ReadArray(roots, file.GetHeaderLeft())) {
        LOG_ERROR_ << "read root addr error!";
        return false;
    }

    AddSyntheticRootNode(roots);
    LOG_INFO_ << "root node count " << file.GetHeaderLeft();
    return true;
}

bool RawHeapTranslateV1::ReadStringTable(FileReader &file)
{
    // 2: string table start from sections_[2]
    if (!file.CheckAndGetHeaderAt(sections_[2], 0)) {
        LOG_ERROR_ << "string table header error!";
        return false;
    }

    uint32_t strCnt = file.GetHeaderLeft();
    for (uint32_t i = 0; i < strCnt; ++i) {
        ParseStringTable(file);
    }

    LOG_INFO_ << "string table count " << strCnt;
    return true;
}

bool RawHeapTranslateV1::ReadObjectTable(FileReader &file, uint32_t offset, uint32_t totalSize)
{
    if (!file.CheckAndGetHeaderAt(offset, sizeof(AddrTableItem))) {
        LOG_ERROR_ << "object table header error!";
        return false;
    }

    uint32_t tableSize = file.GetHeaderLeft() * file.GetHeaderRight();
    uint32_t memSize = totalSize - tableSize - sizeof(uint64_t);
    std::vector<char> objTableData(tableSize);
    char *mem = new char[memSize];
    if (!file.Read(objTableData.data(), tableSize) || !file.Read(mem, memSize)) {
        delete[] mem;
        return false;
    }

    mem_.push_back(mem);
    char *data = objTableData.data();
    for (uint32_t i = 0; i < file.GetHeaderLeft(); ++i) {
        uint64_t addr = ByteToU64(data);
        Node *node = FindOrCreateNode(addr);
        data += sizeof(uint64_t);

        node->nodeId = ByteToU64(data);
        data += sizeof(uint64_t);

        node->size = ByteToU32(data);
        data += sizeof(uint32_t);

        uint32_t memOffset = ByteToU32(data) - tableSize;
        data += sizeof(uint32_t);

        if (memOffset + sizeof(uint64_t) > memSize) {
            LOG_ERROR_ << "object memory offset error!";
            return false;
        }

        node->data = mem + memOffset;
    }
    LOG_INFO_ << "section objects count " << file.GetHeaderLeft();
    return true;
}

bool RawHeapTranslateV1::ParseStringTable(FileReader &file)
{
    constexpr int HEADER_SIZE = sizeof(uint64_t) / sizeof(uint32_t);
    std::vector<uint32_t> header(HEADER_SIZE);
    if (!file.ReadArray(header, HEADER_SIZE)) {
        return false;
    }

    std::vector<uint64_t> objects(header[1]);
    if (!file.ReadArray(objects, header[1])) {
        LOG_ERROR_ << "read objects addr error!";
        return false;
    }

    std::vector<char> str(header[0] + 1);
    if (!file.Read(str.data(), header[0] + 1)) {
        LOG_ERROR_ << "read string error!";
        return false;
    }

    StringId strId = InsertAndGetStringId(std::string(str.data()));
    SetNodeStringId(objects, strId);
    return true;
}

void RawHeapTranslateV1::AddSyntheticRootNode(std::vector<uint64_t> &roots)
{
    Node *syntheticRoot = CreateNode();
    syntheticRoot->nodeId = 1;      // 1: means root node
    syntheticRoot->type = 9;        // 9: means SYNTHETIC node type
    syntheticRoot->strId = InsertAndGetStringId("SyntheticRoot");
    syntheticRoot->edgeCount = roots.size();

    StringId strId = InsertAndGetStringId("-subroot-");
    EdgeType type = EdgeType::SHORTCUT;
    for (auto addr : roots) {
        Node *root = FindOrCreateNode(addr);
        InsertEdge(root, strId, type);
    }
}

void RawHeapTranslateV1::SetNodeStringId(const std::vector<uint64_t> &objects, StringId strId)
{
    for (auto addr : objects) {
        Node *node = FindOrCreateNode(addr);
        node->strId = strId;
    }
}

Node *RawHeapTranslateV1::FindOrCreateNode(uint64_t addr)
{
    Node *node = FindNode(addr);
    if (node != nullptr) {
        return node;
    }
    node = CreateNode();
    nodesMap_.emplace(addr, node);
    return node;
}

Node *RawHeapTranslateV1::FindNode(uint64_t addr)
{
    auto it = nodesMap_.find(addr);
    if (it != nodesMap_.end()) {
        return it->second;
    }
    return nullptr;
}

void RawHeapTranslateV1::FillNodes(Node *node, JSType type)
{
    node->type = metaParser_->GetNodeType(type);
    node->nativeSize = metaParser_->GetNativateSize(node, type);
    if (node->strId >= StringHashMap::CUSTOM_STRID_START) {
        StringKey stringKey = GetStringTable()->GetKeyByStringId(node->strId);
        std::string nodeName = GetStringTable()->GetStringByKey(stringKey);
        if (nodeName.find("_GLOBAL") != std::string::npos) {
            node->type = FRAMEWORK_NODETYPE;
        }
    } else if (!metaParser_->IsString(type)) {
        std::string name = metaParser_->GetTypeName(type);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        node->strId = InsertAndGetStringId(name);
    }
}

void RawHeapTranslateV1::BuildEdges(Node *node, JSType type)
{
    if (metaParser_->IsArray(type)) {
        BuildArrayEdges(node, type);
    } else if (metaParser_->IsGlobalEnv(type)) {
        BuildGlobalEnvEdges(node, type);
    } else {
        BuildFieldEdges(node, type);
        if (metaParser_->IsJSObject(type)) {
            BuildJSObjectEdges(node, type);
        }
    }
}

void RawHeapTranslateV1::BuildGlobalEnvEdges(Node *node, JSType type)
{
    uint32_t offset = sizeof(uint64_t);
    while ((offset + sizeof(uint64_t)) <= node->size) {
        uint64_t addr = ByteToU64(node->data + offset);
        offset += sizeof(uint64_t);
        EdgeType edgeType = GenerateEdgeTypeAndRemoveWeak(node, type, addr);
        CreateEdge(node, addr, 1, edgeType);
    }
}

void RawHeapTranslateV1::BuildArrayEdges(Node *node, JSType type)
{
    BitField *bitField = metaParser_->GetBitField();
    uint32_t lengthOffset = bitField->taggedArrayLengthField.offset;
    uint32_t dataOffset = bitField->taggedArrayDataField.offset;
    uint32_t step = bitField->taggedArrayDataField.size;

    uint32_t len = ByteToU32(node->data + lengthOffset);
    if (step != sizeof(uint64_t) || len <= 0) {
        return;
    }

    uint32_t offset = dataOffset;
    uint32_t index = 0;
    while (index < len && offset + step <= node->size) {
        uint64_t addr = ByteToU64(node->data + offset);
        offset += step;
        EdgeType edgeType = GenerateEdgeTypeAndRemoveWeak(node, type, addr);
        CreateEdge(node, addr, index++, edgeType);
    }
}

void RawHeapTranslateV1::BuildFieldEdges(Node *node, JSType type)
{
    MetaData *meta = metaParser_->GetMetaData(type);
    if (meta == nullptr) {
        return;
    }

    for (const auto &field : meta->fields) {
        if (field.size != sizeof(uint64_t)) {
            continue;
        }
        uint64_t addr = ByteToU64(node->data + field.offset);
        EdgeType edgeType = GenerateEdgeTypeAndRemoveWeak(node, type, addr);
        StringId strId = InsertAndGetStringId(field.name);
        CreateEdge(node, addr, strId, edgeType);
    }
}

void RawHeapTranslateV1::BuildJSObjectEdges(Node *node, JSType type)
{
    MetaData *meta = metaParser_->GetMetaData(type);
    if (meta == nullptr) {
        return;
    }

    StringId inlinePropertyStrId = InsertAndGetStringId("InlineProperty");
    uint32_t offset = meta->endOffset;
    while (offset + sizeof(uint64_t) <= node->size) {
        uint64_t addr = ByteToU64(node->data + offset);
        EdgeType edgeType = GenerateEdgeTypeAndRemoveWeak(node, type, addr);
        CreateEdge(node, addr, inlinePropertyStrId, edgeType);
        offset += sizeof(uint64_t);
    }
}

void RawHeapTranslateV1::CreateEdge(Node *node, uint64_t addr, uint32_t nameOrIndex, EdgeType type)
{
    Node *to = FindNode(addr);
    if (to == nullptr || to == node) {
        return;
    }
    InsertEdge(to, nameOrIndex, type);
    node->edgeCount++;
}

void RawHeapTranslateV1::CreateHClassEdge(Node *node, Node *hclass)
{
    static StringId hclassStrId = InsertAndGetStringId("hclass");
    InsertEdge(hclass, hclassStrId, EdgeType::DEFAULT);
    node->edgeCount++;
}

EdgeType RawHeapTranslateV1::GenerateEdgeTypeAndRemoveWeak(Node *node, JSType type, uint64_t &addr)
{
    EdgeType edgeType = EdgeType::DEFAULT;
    if (IsWeak(addr)) {
        RemoveWeak(addr);
        edgeType = EdgeType::WEAK;
    }
    if (metaParser_->IsArray(type)) {
        edgeType = EdgeType::ELEMENT;
    }
    return edgeType;
}

bool RawHeapTranslateV1::IsHeapObject(uint64_t addr)
{
    return ((addr & TAG_HEAPOBJECT_MASK) == 0U);
}

bool RawHeapTranslateV1::IsWeak(uint64_t addr)
{
    return (addr & TAG_WEAK_MASK) == TAG_WEAK;
}

void RawHeapTranslateV1::RemoveWeak(uint64_t &addr)
{
    addr &= (~TAG_WEAK);
}
}
// namespace rawheap_translate
