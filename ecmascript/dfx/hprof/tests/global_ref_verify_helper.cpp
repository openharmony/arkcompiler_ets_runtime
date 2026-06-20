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

#include "global_ref_verify_helper.h"

#include <cctype>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace panda::test {

namespace {

constexpr int NODE_FIELDS = 8;
constexpr int EDGE_FIELDS = 3;
constexpr int NODE_NAME_INDEX = 1;
constexpr int NODE_EDGE_COUNT_INDEX = 4;
constexpr int EDGE_TYPE_INDEX = 0;
constexpr int EDGE_NAME_OR_INDEX = 1;
constexpr int EDGE_TO_NODE_INDEX = 2;

constexpr int EDGE_ELEMENT = 1;
constexpr int EDGE_PROPERTY = 2;
constexpr int EDGE_SHORTCUT = 5;

struct HeapSnapshotData {
    const std::vector<int64_t> &nodes;
    const std::vector<int64_t> &edges;
    const std::vector<int> &edgeOffset;
    const std::vector<std::string> &strings;
    int nodeCount;
};

struct EdgeInfo {
    int type;
    int nameId;
    int toNode;
};

char DecodeJsonEscape(char c)
{
    switch (c) {
        case 'n':
            return '\n';
        case 't':
            return '\t';
        case 'r':
            return '\r';
        case '\\':
            return '\\';
        case '"':
            return '"';
        default:
            return c;
    }
}

bool LoadFileContent(const std::string &filePath, std::string &out)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return true;
}

std::vector<int> BuildEdgeOffsets(const std::vector<int64_t> &nodes, int nodeCount)
{
    std::vector<int> offsets(nodeCount + 1, 0);
    for (int i = 0; i < nodeCount; i++) {
        offsets[i + 1] = offsets[i] +
            static_cast<int>(nodes[i * NODE_FIELDS + NODE_EDGE_COUNT_INDEX]);
    }
    return offsets;
}

std::string GetNodeName(const HeapSnapshotData &snap, int idx)
{
    int nameId = static_cast<int>(snap.nodes[idx * NODE_FIELDS + NODE_NAME_INDEX]);
    if (nameId < 0 || nameId >= static_cast<int>(snap.strings.size())) {
        return "";
    }
    return snap.strings[nameId];
}

EdgeInfo GetEdgeInfo(const HeapSnapshotData &snap, int edgeIdx)
{
    int base = edgeIdx * EDGE_FIELDS;
    int toNodeRaw = static_cast<int>(snap.edges[base + EDGE_TO_NODE_INDEX]) / NODE_FIELDS;
    return {
        static_cast<int>(snap.edges[base + EDGE_TYPE_INDEX]),
        static_cast<int>(snap.edges[base + EDGE_NAME_OR_INDEX]),
        toNodeRaw,
    };
}

bool IsReferenceAddressNodeName(const std::string &name)
{
    return name.find("ReferenceAddress:0x") != std::string::npos;
}

int FindGlobalHandleObjectNode(const HeapSnapshotData &snap)
{
    for (int i = 0; i < snap.nodeCount; i++) {
        if (GetNodeName(snap, i).find("GlobalHandleObject") != std::string::npos) {
            return i;
        }
    }
    return -1;
}

bool CheckReferenceEdges(const HeapSnapshotData &snap, int ghoIdx, std::string &errorMsg)
{
    int ghoEdgeStart = snap.edgeOffset[ghoIdx];
    int ghoEdgeCount = static_cast<int>(snap.nodes[ghoIdx * NODE_FIELDS + NODE_EDGE_COUNT_INDEX]);
    int elemEdgeCount = 0;
    for (int j = ghoEdgeStart; j < ghoEdgeStart + ghoEdgeCount; j++) {
        EdgeInfo info = GetEdgeInfo(snap, j);
        if (info.type != EDGE_ELEMENT) {
            continue;
        }
        if (info.toNode >= snap.nodeCount) {
            errorMsg = "GlobalHandleObject ELEMENT edge points out of range";
            return false;
        }
        std::string virtName = GetNodeName(snap, info.toNode);
        if (!IsReferenceAddressNodeName(virtName)) {
            errorMsg = "GlobalHandleObject ELEMENT edge does not point to ReferenceAddress node: " + virtName;
            return false;
        }
        int virtEdgeStart = snap.edgeOffset[info.toNode];
        int virtEdgeCount = static_cast<int>(snap.nodes[info.toNode * NODE_FIELDS + NODE_EDGE_COUNT_INDEX]);
        if (virtEdgeCount != 1) {
            errorMsg = "Virtual ReferenceAddress node has edgeCount != 1: " + std::to_string(virtEdgeCount);
            return false;
        }
        EdgeInfo virtEdge = GetEdgeInfo(snap, virtEdgeStart);
        if (virtEdge.type != EDGE_PROPERTY) {
            errorMsg = "Virtual ReferenceAddress node's edge is not PROPERTY";
            return false;
        }
        if (virtEdge.nameId < 0 || virtEdge.nameId >= static_cast<int>(snap.strings.size()) ||
            snap.strings[virtEdge.nameId] != "JSObject") {
            errorMsg = "Virtual ReferenceAddress node's PROPERTY edge is not named 'JSObject'";
            return false;
        }
        if (virtEdge.toNode == 0) {
            errorMsg = "Virtual ReferenceAddress PROPERTY edge points to SyntheticRoot";
            return false;
        }
        elemEdgeCount++;
    }
    if (elemEdgeCount == 0) {
        errorMsg = "GlobalHandleObject has no ELEMENT edges to ReferenceAddress nodes";
        return false;
    }
    return true;
}

