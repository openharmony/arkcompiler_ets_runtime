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
    for (auto node : nodes_) {
        delete node;
    }

    for (auto edge : edges_) {
        delete edge;
    }

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

    Version version;
    if (!version.Parse(RawHeap::ReadVersion(file))) {
        return false;
    }

    RawHeap *rawheap = ParseRawheap(version, &metaParser);
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

    cJSON *json = cJSON_ParseWithLength(metadata.data(), metadata.size());
    if (json == nullptr) {
        LOG_ERROR_ << "metadata cjson parse failed!";
        return false;
    }

    bool ret = parser->Parse(json);
    cJSON_Delete(json);
    return ret;
}

RawHeap *RawHeap::ParseRawheap(const Version &version, MetaParser *metaParser)
{
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

bool RawHeap::ReadHeapMetaData(FileReader &file, uint32_t section)
{
    uint32_t spaceTypeStrSize = 32; // 32: space type size
    uint32_t heapTypeStrSize = 16;  // 16: heap type size
    uint32_t vmTypeStrSize = 8;     // 8: vm type size
    uint32_t headSize = 16;         // 16: skip version(8) + timestamp(8)
    std::vector<char> spaceTypeBuf(spaceTypeStrSize);
    if (section <= headSize) {
        return true;
    }
    if (!file.Seek(headSize) || !file.Read(spaceTypeBuf.data(), spaceTypeStrSize)) {
        return false;
    }
    spaceType_ = std::string(spaceTypeBuf.data());

    std::vector<char> heapTypeBuf(heapTypeStrSize);
    if (!file.Read(heapTypeBuf.data(), heapTypeStrSize)) {
        return false;
    }
    heapType_ = std::string(heapTypeBuf.data());

    std::vector<char> vmTypeBuf(vmTypeStrSize);
    if (!file.Read(vmTypeBuf.data(), vmTypeStrSize)) {
        return false;
    }
    vmType_ = std::string(vmTypeBuf.data());

    LOG_INFO_ << "space type: " << spaceType_ << ", heap type: " << heapType_ << ", vm type: " << vmType_;
    return true;
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

void RawHeap::CreateHashEdge(Node *node)
{
    static const StringId hashStrId = InsertAndGetStringId("ArkInternalHash");
    uint32_t hash = static_cast<uint32_t>(node->nodeId >> 32);  // 32: the high-32bits means hash value
    node->nodeId &= 0xFFFFFFFFULL;
    if (hash == 0) {
        return;
    }

    Node *hashNode = new Node(0);
    hashNode->nodeId = 0;
    hashNode->type = 7;  // 7: means HEAPNUMBER
    hashNode->strId = InsertAndGetStringId("Int:" + std::to_string(hash));
    InsertEdge(hashNode, hashStrId, EdgeType::DEFAULT);
    primitiveNodes_.push_back(hashNode);
    node->edgeCount++;

#ifdef OHOS_UNIT_TEST
    hashSet_.insert(hash);
#endif
}

void RawHeap::AddPrimitiveNodes()
{
    uint32_t index = static_cast<uint32_t>(nodes_.size());
    for (auto &node : primitiveNodes_) {
        node->index = index++;
    }
    nodes_.insert(nodes_.end(), primitiveNodes_.begin(), primitiveNodes_.end());
}

void RawHeap::CreateRootNode(Node *root, const std::string &name, size_t count)
{
    root->nodeId = 0;
    root->type = ROOT;
    root->strId = InsertAndGetStringId(name + "[" + std::to_string(count) + ']');
    root->edgeCount = count;
    root->size = VIRTUAL_NODE_SIZE;
}

void RawHeap::CreateMetadataNode(Node *metadataNode)
{
    metadataNode->nodeId = 0;
    metadataNode->type = 8;     // 8 is native nodetype
    metadataNode->strId = InsertAndGetStringId("HeapMetadata");
    metadataNode->edgeCount = 0;
    metadataNode->size = VIRTUAL_NODE_SIZE;
}

void RawHeap::AddMetadataPropertyNode(Node *metadataNode, const std::string &value, const std::string &propertyName)
{
    Node *propertyNode = new Node(0);
    propertyNode->nodeId = 0;
    propertyNode->type = HEAP_NUMBER;
    propertyNode->strId = InsertAndGetStringId(value);
    propertyNode->size = VIRTUAL_NODE_SIZE;
    primitiveNodes_.push_back(propertyNode);

    StringId propertyStrId = InsertAndGetStringId(propertyName);
    Edge *propertyEdge = new Edge(propertyNode, propertyStrId, EdgeType::PROPERTY);
    edges_.push_back(propertyEdge);

    metadataNode->edgeCount++;
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
    FillNodes();
    auto nodes = GetNodes();
    size_t cnt = nodes->size();
    for (size_t i = 5; i < cnt; i++) {  // 5:normal node start from 5 (synthetic + 4 root groups)
        Node *node = (*nodes)[i];
        if (!node->hclass || !node->data) {
            continue;
        }
        CreateHClassEdge(node);
        CreateHashEdge(node);
        if (!metaParser_->IsString(node->jsType)) {
            BuildEdges(node);
        }
    }

    AddPrimitiveNodes();
    LOG_INFO_ << "success!";
    return true;
}

bool RawHeapTranslateV1::ReadLocalHandleRoots(FileReader &file)
{
    // Determine whether the current rawheap contains handle classifications
    // 2: string table start from sections_[2]
    if (sections_[2] - sections_[0] - sections_[1] <= HANDLE_COUNT_ADDR_SIZE) {
        return true;
    }

    if (!file.CheckAndGetHeaderAt(sections_[0] + sections_[1], 0)) {
        LOG_ERROR_ << "read localhandle count error!";
        return false;
    }

    uint32_t handleCount = file.GetHeaderLeft();
    if (handleCount == 0) {
        return true;
    }

    localHandleRoots_.resize(handleCount);
    file.Seek(sections_[0] + sections_[1] + HANDLE_COUNT_ADDR_SIZE);
    if (!file.ReadArray(localHandleRoots_, handleCount)) {
        LOG_ERROR_ << "read localhandle addr error!";
        return false;
    }

    return true;
}

bool RawHeapTranslateV1::ReadGlobalHandleRoots(FileReader &file)
{
    // Determine whether the current rawheap contains handle classifications
    // 2: string table start from sections_[2]
    if (sections_[2] - sections_[0] - sections_[1] <= HANDLE_COUNT_ADDR_SIZE) {
        return true;
    }
    uint64_t localHandleSize = localHandleRoots_.size() * ADDRESS_SIZE_V1;  // V1 uses 8 bytes per address
    uint64_t offset = sections_[0] + sections_[1] + localHandleSize + HANDLE_COUNT_ADDR_SIZE;

    if (!file.CheckAndGetHeaderAt(offset, 0)) {
        LOG_ERROR_ << "read globalhandle count error!";
        return false;
    }

    uint32_t handleCount = file.GetHeaderLeft();
    if (handleCount == 0) {
        return true;
    }

    globalHandleRoots_.resize(handleCount);
    file.Seek(offset + sizeof(uint32_t));
    if (!file.ReadArray(globalHandleRoots_, handleCount)) {
        LOG_ERROR_ << "read globalhandle addr error!";
        return false;
    }

    return true;
}

bool RawHeapTranslateV1::ReadVmRoots(FileReader &file)
{
    // 2: string table start from sections_[2]
    uint64_t remainingSpace = sections_[2] - sections_[0] - sections_[1];
    uint64_t localHandleSize = localHandleRoots_.size() * ADDRESS_SIZE_V1;
    uint64_t globalHandleSize = globalHandleRoots_.size() * ADDRESS_SIZE_V1;
    uint64_t usedSpace = localHandleSize + globalHandleSize + 2 * HANDLE_COUNT_ADDR_SIZE;  // 2: two count fields
    // Check if remaining space contains vmRoot data
    if (remainingSpace <= usedSpace) {
        return true;
    }

    uint64_t offset = sections_[0] + sections_[1] + usedSpace;
    if (!file.CheckAndGetHeaderAt(offset, 0)) {
        LOG_ERROR_ << "read vmroot count error!";
        return false;
    }

    uint32_t rootCount = file.GetHeaderLeft();
    if (rootCount == 0) {
        return true;
    }

    vmRoots_.resize(rootCount);
    file.Seek(offset + HANDLE_COUNT_ADDR_SIZE);
    if (!file.ReadArray(vmRoots_, rootCount)) {
        LOG_ERROR_ << "read vmroot addr error!";
        return false;
    }

    return true;
}

bool RawHeapTranslateV1::ReadFrameRoots(FileReader &file)
{
    // 2: string table start from sections_[2]
    uint64_t remainingSpace = sections_[2] - sections_[0] - sections_[1];
    uint64_t localHandleSize = localHandleRoots_.size() * ADDRESS_SIZE_V1;
    uint64_t globalHandleSize = globalHandleRoots_.size() * ADDRESS_SIZE_V1;
    uint64_t vmRootsSize = vmRoots_.size() * ADDRESS_SIZE_V1;
    uint64_t usedSpace = localHandleSize + globalHandleSize + vmRootsSize +
                         3 * HANDLE_COUNT_ADDR_SIZE;  // 3: three count fields
    // Check if remaining space contains frameRoot data
    if (remainingSpace <= usedSpace) {
        return true;
    }

    uint64_t offset = sections_[0] + sections_[1] + usedSpace;
    if (!file.CheckAndGetHeaderAt(offset, 0)) {
        LOG_ERROR_ << "read frameroot count error!";
        return false;
    }

    uint32_t rootCount = file.GetHeaderLeft();
    if (rootCount == 0) {
        return true;
    }

    frameRoots_.resize(rootCount);
    file.Seek(offset + HANDLE_COUNT_ADDR_SIZE);
    if (!file.ReadArray(frameRoots_, rootCount)) {
        LOG_ERROR_ << "read frameroot addr error!";
        return false;
    }

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

    if (!ReadHeapMetaData(file, sections_[0])) {
        LOG_ERROR_ << "Read HeapMetaData failed!";
        return false;
    }

    // 2: string table start from sections_[2]
    if (!ReadLocalHandleRoots(file)) {
        return false;
    }

    if (!ReadGlobalHandleRoots(file)) {
        return false;
    }

    if (!ReadVmRoots(file)) {
        return false;
    }

    if (!ReadFrameRoots(file)) {
        return false;
    }

    AddSyntheticRootNode(roots);
    LOG_INFO_ << "root node count " << roots.size()
              << ", local handle count " << localHandleRoots_.size()
              << ", global handle count " << globalHandleRoots_.size()
              << ", vm root count " << vmRoots_.size()
              << ", frame root count " << frameRoots_.size();
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
    if (!file.CheckAndGetHeaderAt(offset, 0) || file.GetHeaderRight() < sizeof(AddrTableItem)) {
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
        AddrTableItem table = {
            ByteToU64(data),    // addr
            ByteToU64(data + sizeof(uint64_t)),     // id
            ByteToU32(data + sizeof(uint64_t) * 2),     // objSize
            ByteToU32(data + sizeof(uint64_t) * 2 + sizeof(uint32_t))   // offset
        };
        uint64_t addr = table.addr;
        Node *node = FindOrCreateNode(addr);
        node->nodeId = table.id;
        node->size = table.objSize;

        uint32_t memOffset = table.offset - tableSize;
        if (memOffset + sizeof(uint64_t) > memSize) {
            LOG_ERROR_ << "object memory offset error!";
            return false;
        }

        node->data = mem + memOffset;
        data += file.GetHeaderRight();
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

void RawHeapTranslateV1::AddHandleRootEdges(const std::vector<uint64_t> &handleRoots)
{
    int index = 0;
    for (auto addr : handleRoots) {
        if (IsHeapObject(addr)) {
            Node *root = FindOrCreateNode(addr);
            InsertEdge(root, index++, EdgeType::ELEMENT);
        }
    }
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

    Node *localHandleRoot = CreateNode();
    CreateRootNode(localHandleRoot, "LocalHandleRoot", localHandleRoots_.size());
    syntheticRoot->edgeCount++;
    InsertEdge(localHandleRoot, strId, type);

    Node *globalHandleRoot = CreateNode();
    CreateRootNode(globalHandleRoot, "GlobalHandleRoot", globalHandleRoots_.size());
    syntheticRoot->edgeCount++;
    InsertEdge(globalHandleRoot, strId, type);

    Node *vmRoot = CreateNode();
    CreateRootNode(vmRoot, "VMRoot", vmRoots_.size());
    syntheticRoot->edgeCount++;
    InsertEdge(vmRoot, strId, type);

    Node *frameRoot = CreateNode();
    CreateRootNode(frameRoot, "FrameRoot", frameRoots_.size());
    syntheticRoot->edgeCount++;
    InsertEdge(frameRoot, strId, type);

    Node *metadataNode = nullptr;
    if (!spaceType_.empty() || !heapType_.empty()) {
        metadataNode = CreateNode();
        CreateMetadataNode(metadataNode);
        syntheticRoot->edgeCount++;
        InsertEdge(metadataNode, strId, type);
    }

    for (auto addr : roots) {
        if (IsHeapObject(addr)) {
            Node *root = FindOrCreateNode(addr);
            InsertEdge(root, strId, type);
        }
    }

    AddHandleRootEdges(localHandleRoots_);
    AddHandleRootEdges(globalHandleRoots_);
    AddHandleRootEdges(vmRoots_);
    AddHandleRootEdges(frameRoots_);
    if (metadataNode != nullptr) {
        AddMetadataPropertyNode(metadataNode, spaceType_, "spaceType");
        AddMetadataPropertyNode(metadataNode, heapType_, "heapType");
        AddMetadataPropertyNode(metadataNode, vmType_, "vmType");
    }
}

void RawHeapTranslateV1::SetNodeStringId(const std::vector<uint64_t> &objects, StringId strId)
{
    for (auto addr : objects) {
        Node *node = FindOrCreateNode(addr);
        node->strId = strId;
        node->type = STRING;
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

void RawHeapTranslateV1::FillNodes()
{
    auto nodes = GetNodes();
    for (auto &node : *nodes) {
        if (!node->data) {
            continue;
        }
        node->hclass = FindNode(ByteToU64(node->data));
        node->jsType = metaParser_->GetJSTypeFromHClass(node->hclass);
        node->type = node->jsType != 0 ? metaParser_->GetNodeType(node->jsType) : node->type;
        node->nativeSize = metaParser_->GetNativateSize(node);
        // For nodes with strId less than CUSTOM_STRID_START and non-string type, need to query NodeName from metadata
        if (node->strId < StringHashMap::CUSTOM_STRID_START && !metaParser_->IsString(node->jsType)) {
            std::string name = metaParser_->GetNodeName(node->jsType);
            node->strId = InsertAndGetStringId(name);
        }
        // For nodes with NodeName containing "_GLOBAL" (except ROOT), set nodeType to FRAMEWORK_NODETYPE
        // because these objects belong to the framework layer to avoid interference with business object analysis
        if (node->strId >= StringHashMap::CUSTOM_STRID_START && node->type != ROOT) {
            StringKey stringKey = GetStringTable()->GetKeyByStringId(node->strId);
            std::string nodeName = GetStringTable()->GetStringByKey(stringKey);
            if (nodeName.find("_GLOBAL") != std::string::npos) {
                node->type = FRAMEWORK_NODETYPE;
            }
        }
    }
}

void RawHeapTranslateV1::BuildEdges(Node *node)
{
    if (metaParser_->IsGlobalEnv(node->jsType)) {
        BuildGlobalEnvEdges(node);
    } else if (metaParser_->IsArray(node->jsType)) {
        BuildArrayEdges(node);
    } else if (metaParser_->IsNativePointer(node->jsType)) {
        BuildJSNativePointerEdges(node);
    } else {
        BuildFieldEdges(node);
        if (metaParser_->IsJSObject(node->jsType)) {
            BuildJSObjectEdges(node);
        }

        if (metaParser_->IsJSWrappedNapiObject(node->jsType)) {
            BuildJSWrappedObjectEdges(node);
        }
    }
}

void RawHeapTranslateV1::BuildGlobalEnvEdges(Node *node)
{
    uint32_t offset = sizeof(uint64_t);
    uint32_t index = 0;
    while ((offset + sizeof(uint64_t)) <= node->size) {
        uint64_t addr = ByteToU64(node->data + offset);
        offset += sizeof(uint64_t);
        EdgeType edgeType = GenerateEdgeTypeAndRemoveWeak(node, addr);
        CreateEdge(node, addr, index++, edgeType);
    }
}

void RawHeapTranslateV1::BuildArrayEdges(Node *node)
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
        EdgeType edgeType = GenerateEdgeTypeAndRemoveWeak(node, addr);
        CreateEdge(node, addr, index++, edgeType);
    }
}

void RawHeapTranslateV1::BuildFieldEdges(Node *node)
{
    MetaData *meta = metaParser_->GetMetaData(node->jsType);
    if (meta == nullptr) {
        return;
    }

    for (const auto &field : meta->fields) {
        if (field.size != sizeof(uint64_t)) {
            continue;
        }
        uint64_t addr = ByteToU64(node->data + field.offset);
        EdgeType edgeType = GenerateEdgeTypeAndRemoveWeak(node, addr);
        StringId strId = InsertAndGetStringId(field.name);
        CreateEdge(node, addr, strId, edgeType);
    }
}

void RawHeapTranslateV1::BuildJSObjectEdges(Node *node)
{
    BitField *bitField = metaParser_->GetBitField();
    Node *properties = FindNode(ByteToU64(node->data + bitField->jsObjectPropertiesField.offset));
    if (properties == nullptr) {
        return;
    }

    if (metaParser_->IsDictionaryMode(properties->jsType)) {
        BuildDictionaryModeEdges(node, properties);
    } else {
        BuildNonDictionaryModeEdges(node, properties);
    }
}

void RawHeapTranslateV1::BuildDictionaryModeEdges(Node *node, Node *properties)
{
    if (properties->data == nullptr) {
        return;
    }

    BitField *bitField = metaParser_->GetBitField();
    DictionaryLayout *dictLayout = metaParser_->GetDictLayout();
    uint32_t size = ByteToU32(properties->data + bitField->taggedArrayDataField.offset + sizeof(uint64_t) * 2);
    uint32_t entryStart = bitField->taggedArrayDataField.offset + dictLayout->headerSize * sizeof(uint64_t);
    uint32_t entrySize = dictLayout->entrySize * sizeof(uint64_t);

    for (uint32_t entry = 0; entry < size; ++entry) {
        uint32_t entryOffset = entryStart + entry * entrySize;
        if (entryOffset + entrySize > properties->size) {
            break;
        }

        uint64_t keyAddr = ByteToU64(properties->data + entryOffset + dictLayout->keyIndex * sizeof(uint64_t));
        uint64_t valueAddr = ByteToU64(properties->data + entryOffset + dictLayout->valueIndex * sizeof(uint64_t));

        if (keyAddr == VALUE_HOLE || keyAddr == VALUE_UNDEFINED) {
            continue;
        }

        Node *keyNode = FindNode(keyAddr);
        if (keyNode == nullptr) {
            continue;
        }

        CreateEdge(node, valueAddr, keyNode->strId, EdgeType::DEFAULT);
    }
}

void RawHeapTranslateV1::BuildNonDictionaryModeEdges(Node *node, Node *properties)
{
    BitField *bitField = metaParser_->GetBitField();
    Node *layout = FindNode(ByteToU64(node->hclass->data + bitField->hclassLayoutField.offset));
    if (!layout || !layout->data) {
        return;
    }

    MetaData *meta = metaParser_->GetMetaData(node->jsType);
    if (meta == nullptr) {
        return;
    }

    StringId defaultStrId = InsertAndGetStringId("InlineProperty");
    uint32_t propNumber = metaParser_->GetPropsNumberOfJSObject(node->hclass);
    uint32_t inlinedPropsCount = metaParser_->GetInlinedPropertiesCount(node->hclass);

    for (uint32_t i = 0; i < propNumber; i++) {
        uint32_t entryOffset = bitField->taggedArrayDataField.offset + sizeof(uint64_t) * (i * 2);
        Node *key = FindNode(ByteToU64(layout->data + entryOffset));
        if (!key) {
            continue;
        }

        uint64_t attrValue = ByteToU64(layout->data + entryOffset + sizeof(uint64_t));
        bool isInlinedProps = metaParser_->IsPropertyInlinedProps(attrValue);

        uint64_t valueAddr;
        if (isInlinedProps) {
            uint32_t inlinedPropsOffset = meta->endOffset + i * sizeof(uint64_t);
            valueAddr = ByteToU64(node->data + inlinedPropsOffset);
        } else {
            uint32_t noInlinedPropsOffset = (i - inlinedPropsCount) * sizeof(uint64_t);
            valueAddr = ByteToU64(properties->data + bitField->taggedArrayDataField.offset + noInlinedPropsOffset);
        }

        EdgeType edgeType = GenerateEdgeTypeAndRemoveWeak(node, valueAddr);
        CreateEdge(node, valueAddr, key->strId == 1 ? defaultStrId : key->strId, edgeType);
    }
}

static bool FindNativePointersFieldOffset(MetaParser *metaParser, JSType type, uint32_t &offset)
{
    MetaData *meta = metaParser->GetMetaData(type);
    if (meta == nullptr) {
        return false;
    }

    for (const auto &field : meta->fields) {
        if (field.name == "NativePointers") {
            offset = field.offset;
            return true;
        }
    }
    return false;
}

static bool GetExternalPointerFieldOffset(MetaParser *metaParser, uint32_t &externalPtrOffset)
{
    MetaData *nativePtrMeta = metaParser->GetMetaData("JS_NATIVE_POINTER");
    if (nativePtrMeta == nullptr) {
        return false;
    }

    for (const auto &field : nativePtrMeta->fields) {
        if (field.name == "ExternalPointer") {
            externalPtrOffset = field.offset;
            return true;
        }
    }
    return false;
}

void RawHeapTranslateV1::BuildJSNativePointerEdges(Node *node)
{
    if (node->data == nullptr) {
        return;
    }
    JSType type = metaParser_->GetJSTypeFromTypeName("JS_NATIVE_POINTER");
    MetaData *meta = metaParser_->GetMetaData(type);
    if (meta == nullptr) {
        return;
    }
    for (const auto &field : meta->fields) {
        if (field.size != sizeof(uint64_t)) {
            continue;
        }
        uint64_t addr = ByteToU64(node->data + field.offset);
        if (field.name == "ExternalPointer") {
            if (addr == 0) {
                continue;
            }
            Node *to = FindOrCreateNode(addr);
            to->nodeId = 0;
            to->type = HEAP_NUMBER;
            std::stringstream ss;
            ss << "0x" << std::hex << addr;
            to->strId = InsertAndGetStringId(ss.str());
            StringId edgeStrId = InsertAndGetStringId(field.name);
            InsertEdge(to, edgeStrId, EdgeType::PROPERTY);
            node->edgeCount++;
        } else {
            EdgeType edgeType = GenerateEdgeTypeAndRemoveWeak(node, addr);
            StringId strId = InsertAndGetStringId(field.name);
            CreateEdge(node, addr, strId, edgeType);
        }
    }
}

void RawHeapTranslateV1::BuildNativePointerEdges(Node *node, Node *array, uint32_t dataOffset,
    uint32_t externalPtrOffset, uint32_t length)
{
    std::stringstream ss;
    for (uint32_t i = 0; i < length; i++) {
        uint32_t elemOffset = dataOffset + i * sizeof(uint64_t);
        uint64_t elemAddr = ByteToU64(array->data + elemOffset);
        Node *nativePtr = FindNode(elemAddr);
        if (nativePtr != nullptr && nativePtr->data != nullptr) {
            uint64_t externalPtrAddr = ByteToU64(nativePtr->data + externalPtrOffset);
            if (externalPtrAddr != 0) {
                Node *to = FindOrCreateNode(externalPtrAddr);
                to->nodeId = 0;
                to->type = HEAP_NUMBER;
                ss << "0x" << std::hex << externalPtrAddr;
                to->strId = InsertAndGetStringId(ss.str());
                StringId edgeStrId = InsertAndGetStringId("NativeAddress" + std::to_string(i));
                InsertEdge(to, edgeStrId, EdgeType::PROPERTY);
                node->edgeCount++;
                ss.str("");
            }
        }
    }
}

void RawHeapTranslateV1::BuildJSWrappedObjectEdges(Node *node)
{
    uint32_t nativePointersOffset = 0;
    if (!FindNativePointersFieldOffset(metaParser_, node->jsType, nativePointersOffset)) {
        return;
    }

    uint64_t arrayAddr = ByteToU64(node->data + nativePointersOffset);
    Node *array = FindNode(arrayAddr);
    if (array == nullptr || array->data == nullptr) {
        return;
    }

    BitField *bitField = metaParser_->GetBitField();
    uint32_t lengthOffset = bitField->taggedArrayLengthField.offset;
    uint32_t dataOffset = bitField->taggedArrayDataField.offset;
    if (array->size < lengthOffset + sizeof(uint32_t)) {
        return;
    }
    uint32_t length = ByteToU32(array->data + lengthOffset);
    if (static_cast<uint64_t>(array->size) <
        static_cast<uint64_t>(dataOffset) + static_cast<uint64_t>(length) * sizeof(uint64_t)) {
        return;
    }

    uint32_t externalPtrOffset = 0;
    if (!GetExternalPointerFieldOffset(metaParser_, externalPtrOffset)) {
        return;
    }

    BuildNativePointerEdges(node, array, dataOffset, externalPtrOffset, length);
}

Node* RawHeapTranslateV1::CreatePrimitiveNode(uint64_t addr)
{
    Node *to = FindOrCreateNode(addr);
    to->nodeId = 0;
    to->type = HEAP_NUMBER;
    if ((addr & TAG_MARK) == TAG_INT) {
        to->strId = InsertAndGetStringId("Int");
    } else if ((addr & TAG_MARK) != TAG_INT && (addr & TAG_MARK) != TAG_OBJECT) {
        to->strId = InsertAndGetStringId("Double");
    } else if (addr == VALUE_HOLE || addr == 0U) {
        to->strId = InsertAndGetStringId("Hole");
    } else if (addr == VALUE_NULL) {
        to->strId = InsertAndGetStringId("Null");
    } else if ((addr & TAG_HEAPOBJECT_MASK) == TAG_BOOLEAN_MASK) {
        to->strId = InsertAndGetStringId((addr & 1) == 0 ? "Boolean:false" : "Boolean:true");
    } else if (addr == VALUE_EXCEPTION) {
        to->strId = InsertAndGetStringId("Exception");
    } else if (addr == VALUE_UNDEFINED) {
        to->strId = InsertAndGetStringId("Undefined");
    } else {
        to->strId = InsertAndGetStringId("Illegal_Primitive");
    }
    return to;
}

void RawHeapTranslateV1::CreateEdge(Node *node, uint64_t addr, uint32_t nameOrIndex, EdgeType type)
{
    Node *to = nullptr;
    if (IsHeapObject(addr)) {
        to = FindNode(addr);
    } else {
        to = CreatePrimitiveNode(addr);
    }

    if (to == nullptr || to == node) {
        return;
    }
    InsertEdge(to, nameOrIndex, type);
    node->edgeCount++;
}

void RawHeapTranslateV1::CreateHClassEdge(Node *node)
{
    if (node->nodeId == node->hclass->nodeId) {
        return;
    }
    static StringId hclassStrId = InsertAndGetStringId("hclass");
    InsertEdge(node->hclass, hclassStrId, EdgeType::DEFAULT);
    node->edgeCount++;
}

EdgeType RawHeapTranslateV1::GenerateEdgeTypeAndRemoveWeak(Node *node, uint64_t &addr)
{
    EdgeType edgeType = EdgeType::DEFAULT;
    if (metaParser_->IsArray(node->jsType)) {
        edgeType = EdgeType::ELEMENT;
    }

    if (!IsHeapObject(addr)) {
        return edgeType;
    }

    if (IsWeak(addr)) {
        RemoveWeak(addr);
        edgeType = EdgeType::WEAK;
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
    for (size_t i = 6; i < size; ++i) {  // 6:normal node start from 6 (synthetic + 4 root groups + 1 metadata)
        Node *node = (*nodes)[i];
        Node *hclass = GetNextEdgeTo();
        if (hclass == nullptr) {
            LOG_ERROR_ << "missed hclass, node_id=" << node->nodeId;
            return false;
        } else if (hclass->nodeId != node->nodeId) {
            CreateEdge(node, hclass, InsertAndGetStringId("hclass"), EdgeType::DEFAULT);
        }

        CreateHashEdge(node);
        if (metaParser_->IsString(node->jsType)) {
            continue;
        }
        BuildEdges(node);
    }

    AddPrimitiveNodes();
    LOG_INFO_ << "success!";
    return true;
}

bool RawHeapTranslateV2::ReadLocalHandleRoots(FileReader &file)
{
    // 2: string table start from sections_[2]
    if (sections_[2] - sections_[0] - sections_[1] <= HANDLE_COUNT_ADDR_SIZE) {
        return true;
    }

    if (!file.CheckAndGetHeaderAt(sections_[0] + sections_[1], 0)) {
        LOG_ERROR_ << "read localhandle count error!";
        return false;
    }

    uint32_t handleCount = file.GetHeaderLeft();
    if (handleCount == 0) {
        return true;
    }

    localHandleRoots_.resize(handleCount);
    file.Seek(sections_[0] + sections_[1] + HANDLE_COUNT_ADDR_SIZE);
    if (!file.ReadArray(localHandleRoots_, handleCount)) {
        LOG_ERROR_ << "read localhandle addr error!";
        return false;
    }

    return true;
}

bool RawHeapTranslateV2::ReadGlobalHandleRoots(FileReader &file)
{
    // 2: string table start from sections_[2]
    if (sections_[2] - sections_[0] - sections_[1] <= HANDLE_COUNT_ADDR_SIZE) {
        return true;
    }
    uint64_t localHandleSize = localHandleRoots_.size() * ADDRESS_SIZE_V2;
    uint64_t offset = sections_[0] + sections_[1] + localHandleSize + HANDLE_COUNT_ADDR_SIZE;

    if (!file.CheckAndGetHeaderAt(offset, 0)) {
        LOG_ERROR_ << "read globalhandle count error!";
        return false;
    }

    uint32_t handleCount = file.GetHeaderLeft();
    if (handleCount == 0) {
        return true;
    }

    globalHandleRoots_.resize(handleCount);
    file.Seek(offset + HANDLE_COUNT_ADDR_SIZE);
    if (!file.ReadArray(globalHandleRoots_, handleCount)) {
        LOG_ERROR_ << "read globalhandle addr error!";
        return false;
    }

    return true;
}

bool RawHeapTranslateV2::ReadVmRoots(FileReader &file)
{
    // 2: string table start from sections_[2]
    uint64_t remainingSpace = sections_[2] - sections_[0] - sections_[1];
    uint64_t localHandleSize = localHandleRoots_.size() * ADDRESS_SIZE_V2;
    uint64_t globalHandleSize = globalHandleRoots_.size() * ADDRESS_SIZE_V2;
    uint64_t usedSpace = localHandleSize + globalHandleSize + 2 * HANDLE_COUNT_ADDR_SIZE;  // 2: two count fields
    // Check if remaining space contains vmRoot data
    if (remainingSpace <= usedSpace) {
        return true;
    }

    uint64_t offset = sections_[0] + sections_[1] + usedSpace;
    if (!file.CheckAndGetHeaderAt(offset, 0)) {
        LOG_ERROR_ << "read vmroot count error!";
        return false;
    }

    uint32_t rootCount = file.GetHeaderLeft();
    if (rootCount == 0) {
        return true;
    }

    vmRoots_.resize(rootCount);
    file.Seek(offset + HANDLE_COUNT_ADDR_SIZE);
    if (!file.ReadArray(vmRoots_, rootCount)) {
        LOG_ERROR_ << "read vmroot addr error!";
        return false;
    }

    return true;
}

bool RawHeapTranslateV2::ReadFrameRoots(FileReader &file)
{
    // 2: string table start from sections_[2]
    uint64_t remainingSpace = sections_[2] - sections_[0] - sections_[1];
    uint64_t localHandleSize = localHandleRoots_.size() * ADDRESS_SIZE_V2;
    uint64_t globalHandleSize = globalHandleRoots_.size() * ADDRESS_SIZE_V2;
    uint64_t vmRootsSize = vmRoots_.size() * ADDRESS_SIZE_V2;
    uint64_t usedSpace = localHandleSize + globalHandleSize + vmRootsSize +
                         3 * HANDLE_COUNT_ADDR_SIZE;  // 3: three count fields
    // Check if remaining space contains frameRoot data
    if (remainingSpace <= usedSpace) {
        return true;
    }

    uint64_t offset = sections_[0] + sections_[1] + usedSpace;
    if (!file.CheckAndGetHeaderAt(offset, 0)) {
        LOG_ERROR_ << "read frameroot count error!";
        return false;
    }

    uint32_t rootCount = file.GetHeaderLeft();
    if (rootCount == 0) {
        return true;
    }

    frameRoots_.resize(rootCount);
    file.Seek(offset + HANDLE_COUNT_ADDR_SIZE);
    if (!file.ReadArray(frameRoots_, rootCount)) {
        LOG_ERROR_ << "read frameroot addr error!";
        return false;
    }

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

    if (!ReadLocalHandleRoots(file)) {
        return false;
    }

    if (!ReadGlobalHandleRoots(file)) {
        return false;
    }

    if (!ReadVmRoots(file)) {
        return false;
    }

    if (!ReadFrameRoots(file)) {
        return false;
    }

    if (!ReadHeapMetaData(file, sections_[0])) {
        LOG_ERROR_ << "Read HeapMetaData failed!";
        return false;
    }

    AddSyntheticRootNode(roots);
    LOG_INFO_ << "root node count " << roots.size()
              << ", local handle count " << localHandleRoots_.size()
              << ", global handle count " << globalHandleRoots_.size()
              << ", vm root count " << vmRoots_.size()
              << ", frame root count " << frameRoots_.size();
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
    if (!file.CheckAndGetHeaderAt(sections_[4], 0) || file.GetHeaderRight() < sizeof(AddrTableItemV2)) {
        LOG_ERROR_ << "object table header error!";
        return false;
    }

    syntheticRoot_ = CreateNode();
    localHandleRoot_ = CreateNode();
    globalHandleRoot_ = CreateNode();
    vmRoot_ = CreateNode();
    frameRoot_ = CreateNode();
    metadataNode_ = CreateNode();
    uint32_t tableSize = file.GetHeaderLeft() * file.GetHeaderRight();
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
        tableData += file.GetHeaderRight();
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
        if (node->type == DEFAULT_NODETYPE && name.find("_GLOBAL") != std::string::npos) {
            node->type = FRAMEWORK_NODETYPE;
        }
    }
    return true;
}

void RawHeapTranslateV2::AddHandleRootEdges(const std::vector<uint32_t> &handleRoots)
{
    int index = 0;
    for (auto addr : handleRoots) {
        Node *root = FindNode(addr);
        if (root == nullptr) {
            continue;
        }
        InsertEdge(root, index++, EdgeType::ELEMENT);
    }
}

void RawHeapTranslateV2::AddSyntheticRootNode(std::vector<uint32_t> &roots)
{
    syntheticRoot_->nodeId = 1;      // 1: means root node
    syntheticRoot_->type = 9;        // 9: means SYNTHETIC node type
    syntheticRoot_->strId = InsertAndGetStringId("SyntheticRoot");
    syntheticRoot_->edgeCount = roots.size();
    StringId strId = InsertAndGetStringId("-subroot-");
    EdgeType type = EdgeType::SHORTCUT;

    CreateRootNode(localHandleRoot_, "LocalHandleRoot", localHandleRoots_.size());
    syntheticRoot_->edgeCount++;
    InsertEdge(localHandleRoot_, strId, type);

    CreateRootNode(globalHandleRoot_, "GlobalHandleRoot", globalHandleRoots_.size());
    syntheticRoot_->edgeCount++;
    InsertEdge(globalHandleRoot_, strId, type);

    CreateRootNode(vmRoot_, "VMRoot", vmRoots_.size());
    syntheticRoot_->edgeCount++;
    InsertEdge(vmRoot_, strId, type);

    CreateRootNode(frameRoot_, "FrameRoot", frameRoots_.size());
    syntheticRoot_->edgeCount++;
    InsertEdge(frameRoot_, strId, type);

    if (!spaceType_.empty() || !heapType_.empty()) {
        CreateMetadataNode(metadataNode_);
        syntheticRoot_->edgeCount++;
        InsertEdge(metadataNode_, strId, type);
    }

    for (auto addr : roots) {
        Node *root = FindNode(addr);
        if (root == nullptr) {
            continue;
        }
        InsertEdge(root, strId, type);
    }

    AddHandleRootEdges(localHandleRoots_);
    AddHandleRootEdges(globalHandleRoots_);
    AddHandleRootEdges(vmRoots_);
    AddHandleRootEdges(frameRoots_);
    if (!spaceType_.empty() || !heapType_.empty()) {
        AddMetadataPropertyNode(metadataNode_, spaceType_, "spaceType");
        AddMetadataPropertyNode(metadataNode_, heapType_, "heapType");
        AddMetadataPropertyNode(metadataNode_, vmType_, "vmType");
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
        std::string name = metaParser_->GetNodeName((*it)->jsType);
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
