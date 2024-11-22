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

#include <algorithm>
#include <sstream>
#include "ecmascript/dfx/hprof/rawheap_translate/rawheap_translate.h"
#include "ecmascript/dfx/hprof/rawheap_translate/utils.h"

namespace rawheap_translate {
bool RawHeapTranslate::Translate(const std::string &rawheapPath)
{
    uint32_t fileSize = 0;
    std::ifstream rawheapFile;
    if (!FileCheckAndOpenBinary(rawheapPath, rawheapFile, fileSize)) {
        return false;
    }

    std::vector<uint32_t> sections;
    if (!ParseMetaData(rawheapFile, fileSize) ||
        !ReadSectionInfo(rawheapFile, fileSize, sections) ||
        !ReadVersion(rawheapFile, 0, sections[0]) ||
        !ReadObjTableBySection(rawheapFile, sections) ||
        !ReadStringTable(rawheapFile, sections[2], sections[3]) ||  // 2: str offset, 3: str size
        !ReadRootTable(rawheapFile, sections[0], sections[1])) {
        return false;
    }

    FillNodesAndBuildEdges();
    return true;
}

bool RawHeapTranslate::ParseMetaData(std::ifstream &file, uint32_t &offset)
{
    cJSON *object = nullptr;
    if (!ReadMetaDataJson(file, offset, &object)) {
        return false;
    }

    bool result = meta_->Parse(object);
    DelMetaDataJson(object);
    return result;
}

bool RawHeapTranslate::ReadMetaDataJson(std::ifstream &file, uint32_t &offset, cJSON **json)
{
    auto result = CheckAndGetHead(file, offset - sizeof(uint64_t), 0);
    if (!result.has_value()) {
        LOG_ERROR("RawHeapTranslate::ReadMetaDataJson: metadata header error!");
        return false;
    }

    auto [metaOffset, metaSize] = result.value();
    std::vector<char> metaJson(metaSize);
    if (!ReadFileAtOffset(file, metaOffset, metaSize, metaJson.data())) {
        return false;
    }

    cJSON *object = cJSON_ParseWithOpts(metaJson.data(), nullptr, true);
    if (!object) {
        LOG_ERROR("RawHeapTranslate::ReadMetaDataJson: json format error!");
        return false;
    }
    
    *json = object;
    offset = metaOffset;
    return true;
}

void RawHeapTranslate::DelMetaDataJson(cJSON *json)
{
    cJSON_Delete(json);
}

bool RawHeapTranslate::ReadSectionInfo(std::ifstream &file, uint32_t endOffset, std::vector<uint32_t> &sections)
{
    auto result = CheckAndGetHead(file, endOffset - sizeof(uint64_t), sizeof(uint32_t));
    if (!result.has_value()) {
        LOG_ERROR("RawHeapTranslate::ReadSectionInfo: sections header error!");
        return false;
    }

    auto [secSize, unitSize] = result.value();
    uint32_t sectionSize = secSize * unitSize;
    std::vector<char> sectionBytes(sectionSize);
    if (!ReadFileAtOffset(file, endOffset - sectionSize - sizeof(uint64_t), sectionSize, sectionBytes.data())) {
        return false;
    }

    sections.resize(secSize);
    ByteToU32Array(sectionBytes.data(), sections.data(), secSize);
    LOG_INFO("RawHeapTranslate::ReadSectionInfo: sectionSize=" + std::to_string(secSize));
    return true;
}

bool RawHeapTranslate::ReadVersion(std::ifstream &file, uint32_t offset, uint32_t size)
{
    std::vector<char> version(size);
    if (!ReadFileAtOffset(file, offset, size, version.data())) {
        return false;
    }
    LOG_INFO("Rawheap version is " + std::string(version.data()));
    return true;
}

bool RawHeapTranslate::ReadObjTableBySection(std::ifstream &file, const std::vector<uint32_t> &sections)
{
    LOG_INFO("RawHeapTranslate::Translate: start to read objects");
    // 4: index of obj table section start from 4
    // 2: step is 2
    for (size_t secIndex = 4; secIndex < sections.size(); secIndex += 2) {
        uint32_t offset = sections[secIndex];
        uint32_t size = sections[secIndex + 1];
        if (!ReadObjTable(file, offset, size)) {
            return false;
        }
    }
    LOG_INFO("RawHeapTranslate::Translate: read objects finish!");
    return true;
}

bool RawHeapTranslate::ReadObjTable(std::ifstream &file, uint32_t offset, uint32_t size)
{
    auto result = CheckAndGetHead(file, offset, sizeof(AddrTableItem));
    if (!result.has_value()) {
        LOG_ERROR("RawHeapTranslate::ReadObjTable: obj table header error!");
        return false;
    }

    auto [objNum, unitSize] = result.value();
    uint32_t baseOffset = sizeof(uint64_t) + offset;
    std::vector<AddrTableItem> objTable;
    if (!ByteToAddrTableItem(file, baseOffset, objNum, objTable)) {
        return false;
    }

    uint32_t tableSize = objNum * unitSize;
    uint32_t curOffset = baseOffset + tableSize;
    uint32_t objMemSize = size - tableSize - sizeof(uint64_t);
    char *objMem = new char[objMemSize];
    if (!ReadFileAtOffset(file, curOffset, objMemSize, objMem)) {
        delete[] objMem;
        return false;
    }
    memBuf_.emplace_back(objMem);

    for (uint32_t i = 0; i < objNum; i++) {
        uint32_t nextOffset = i + 1 < objNum ? objTable[i + 1].offset : size - sizeof(uint64_t);
        auto actSize = nextOffset - objTable[i].offset;
        if (actSize != objTable[i].objSize && actSize != sizeof(uint64_t)) {
            LOG_ERROR("RawHeapTranslate::ReadObjTable: expected objSize=" + std::to_string(objTable[i].objSize));
            continue;
        }
        uint32_t objMemOffset = objTable[i].offset - tableSize;
        if (objMemOffset > objMemSize) {
            LOG_ERROR("RawHeapTranslate::ReadObjTable: obj out of memory buf!");
            return false;
        }
        CreateNode(objTable[i], objMem + objMemOffset);
    }

    LOG_INFO("RawHeapTranslate::ReadObjTable: read object, cnt=" + std::to_string(objNum));
    return true;
}

bool RawHeapTranslate::ReadStringTable(std::ifstream &file, uint32_t offset, uint32_t size)
{
    if (size == 0) {
        LOG_ERROR("RawHeapTranslate::ReadStringTable: string section size is 0");
        return false;
    }
    char *strSection = new char[size];
    if (!ReadFileAtOffset(file, offset, size, strSection)) {
        delete[] strSection;
        return false;
    }

    uint32_t strNum = ByteToU32(strSection);
    char *curStrSection = strSection + sizeof(uint32_t) * 2;
    uint32_t strIndex = 0;
    while (strIndex++ < strNum) {
        uint32_t strSize = ByteToU32(curStrSection);
        uint32_t objCnt = ByteToU32(curStrSection + sizeof(uint32_t));
        char *objAddr = curStrSection + sizeof(uint32_t) * 2;
        uint32_t strOffset = sizeof(uint32_t) * 2 + sizeof(uint64_t) * objCnt;
        char *str = curStrSection + strOffset;
        StringId strId = strTable_->InsertStrAndGetStringId(std::string(str));
        SetNodeStringId(objAddr, objCnt, strId);
        curStrSection += strOffset + strSize + 1;
    }

    delete[] strSection;
    LOG_INFO("RawHeapTranslate::ReadStringTable: read string table, cnt=" + std::to_string(strNum));
    return true;
}

bool RawHeapTranslate::ReadRootTable(std::ifstream &file, uint32_t offset, uint32_t size)
{
    auto result = CheckAndGetHead(file, offset, sizeof(uint64_t));
    if (!result.has_value()) {
        LOG_ERROR("RawHeapTranslate::ReadObjTable: obj table header error!");
        return false;
    }

    auto [rootNum, unitSize] = result.value();
    std::vector<char> roots(rootNum * unitSize);
    if (!ReadFileAtOffset(file, offset + sizeof(uint64_t), rootNum * unitSize, roots.data())) {
        return false;
    }

    auto syntheticRoot = std::make_shared<Node>(Node(0));
    syntheticRoot->nodeId = 1;  // 1: means root node
    syntheticRoot->type = 9;  // 9: means SYNTHETIC node type
    syntheticRoot->strId = strTable_->InsertStrAndGetStringId("SyntheticRoot");
    nodes_.insert(nodes_.begin(), syntheticRoot);
    StringId edgeStrId = strTable_->InsertStrAndGetStringId("-subroot-");
    EdgeType type = EdgeType::SHORTCUT;

    char *addr = roots.data();
    std::vector<std::shared_ptr<Edge>> rootEdges;
    for (uint32_t i = 0; i < rootNum; i++) {
        uint64_t rootAddr = ByteToU64(addr);
        addr += sizeof(uint64_t);
        auto rootNode = nodesMap_.find(rootAddr);
        if (rootNode == nodesMap_.end()) {
            continue;
        }
        auto edge = std::make_shared<Edge>(Edge(type, syntheticRoot, rootNode->second, edgeStrId));
        rootEdges.emplace_back(edge);
        syntheticRoot->edgeCount++;
    }

    edges_.insert(edges_.begin(), rootEdges.begin(), rootEdges.end());
    LOG_INFO("RawHeapTranslate::ReadRootTable: find root obj " + std::to_string(rootNum));
    return true;
}

bool RawHeapTranslate::ReadFileAtOffset(std::ifstream &file, uint32_t offset, uint32_t size, char *buf)
{
    if (buf == nullptr) {
        LOG_ERROR("RawHeapTranslate::ReadFileAtOffset: file buf is nullptr!");
        return false;
    }
    if (!file.is_open()) {
        LOG_ERROR("RawHeapTranslate::ReadFileAtOffset: file not open!");
        return false;
    }
    file.clear();
    if (!file.seekg(offset)) {
        LOG_ERROR("RawHeapTranslate::ReadFileAtOffset: file set offset failed, offset=" + std::to_string(offset));
        return false;
    }
    if (file.read(buf, size).fail()) {
        LOG_ERROR("RawHeapTranslate::ReadFileAtOffset: file read failed, offset=" + std::to_string(offset));
        return false;
    }
    return true;
}

void RawHeapTranslate::CreateNode(AddrTableItem &item, char *data)
{
    static uint32_t nodeIndex = 1;
    auto node = std::make_shared<Node>(Node(nodeIndex++));
    node->nodeId = item.id;
    node->size = item.objSize;
    node->data = data;
    nodes_.emplace_back(node);
    nodesMap_.emplace(item.addr, node);
}

void RawHeapTranslate::FillNodesAndBuildEdges()
{
    LOG_INFO("RawHeapTranslate::FillNodesAndBuildEdges: start to build edges!");
    int missNodes = 0;
    StringId hclassStrId = strTable_->InsertStrAndGetStringId("hclass");
    for (size_t i = 1; i < nodes_.size(); i++) {
        auto node = nodes_[i];
        auto result = FindNodeFromAddr(ByteToU64(node->data), nullptr);
        if (!result.has_value()) {
            missNodes++;
            continue;
        }
        auto hclass = result.value();
        FillNodes(node, hclass->data);
        if (hclass->nodeId == node->nodeId) {
            continue;
        }
        CreateEdge(node, hclass, EdgeType::DEFAULT, hclassStrId);
        if (meta_->IsString(hclass->data)) {
            continue;
        }
        BuildEdges(node, hclass->data);
    }
    LOG_INFO("RawHeapTranslate::FillNodesAndBuildEdges: build edges finish!");
    if (missNodes > 0) {
        LOG_ERROR("RawHeapTranslate::FillNodesAndBuildEdges: " + std::to_string(missNodes) + " nodes missed hclass!");
    }
}

void RawHeapTranslate::FillNodes(const std::shared_ptr<Node> &node, char *hclass)
{
    node->nativeSize = meta_->GetNativateSize(node->data, hclass);
    node->type = meta_->GetNodeTypeFromHClass(hclass);
    if (node->strId < 3 && !meta_->IsString(hclass)) {  // 3: custom strId start from 3
        std::string name = meta_->GetTypeNameFromHClass(hclass);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        node->strId = strTable_->InsertStrAndGetStringId(name);
    }
}

void RawHeapTranslate::BuildEdges(const std::shared_ptr<Node> &from, char *hclass)
{
    if (meta_->IsGlobalEnv(hclass)) {
        BuildGlobalEnvEdges(from);
        return;
    }

    std::string typeName = meta_->GetTypeNameFromHClass(hclass);
    auto visitor = [&from, this] (std::shared_ptr<MetaData> &metadata, int offset) {
        if (!metadata->IsArray()) {
            BuildFieldsEdges(from, metadata, offset);
            return;
        }
        BuildArrayEdges(from, metadata, offset);
    };
    uint32_t baseOffset = 0;
    meta_->VisitObjectBody(typeName, visitor, baseOffset);
    BuildJSObjectInlEdges(from, hclass, baseOffset);
}

void RawHeapTranslate::BuildFieldsEdges(
    const std::shared_ptr<Node> &from, const std::shared_ptr<MetaData> &metadata, int offset)
{
    for (const auto &field : metadata->fields) {
        if (field->size != sizeof(uint64_t) || offset + sizeof(uint64_t) > from->size) {
            continue;
        }
        uint64_t addr = ByteToU64(from->data + offset + field->offset);
        EdgeType type = EdgeType::DEFAULT;
        auto result = FindNodeFromAddr(addr, &type);
        if (!result.has_value()) {
            continue;
        }
        StringId strId = strTable_->InsertStrAndGetStringId(field->name);
        CreateEdge(from, result.value(), type, strId);
    }
}

void RawHeapTranslate::BuildGlobalEnvEdges(const std::shared_ptr<Node> &from)
{
    uint32_t index = sizeof(uint64_t);
    while ((index + sizeof(uint64_t)) < from->size) {
        uint64_t addr = ByteToU64(from->data + index);
        index += sizeof(uint64_t);
        auto result = FindNodeFromAddr(addr, nullptr);
        if (!result.has_value()) {
            continue;
        }
        CreateEdge(from, result.value(), EdgeType::DEFAULT, result.value()->strId);
    }
}

void RawHeapTranslate::BuildArrayEdges(
    const std::shared_ptr<Node> &from, const std::shared_ptr<MetaData> &metadata, int offset)
{
    auto lengthField = FindFieldInMetaData(metadata, "Length");
    auto dataField = FindFieldInMetaData(metadata, "Data");
    if (!lengthField || !dataField) {
        LOG_ERROR("RawHeapTranslate::BuildArrayEdges: field length or data of array not found!");
        return;
    }

    uint32_t len = ByteToU32(from->data + offset + lengthField->offset);
    uint32_t step = dataField->size;
    if (step != sizeof(uint64_t) || len <= 0 || len * step + offset > from->size) {
        return;
    }

    uint32_t index = offset + dataField->offset;
    uint32_t itemIndex = 0;
    uint32_t itemCnt = 0;
    while (itemCnt++ < len && index < from->size) {
        uint64_t addr = ByteToU64(from->data + index);
        index += step;
        auto result = FindNodeFromAddr(addr, nullptr);
        if (!result.has_value()) {
            continue;
        }
        CreateEdge(from, result.value(), EdgeType::ELEMENT, itemIndex++);
    }
}

void RawHeapTranslate::BuildDictionaryEdges(
    const std::shared_ptr<Node> &from, const std::shared_ptr<MetaData> &metadata, int offset)
{
    auto lengthField = FindFieldInMetaData(metadata, "Length");
    auto dataField = FindFieldInMetaData(metadata, "Data");
    auto layout = meta_->GetObjectLayout("Dictionary");
    if (!lengthField || !dataField || !layout) {
        LOG_ERROR("RawHeapTranslate::BuildDictionaryEdges: field length or data or layout of dictionary not found!");
        return;
    }

    uint32_t len = ByteToU32(from->data + offset + lengthField->offset);
    uint32_t step = dataField->size;
    if (step != sizeof(uint64_t) || len <= 0 || len * step + offset > from->size) {
        return;
    }

    uint32_t eleIndex = 0;
    uint32_t index = offset + dataField->offset + layout->headerSize * step;
    while (index < from->size) {
        std::vector<uint64_t> item(layout->entrySize);
        ByteToU64Array(from->data + index, item.data(), layout->entrySize);
        index += layout->entrySize * step;

        auto value = FindNodeFromAddr(item[layout->valueIndex], nullptr);
        if (!value.has_value()) {
            continue;
        }
        CreateEdge(from, value.value(), EdgeType::ELEMENT, eleIndex++);
    }
}

void RawHeapTranslate::BuildJSObjectInlEdges(const std::shared_ptr<Node> &from, char *hclass, uint32_t offset)
{
    if (!meta_->IsJSObject(hclass)) {
        return;
    }

    StringId strId = strTable_->InsertStrAndGetStringId("InlineProperty");
    uint32_t propOffset = offset;
    while (propOffset + sizeof(uint64_t) <= from->size) {
        uint64_t addr = ByteToU64(from->data + propOffset);
        propOffset += sizeof(uint64_t);
        auto result = FindNodeFromAddr(addr, nullptr);
        if (!result.has_value()) {
            continue;
        }
        CreateEdge(from, result.value(), EdgeType::DEFAULT, strId);
    }
}

void RawHeapTranslate::CreateEdge(
    const std::shared_ptr<Node> &from, const std::shared_ptr<Node> &to, EdgeType type, uint32_t index)
{
    auto edge = std::make_shared<Edge>(Edge(type, from, to, index));
    edges_.emplace_back(edge);
    from->edgeCount++;
}

void RawHeapTranslate::SetNodeStringId(char *addr, uint32_t size, StringId strId)
{
    char *objAddr = addr;
    for (uint32_t i = 0; i < size; i++) {
        auto node = nodesMap_.find(ByteToU64(objAddr));
        if (node != nodesMap_.end()) {
            node->second->strId = strId;
        }
        objAddr += sizeof(uint64_t);
    }
}

bool RawHeapTranslate::ByteToAddrTableItem(
    std::ifstream &file, uint32_t offset, uint32_t objNum, std::vector<AddrTableItem> &table)
{
    uint32_t unitSize = sizeof(AddrTableItem);
    uint32_t tableSize = objNum * unitSize;
    std::vector<char> tableByte(tableSize);
    if (!ReadFileAtOffset(file, offset, tableSize, tableByte.data())) {
        return false;
    }
    table.resize(objNum);
    char *item = tableByte.data();
    for (uint32_t i = 0; i < objNum; i++) {
        table[i].addr = ByteToU64(item);
        item += sizeof(uint64_t);

        table[i].id = ByteToU64(item);
        item += sizeof(uint64_t);

        table[i].objSize = ByteToU32(item);
        item += sizeof(uint32_t);

        table[i].offset = ByteToU32(item);
        item += sizeof(uint32_t);
    }
    return true;
}

std::optional<std::shared_ptr<Node>> RawHeapTranslate::FindNodeFromAddr(uint64_t addr, EdgeType *type)
{
    if (!IsHeapObject(addr)) {
        return std::nullopt;
    }
    CheckAndRemoveWeak(addr, type);
    auto node = nodesMap_.find(addr);
    if (node == nodesMap_.end()) {
        return std::nullopt;
    }
    return node->second;
}

std::optional<std::pair<uint32_t, uint32_t>> RawHeapTranslate::CheckAndGetHead(
    std::ifstream &file, uint32_t offset, uint32_t assertNum)
{
    uint32_t headSize = sizeof(uint64_t);
    std::vector<char> head(headSize);
    if (!ReadFileAtOffset(file, offset, headSize, head.data())) {
        return std::nullopt;
    }

    uint32_t firstNum = ByteToU32(head.data());
    uint32_t secondNum = ByteToU32(head.data() + sizeof(uint32_t));
    if (assertNum != 0 && secondNum != assertNum) {
        return std::nullopt;
    }
    
    return std::pair<uint32_t, uint32_t>(firstNum, secondNum);
}

std::shared_ptr<Field> RawHeapTranslate::FindFieldInMetaData(
    const std::shared_ptr<MetaData> &metadata, const std::string &name)
{
    for (const auto &field : metadata->fields) {
        if (field->name == name) {
            return field;
        }
    }
    return nullptr;
}

void RawHeapTranslate::CheckAndRemoveWeak(uint64_t &addr, EdgeType *type)
{
    if ((addr & TAG_WEAK_MASK) == TAG_WEAK) {
        addr &= (~TAG_WEAK);
        if (type != nullptr) {
            *type = EdgeType::WEAK;
        }
    }
}

bool RawHeapTranslate::IsHeapObject(uint64_t addr)
{
    return ((addr & TAG_HEAPOBJECT_MASK) == 0U);
}
} // namespace rawheap_translate