bool CheckNoShortcutToReferenceAddress(const HeapSnapshotData &snap, std::string &errorMsg)
{
    int synEdgeStart = snap.edgeOffset[0];
    int synEdgeCount = static_cast<int>(snap.nodes[NODE_EDGE_COUNT_INDEX]);
    for (int j = synEdgeStart; j < synEdgeStart + synEdgeCount; j++) {
        EdgeInfo info = GetEdgeInfo(snap, j);
        if (info.type != EDGE_SHORTCUT || info.toNode >= snap.nodeCount) {
            continue;
        }
        if (IsReferenceAddressNodeName(GetNodeName(snap, info.toNode))) {
            errorMsg = "SHORTCUT from SyntheticRoot to ReferenceAddress - top level!";
            return false;
        }
    }
    return true;
}

}  // namespace

bool ParseJsonNumberArray(const std::string &json, const std::string &key, std::vector<int64_t> &values)
{
    std::string searchKey = "\"" + key + "\":[";
    auto start = json.find(searchKey);
    if (start == std::string::npos) {
        return false;
    }
    size_t pos = start + searchKey.size();
    while (pos < json.size() && json[pos] != ']') {
        if (json[pos] == ',' || json[pos] == ' ' || json[pos] == '\n') {
            pos++;
            continue;
        }
        size_t numEnd = pos;
        if (json[pos] == '-') {
            numEnd++;
        }
        while (numEnd < json.size() && (std::isdigit(json[numEnd]) || json[numEnd] == 'e' ||
                json[numEnd] == 'E' || json[numEnd] == '+' || json[numEnd] == '-')) {
            numEnd++;
        }
        if (numEnd == pos) {
            pos++;
            continue;
        }
        values.push_back(std::stoll(json.substr(pos, numEnd - pos)));
        pos = numEnd;
    }
    return !values.empty();
}

bool ParseJsonStringArray(const std::string &json, const std::string &key, std::vector<std::string> &values)
{
    std::string searchKey = "\"" + key + "\":[";
    auto start = json.find(searchKey);
    if (start == std::string::npos) {
        return false;
    }
    size_t pos = start + searchKey.size();
    while (pos < json.size() && json[pos] != ']') {
        if (json[pos] != '"') {
            pos++;
            continue;
        }
        pos++;
        std::string s;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                pos++;
                s += DecodeJsonEscape(json[pos]);
            } else {
                s += json[pos];
            }
            pos++;
        }
        values.push_back(s);
        pos++;
    }
    return !values.empty();
}

bool VerifyGlobalHandleObjectTree(const std::string &filePath, std::string &errorMsg)
{
    std::string json;
    if (!LoadFileContent(filePath, json)) {
        errorMsg = "Failed to open file: " + filePath;
        return false;
    }

    std::vector<int64_t> nodes;
    std::vector<int64_t> edges;
    std::vector<std::string> strings;
    if (!ParseJsonNumberArray(json, "nodes", nodes)) {
        errorMsg = "Failed to parse nodes";
        return false;
    }
    if (!ParseJsonNumberArray(json, "edges", edges)) {
        errorMsg = "Failed to parse edges";
        return false;
    }
    if (!ParseJsonStringArray(json, "strings", strings)) {
        errorMsg = "Failed to parse strings";
        return false;
    }

    int nodeCount = static_cast<int>(nodes.size()) / NODE_FIELDS;
    HeapSnapshotData snap {nodes, edges, BuildEdgeOffsets(nodes, nodeCount), strings, nodeCount};

    int ghoIdx = FindGlobalHandleObjectNode(snap);
    if (ghoIdx < 0) {
        errorMsg = "GlobalHandleObject node not found";
        return false;
    }

    if (!CheckReferenceEdges(snap, ghoIdx, errorMsg)) {
        return false;
    }

    return CheckNoShortcutToReferenceAddress(snap, errorMsg);
}

}  // namespace panda::test
