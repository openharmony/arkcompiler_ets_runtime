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

#include <chrono>
#include "ecmascript/dfx/hprof/rawheap_translate/rawheap_translate.h"
#include "ecmascript/dfx/hprof/rawheap_translate/serializer.h"

namespace rawheap_translate {
RawHeap::~RawHeap()
{
    delete strTable_;
    nodes_.clear();
    edges_.clear();
}

bool RawHeap::TranslateRawheap(const std::string &inputPath, const std::string &outputPath)
{
    auto start = std::chrono::steady_clock::now();
    FileReader file;
    if (!file.Initialize(inputPath)) {
        return false;
    }

    uint64_t fileSize = FileReader::GetFileSize(inputPath);
    if (!file.CheckAndGetHeaderAt(fileSize - sizeof(uint64_t), 0)) {
        LOG_ERROR_ << "Read rawheap file header failed!";
        return false;
    }

    MetaParser metaParser;
    if (!ParseMetaData(file, &metaParser)) {
        return false;
    }

    RawHeap *rawheap = ParseRawheap(file, &metaParser);
    if (rawheap == nullptr) {
        return false;
    }

    if (!rawheap->Parse(file, file.GetHeaderLeft()) || !rawheap->Translate()) {
        delete rawheap;
        return false;
    }

    StreamWriter writer;
    if (!writer.Initialize(outputPath)) {
        delete rawheap;
        return false;
    }

    HeapSnapshotJSONSerializer::Serialize(rawheap, &writer);
    delete rawheap;
    auto end = std::chrono::steady_clock::now();
    int duration = (int)std::chrono::duration<double>(end - start).count();
    LOG_INFO_ << "file save to " << outputPath << ", cost " << std::to_string(duration) << 's';
    return true;
}

bool RawHeap::ParseMetaData(FileReader &file, MetaParser *parser)
{
    if (!file.CheckAndGetHeaderAt(file.GetFileSize() - sizeof(uint64_t), 0)) {
        LOG_ERROR_ << "rawheap header error!";
        return false;
    }

    std::vector<char> metadata(file.GetHeaderRight());
    if (!file.Seek(file.GetHeaderLeft()) || !file.Read(metadata.data(), file.GetHeaderRight())) {
        LOG_ERROR_ << "read metadata failed!";
        return false;
    }

    cJSON *json = cJSON_Parse(metadata.data());
    if (json == nullptr) {
        LOG_ERROR_ << "metadata cjson parse failed!";
        return false;
    }

    bool ret = parser->Parse(json);
    cJSON_Delete(json);
    return ret;
}

RawHeap *RawHeap::ParseRawheap(FileReader &file, MetaParser *metaParser)
{
    Version version;
    if (!version.Parse(RawHeap::ReadVersion(file))) {
        return nullptr;
    }

    if (VERSION < version) {
        LOG_ERROR_ << "The rawheap file's version " << version.ToString()
                   << " is not matched the current rawheap translator,"
                   << " please use the newest version of the translator!";
        return nullptr;
    }

    if (Version(1, 0, 0) < version) {
        return new RawHeapTranslateV2(metaParser);
    }

    return new RawHeapTranslateV1(metaParser);
}

std::string RawHeap::ReadVersion(FileReader &file)
{
    uint32_t size = 8;  // 8: version size
    std::vector<char> version(size);
    if (!file.Seek(0) || !file.Read(version.data(), size)) {
        return "";
    }
    if (*reinterpret_cast<uint64_t *>(version.data()) == 0) {
        return "1.0.0";
    }
    LOG_INFO_ << "current rawheap version is " << version.data();
    return std::string(version.data());
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
    if (!ReadSectionInfo(file, rawheapFileSize, sections_) || !ReadRootTable(file) || !ReadStringTable(file)) {
        return false;
    }

    // 4: object table section start from 4, step is 2
    for (size_t i = 4; i < sections_.size(); i += 2) {
        if (!ReadObjectTable(file, sections_[i], sections_[i + 1])) {
            return false;
        }
    }
    return true;
}

bool RawHeapTranslateV1::Translate()
{
    auto nodes = GetNodes();
    for (auto it = nodes->begin() + 1; it != nodes->end(); ++it) {
        Node *node = *it;
        Node *hclass = FindNode(ByteToU64(node->data));
        if (hclass == nullptr) {
            LOG_ERROR_ << "missed hclass, node_id=" << node->nodeId;
            return false;
        }

        JSType type = metaParser_->GetJSTypeFromHClass(hclass);
        FillNodes(node, type);
        CreateHClassEdge(node, hclass);
        if (!metaParser_->IsString(type)) {
            BuildEdges(node, type);
        }
    }
    LOG_INFO_ << "success!";
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
    if (metaParser_->IsGlobalEnv(type)) {
        BuildGlobalEnvEdges(node, type);
    } else if (metaParser_->IsArray(type)) {
        BuildArrayEdges(node, type);
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
    uint32_t index = 0;
    while ((offset + sizeof(uint64_t)) <= node->size) {
        uint64_t addr = ByteToU64(node->data + offset);
        offset += sizeof(uint64_t);
        EdgeType edgeType = GenerateEdgeTypeAndRemoveWeak(node, type, addr);
        CreateEdge(node, addr, index++, edgeType);
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
    if (node->nodeId == hclass->nodeId) {
        return;
    }
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

RawHeapTranslateV2::~RawHeapTranslateV2()
{
    delete[] mem_;
    sections_.clear();
    nodesMap_.clear();
}

bool RawHeapTranslateV2::Parse(FileReader &file, uint32_t rawheapFileSize)
{
    return ReadSectionInfo(file, rawheapFileSize, sections_) &&
           ReadObjectTable(file) && ReadRootTable(file) && ReadStringTable(file);
}

bool RawHeapTranslateV2::Translate()
{
    FillNodes();
    auto nodes = GetNodes();
    size_t size = nodes->size();
    for (size_t i = 1; i < size; ++i) {
        Node *node = (*nodes)[i];
        Node *hclass = GetNextEdgeTo();
        if (hclass == nullptr) {
            LOG_ERROR_ << "missed hclass, node_id=" << node->nodeId;
            return false;
        } else if (hclass->nodeId != node->nodeId) {
            CreateEdge(node, hclass, InsertAndGetStringId("hclass"), EdgeType::DEFAULT);
        }

        if (metaParser_->IsString(node->jsType)) {
            continue;
        }
        BuildEdges(node);
    }
    LOG_INFO_ << "success!";
    return true;
}

bool RawHeapTranslateV2::ReadRootTable(FileReader &file)
{
    if (!file.CheckAndGetHeaderAt(sections_[0], sizeof(uint32_t))) {
        LOG_ERROR_ << "root table header error!";
        return false;
    }

    std::vector<uint32_t> roots(file.GetHeaderLeft());
    if (!file.ReadArray(roots, file.GetHeaderLeft())) {
        LOG_ERROR_ << "read root addr error!";
        return false;
    }

    AddSyntheticRootNode(roots);
    LOG_INFO_ << "root node count " << file.GetHeaderLeft();
    return true;
}

bool RawHeapTranslateV2::ReadStringTable(FileReader &file)
{
    // 2: index in sections means the offset of string table
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

bool RawHeapTranslateV2::ReadObjectTable(FileReader &file)
{
    // 4: index in sections means the offset of object table
    if (!file.CheckAndGetHeaderAt(sections_[4], sizeof(AddrTableItemV2))) {
        LOG_ERROR_ << "object table header error!";
        return false;
    }

    syntheticRoot_ = CreateNode();
    uint32_t tableSize = file.GetHeaderLeft() * sizeof(AddrTableItemV2);
    // 5: index in sections means the total size of object table
    memSize_ = sections_[5] - tableSize - sizeof(uint64_t);
    std::vector<char> objTableData(tableSize);
    mem_ = new char[memSize_];
    if (!file.Read(objTableData.data(), tableSize) || !file.Read(mem_, memSize_)) {
        LOG_ERROR_ << "read object table failed!";
        return false;
    }

    char *tableData = objTableData.data();
    for (uint32_t i = 0; i < file.GetHeaderLeft(); ++i) {
        AddrTableItemV2 table = {
            ByteToU32(tableData),
            ByteToU32(tableData + sizeof(uint32_t)),
            ByteToU64(tableData + sizeof(uint64_t)),
            ByteToU32(tableData + sizeof(uint64_t) * 2),
            ByteToU32(tableData + sizeof(uint64_t) * 2 + sizeof(uint32_t))
        };

        Node *node = CreateNode();
        nodesMap_.emplace(table.syntheticAddr, node);
        node->size = table.size;
        node->nodeId = table.nodeId;
        node->nativeSize = table.nativeSize;
        node->jsType = static_cast<uint8_t>(table.type);
        tableData += sizeof(AddrTableItemV2);
    }

    LOG_INFO_ << "objects table count " << file.GetHeaderLeft();
    return true;
}

bool RawHeapTranslateV2::ParseStringTable(FileReader &file)
{
    constexpr int HEADER_SIZE = sizeof(uint64_t) / sizeof(uint32_t);
    std::vector<uint32_t> header(HEADER_SIZE);
    if (!file.ReadArray(header, HEADER_SIZE)) {
        return false;
    }

    std::vector<uint32_t> objects(header[1]);
    if (!file.ReadArray(objects, header[1])) {
        LOG_ERROR_ << "read objects addr error!";
        return false;
    }

    std::vector<char> str(header[0] + 1);
    if (!file.Read(str.data(), header[0] + 1)) {
        LOG_ERROR_ << "read string error!";
        return false;
    }

    std::string name(str.data());
    StringId strId = InsertAndGetStringId(name);
    for (auto addr : objects) {
        Node *node = FindNode(addr);
        if (node == nullptr) {
            continue;
        }
        node->strId = strId;
        if (name.find("_GLOBAL") != std::string::npos) {
            node->type = FRAMEWORK_NODETYPE;
        }
    }
    return true;
}

void RawHeapTranslateV2::AddSyntheticRootNode(std::vector<uint32_t> &roots)
{
    syntheticRoot_->nodeId = 1;      // 1: means root node
    syntheticRoot_->type = 9;        // 9: means SYNTHETIC node type
    syntheticRoot_->strId = InsertAndGetStringId("SyntheticRoot");
    syntheticRoot_->edgeCount = roots.size();

    StringId strId = InsertAndGetStringId("-subroot-");
    EdgeType type = EdgeType::SHORTCUT;
    for (auto addr : roots) {
        Node *root = FindNode(addr);
        if (root == nullptr) {
            continue;
        }
        InsertEdge(root, strId, type);
    }
}

Node *RawHeapTranslateV2::FindNode(uint32_t addr)
{
    auto it = nodesMap_.find(addr);
    if (it != nodesMap_.end()) {
        return it->second;
    }
    return nullptr;
}

void RawHeapTranslateV2::FillNodes()
{
    auto nodes = GetNodes();
    for (auto it = nodes->begin() + 1; it != nodes->end(); it++) {
        if ((*it)->type == DEFAULT_NODETYPE) {
            (*it)->type = metaParser_->GetNodeType((*it)->jsType);
        }

        if ((*it)->strId >= StringHashMap::CUSTOM_STRID_START || metaParser_->IsString((*it)->jsType)) {
            continue;
        }
        std::string name = metaParser_->GetTypeName((*it)->jsType);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        (*it)->strId = InsertAndGetStringId(name);
    }
}

void RawHeapTranslateV2::BuildEdges(Node *node)
{
    if (metaParser_->IsArray(node->jsType)) {
        BuildArrayEdges(node);
    } else {
        std::vector<Node *> refs;
        for (uint32_t offset = sizeof(uint64_t); offset < node->size; offset += sizeof(uint64_t)) {
            refs.push_back(GetNextEdgeTo());
        }
        BuildFieldEdges(node, refs);
    }
}

void RawHeapTranslateV2::BuildArrayEdges(Node *node)
{
    uint32_t index = 0;
    for (uint32_t offset = sizeof(uint64_t); offset < node->size; offset += sizeof(uint64_t)) {
        Node *ref = GetNextEdgeTo();
        if (ref == nullptr) {
            continue;
        }
        CreateEdge(node, ref, index++, EdgeType::ELEMENT);
    }
}

void RawHeapTranslateV2::BuildFieldEdges(Node *node, std::vector<Node *> &refs)
{
    MetaData *meta = metaParser_->GetMetaData(node->jsType);
    if (meta == nullptr) {
        LOG_ERROR_ << "js type error, type=" << static_cast<int>(node->jsType);
        return;
    }

    for (auto &field : meta->fields) {
        size_t index = field.offset / sizeof(uint64_t) - 1;
        if (index >= refs.size()) {
            continue;
        }

        Node *to = refs[index];
        if (to == nullptr) {
            continue;
        }

        StringId strId = InsertAndGetStringId(field.name);
        CreateEdge(node, to, strId, EdgeType::DEFAULT);
    }

    if (metaParser_->IsJSObject(node->jsType)) {
        BuildJSObjectEdges(node, refs, meta->endOffset);
    }
}

void RawHeapTranslateV2::BuildJSObjectEdges(Node *node, std::vector<Node *> &refs, uint32_t endOffset)
{
    size_t index = endOffset / sizeof(uint64_t) - 1;
    for (size_t i = index; i < refs.size(); ++i) {
        Node *ref = refs[i];
        if (ref == nullptr) {
            continue;
        }
        CreateEdge(node, ref, InsertAndGetStringId("InlineProperty"), EdgeType::DEFAULT);
    }
}

void RawHeapTranslateV2::CreateEdge(Node *node, Node *to, uint32_t nameOrIndex, EdgeType type)
{
    InsertEdge(to, nameOrIndex, type);
    node->edgeCount++;
}

Node *RawHeapTranslateV2::GetNextEdgeTo()
{
    if (memPos_ + 1 > memSize_) {
        return nullptr;
    }

    uint8_t tag = *reinterpret_cast<uint8_t *>(mem_ + memPos_++);
    if ((tag & ZERO_VALUE) == ZERO_VALUE) {
        return nullptr;
    }

    if ((tag & INTL_VALUE) == INTL_VALUE) {
        memPos_ += sizeof(uint32_t);
        return nullptr;
    }

    if ((tag & DOUB_VALUE) == DOUB_VALUE) {
        memPos_ += sizeof(uint64_t);
        return nullptr;
    }

    Node *node = FindNode(ByteToU32(mem_ + memPos_ - 1));
    memPos_ += sizeof(uint32_t) - 1;
    return node;
}

EdgeType RawHeapTranslateV2::GenerateEdgeType(Node *node)
{
    EdgeType edgeType = EdgeType::DEFAULT;
    if (metaParser_->IsArray(node->type)) {
        edgeType = EdgeType::ELEMENT;
    }
    return edgeType;
}
}
// namespace rawheap_translate
