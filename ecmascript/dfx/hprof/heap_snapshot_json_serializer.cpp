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

#include "ecmascript/dfx/hprof/heap_snapshot_json_serializer.h"

#include "ecmascript/dfx/hprof/heap_snapshot.h"

namespace panda::ecmascript {

bool HeapSnapshotJSONSerializer::Serialize(HeapSnapshot *snapshot, Stream *stream)
{
    // Serialize Node/Edge/String-Table
    LOG_ECMA(INFO) << "HeapSnapshotJSONSerializer::Serialize begin";
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "HeapSnapshotJSONSerializer::Serialize");
    ASSERT(snapshot->GetNodes() != nullptr && snapshot->GetEdges() != nullptr &&
           snapshot->GetEcmaStringTable() != nullptr);
    auto writer = new StreamWriter(stream);

    SerializeSnapshotHeader(snapshot, writer);     // 1.
    SerializeNodes(snapshot, writer);              // 2.
    SerializeEdges(snapshot, writer);              // 3.
    SerializeTraceFunctionInfo(snapshot, writer);  // 4.
    SerializeTraceTree(snapshot, writer);          // 5.
    SerializeSamples(snapshot, writer);            // 6.
    SerializeLocations(writer);          // 7.
    SerializeStringTable(snapshot, writer);        // 8.
    SerializerSnapshotClosure(writer);   // 9.
    writer->End();

    delete writer;

    LOG_ECMA(INFO) << "HeapSnapshotJSONSerializer::Serialize exit";
    return true;
}

/*
4 byte: str_num
4 byte: unuse
{
4 byte: string size
4 byte: obj_num
[8 byte: obj adrr] * obj_num
string contents
} * str_num
*/
uint32_t HeapSnapshotJSONSerializer::DumpStringTable(const StringHashMap *stringTable, Stream *stream,
                                                     CUnorderedMap<uint64_t, CVector<uint64_t>> &strIdMapObjVec)
{
    ASSERT(stringTable != nullptr);
    size_t bufSize = 8 * 1024 * 1024; // 8MB buf use for string dump
    char *buf = new char[bufSize];
    uint32_t secHead[] = {stringTable->GetCapcity(), 0};
    *reinterpret_cast<uint64_t *>(buf) = *reinterpret_cast<uint64_t *>(secHead);
    uint32_t secTotalSize = sizeof(secHead);
    size_t offset = sizeof(secHead);
    for (auto key : stringTable->GetOrderedKeyStorage()) {
        auto [strId, str] = stringTable->GetStringAndIdPair(key);
        auto objVec = strIdMapObjVec[strId];
        uint32_t objVecSize = objVec.size() * sizeof(uint64_t);
        uint32_t strHead[] = {str->size(), objVec.size()};
        auto currLen = sizeof(strHead) + objVecSize + str->size() + 1;
        if (offset + currLen > bufSize) {
            stream->WriteBinBlock(buf, offset);
            offset = 0;
            if (currLen > bufSize) {
                delete[] buf;
                bufSize = currLen;
                buf = new char[bufSize];
            }
        }
        *reinterpret_cast<uint64_t *>(buf + offset) = *reinterpret_cast<uint64_t *>(strHead);
        offset += sizeof(strHead);
        if (memcpy_s(buf + offset, bufSize - offset, reinterpret_cast<void *>(objVec.data()), objVecSize) != EOK) {
            LOG_ECMA(ERROR) << "DumpStringTable: memcpy_s failed";
            break;
        }
        offset += objVecSize;
        if (memcpy_s(buf + offset, bufSize - offset, str->data(), str->size() + 1) != EOK) {
            LOG_ECMA(ERROR) << "DumpStringTable: memcpy_s failed";
            break;
        }
        offset += str->size() + 1;
        secTotalSize += currLen;
    }
    auto padding = (8 - secTotalSize % 8) % 8;
    if (offset + padding > 0) {
        stream->WriteBinBlock(buf, offset + padding);
    }
    delete[] buf;
    return secTotalSize + padding;
}

void HeapSnapshotJSONSerializer::SerializeSnapshotHeader(HeapSnapshot *snapshot, StreamWriter *writer)
{
    writer->WriteString("{\"snapshot\":\n");  // 1.
    writer->WriteString("{\"meta\":\n");      // 2.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->WriteString("{\"node_fields\":[\"type\",\"name\",\"id\",\"self_size\",\"edge_count\",\"trace_node_id\",");
    writer->WriteString("\"detachedness\",\"native_size\"],\n");  // 3.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->WriteString("\"node_types\":[[\"hidden\",\"array\",\"string\",\"object\",\"code\",\"closure\",\"regexp\",");
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->WriteString("\"number\",\"native\",\"synthetic\",\"concatenated string\",\"slicedstring\",\"symbol\",");
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->WriteString("\"bigint\",\"framework\"],\"string\",\"number\",\"number\",\"number\",\"number\",");
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->WriteString("\"number\"],\n");  // 4.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->WriteString("\"edge_fields\":[\"type\",\"name_or_index\",\"to_node\"],\n");  // 5.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->WriteString("\"edge_types\":[[\"context\",\"element\",\"property\",\"internal\",\"hidden\",\"shortcut\",");
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->WriteString("\"weak\"],\"string_or_number\",\"node\"],\n");  // 6.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->WriteString("\"trace_function_info_fields\":[\"function_id\",\"name\",\"script_name\",\"script_id\",");
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->WriteString("\"line\",\"column\"],\n");  // 7.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->WriteString("\"trace_node_fields\":[\"id\",\"function_info_index\",\"count\",\"size\",\"children\"],\n");
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->WriteString("\"sample_fields\":[\"timestamp_us\",\"last_assigned_id\"],\n");  // 9.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    // 10.
    writer->WriteString("\"location_fields\":[\"object_index\",\"script_id\",\"line\",\"column\"]},\n\"node_count\":");
    writer->WriteNumber(snapshot->GetNodeCount());                         // 11.
    writer->WriteString(",\n\"edge_count\":");
    writer->WriteNumber(snapshot->GetEdgeCount());                         // 12.
    writer->WriteString(",\n\"trace_function_count\":");
    writer->WriteNumber(snapshot->GetTrackAllocationsStack().size());   // 13.
    writer->WriteString("\n},\n");  // 14.
}

void HeapSnapshotJSONSerializer::SerializeNodes(HeapSnapshot *snapshot, StreamWriter *writer)
{
    LOG_ECMA(INFO) << "HeapSnapshotJSONSerializer::SerializeNodes";
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "HeapSnapshotJSONSerializer::SerializeNodes");
    const CList<Node *> *nodes = snapshot->GetNodes();
    const StringHashMap *stringTable = snapshot->GetEcmaStringTable();
    ASSERT(nodes != nullptr);
    writer->WriteString("\"nodes\":[");  // Section Header
    size_t i = 0;
    for (auto *node : *nodes) {
        if (i > 0) {
            writer->WriteChar(',');  // add comma except first line
        }
        writer->WriteNumber(static_cast<int>(node->GetType()));  // 1.
        writer->WriteChar(',');
        writer->WriteNumber(stringTable->GetStringId(node->GetName()));                      // 2.
        writer->WriteChar(',');
        writer->WriteNumber(node->GetId());                                                  // 3.
        writer->WriteChar(',');
        writer->WriteNumber(node->GetSelfSize());                                            // 4.
        writer->WriteChar(',');
        writer->WriteNumber(node->GetEdgeCount());                                           // 5.
        writer->WriteChar(',');
        writer->WriteNumber(node->GetStackTraceId());                                        // 6.
        writer->WriteChar(',');
        writer->WriteChar('0');                                                              // 7.detachedness default 0
        writer->WriteChar(',');
        writer->WriteNumber(node->GetNativeSize());
        if (i == nodes->size() - 1) {    // add comma at last the line
            writer->WriteString("],\n"); // 7. detachedness default
        } else {
            writer->WriteString("\n");   // 7.
        }
        i++;
    }
}

void HeapSnapshotJSONSerializer::SerializeEdges(HeapSnapshot *snapshot, StreamWriter *writer)
{
    LOG_ECMA(INFO) << "HeapSnapshotJSONSerializer::SerializeEdges";
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "HeapSnapshotJSONSerializer::SerializeEdges");
    const CList<Edge *> *edges = snapshot->GetEdges();
    const StringHashMap *stringTable = snapshot->GetEcmaStringTable();
    ASSERT(edges != nullptr);
    writer->WriteString("\"edges\":[");
    size_t i = 0;
    for (auto *edge : *edges) {
        StringId nameOrIndex = edge->GetType() == EdgeType::ELEMENT ?
            edge->GetIndex() : stringTable->GetStringId(edge->GetName());
        if (i > 0) {  // add comma except the first line
            writer->WriteChar(',');
        }
        writer->WriteNumber(static_cast<int>(edge->GetType()));          // 1.
        writer->WriteChar(',');
        writer->WriteNumber(nameOrIndex);  // 2. Use StringId
        writer->WriteChar(',');

        if (i == edges->size() - 1) {  // add comma at last the line
            writer->WriteNumber(edge->GetTo()->GetIndex() * Node::NODE_FIELD_COUNT);  // 3.
            writer->WriteString("],\n");
        } else {
            writer->WriteNumber(edge->GetTo()->GetIndex() * Node::NODE_FIELD_COUNT);    // 3.
            writer->WriteChar('\n');
        }
        i++;
    }
}

void HeapSnapshotJSONSerializer::SerializeTraceFunctionInfo(HeapSnapshot *snapshot, StreamWriter *writer)
{
    const CVector<FunctionInfo> trackAllocationsStack = snapshot->GetTrackAllocationsStack();
    const StringHashMap *stringTable = snapshot->GetEcmaStringTable();

    writer->WriteString("\"trace_function_infos\":[");  // Empty
    size_t i = 0;

    for (const auto &info : trackAllocationsStack) {
        if (i > 0) {  // add comma except the first line
            writer->WriteChar(',');
        }
        writer->WriteNumber(info.functionId);
        writer->WriteChar(',');
        CString functionName(info.functionName.c_str());
        writer->WriteNumber(stringTable->GetStringId(&functionName));
        writer->WriteChar(',');
        CString scriptName(info.scriptName.c_str());
        writer->WriteNumber(stringTable->GetStringId(&scriptName));
        writer->WriteChar(',');
        writer->WriteNumber(info.scriptId);
        writer->WriteChar(',');
        writer->WriteNumber(info.lineNumber);
        writer->WriteChar(',');
        writer->WriteNumber(info.columnNumber);
        writer->WriteChar('\n');
        i++;
    }
    writer->WriteString("],\n");
}

void HeapSnapshotJSONSerializer::SerializeTraceTree(HeapSnapshot *snapshot, StreamWriter *writer)
{
    writer->WriteString("\"trace_tree\":[");
    TraceTree* tree = snapshot->GetTraceTree();
    if ((tree != nullptr) && (snapshot->trackAllocations())) {
        SerializeTraceNode(tree->GetRoot(), writer);
    }
    writer->WriteString("],\n");
}

void HeapSnapshotJSONSerializer::SerializeTraceNode(TraceNode* node, StreamWriter *writer)
{
    if (node == nullptr) {
        return;
    }

    writer->WriteNumber(node->GetId());
    writer->WriteChar(',');
    writer->WriteNumber(node->GetNodeIndex());
    writer->WriteChar(',');
    writer->WriteNumber(node->GetTotalCount());
    writer->WriteChar(',');
    writer->WriteNumber(node->GetTotalSize());
    writer->WriteString(",[");

    int i = 0;
    for (TraceNode* child : node->GetChildren()) {
        if (i > 0) {
            writer->WriteChar(',');
        }
        SerializeTraceNode(child, writer);
        i++;
    }
    writer->WriteChar(']');
}

void HeapSnapshotJSONSerializer::SerializeSamples(HeapSnapshot *snapshot, StreamWriter *writer)
{
    writer->WriteString("\"samples\":[");
    const CVector<TimeStamp> &timeStamps = snapshot->GetTimeStamps();
    if (!timeStamps.empty()) {
        auto firstTimeStamp = timeStamps[0];
        bool isFirst = true;
        for (auto timeStamp : timeStamps) {
            if (!isFirst) {
                writer->WriteString("\n, ");
            } else {
                isFirst = false;
            }
            writer->WriteNumber(timeStamp.GetTimeStamp() - firstTimeStamp.GetTimeStamp());
            writer->WriteString(", ");
            writer->WriteNumber(timeStamp.GetLastSequenceId());
        }
    }
    writer->WriteString("],\n");
}

void HeapSnapshotJSONSerializer::SerializeLocations(StreamWriter *writer)
{
    writer->WriteString("\"locations\":[],\n");
}

void HeapSnapshotJSONSerializer::SerializeStringTable(HeapSnapshot *snapshot, StreamWriter *writer)
{
    LOG_ECMA(INFO) << "HeapSnapshotJSONSerializer::SerializeStringTable";
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "HeapSnapshotJSONSerializer::SerializeStringTable");
    const StringHashMap *stringTable = snapshot->GetEcmaStringTable();
    ASSERT(stringTable != nullptr);
    writer->WriteString("\"strings\":[\"<dummy>\",\n");
    writer->WriteString("\"\",\n");
    writer->WriteString("\"GC roots\",\n");
    // StringId Range from 3
    size_t capcity = stringTable->GetCapcity();
    ASSERT(capcity > 0);
    size_t i = 0;
    for (auto key : stringTable->GetOrderedKeyStorage()) {
        if (i == capcity - 1) {
            writer->WriteChar('\"');
            SerializeString(stringTable->GetStringByKey(key), writer); // No Comma for the last line
            writer->WriteString("\"\n");
        } else {
            writer->WriteChar('\"');
            SerializeString(stringTable->GetStringByKey(key), writer);
            writer->WriteString("\",\n");
        }
        i++;
    }
    writer->WriteString("]\n");
}

void HeapSnapshotJSONSerializer::SerializeString(CString *str, StreamWriter *writer)
{
    if (str == nullptr || writer == nullptr) {
        return;
    }
    const char *s = str->c_str();
    while (*s != '\0') {
        if (*s == '\"' || *s == '\\') {
            writer->WriteChar('\\');
            writer->WriteChar(*s);
            s++;
        } else if (*s == '\n') {
            writer->WriteString("\\n");
            s++;
        } else if (*s == '\b') {
            writer->WriteString("\\b");
            s++;
        } else if (*s == '\f') {
            writer->WriteString("\\f");
            s++;
        } else if (*s == '\r') {
            writer->WriteString("\\r");
            s++;
        } else if (*s == '\t') {
            writer->WriteString("\\t");
            s++;
        } else if (*s > ASCII_US && *s < ASCII_DEL) {
            writer->WriteChar(*s);
            s++;
        } else if (*s <= ASCII_US || *s == ASCII_DEL) {
            // special char convert to \u unicode
            SerializeUnicodeChar(static_cast<uint32_t>(*s), writer);
            s++;
        } else {
            // convert utf-8 to \u unicode
            size_t len = 1;
            while (len <= UTF8_MAX_BYTES && *(s + len) != '\0') {
                len++;
            }
            auto [unicode, bytes] =
                base::utf_helper::ConvertUtf8ToUnicodeChar(reinterpret_cast<const uint8_t *>(s), len);
            if (unicode == base::utf_helper::INVALID_UTF8) {
                LOG_ECMA(WARN) << "HeapSnapshotJSONSerializer::SerializeString, str is not utf-8";
                writer->WriteChar('?');
                s++;
            } else {
                SerializeUnicodeChar(unicode, writer);
                s += bytes;
            }
        }
    }
}

void HeapSnapshotJSONSerializer::SerializeUnicodeChar(uint32_t unicodeChar, StreamWriter *writer)
{
    static const char hexChars[] = "0123456789ABCDEF";
    writer->WriteString("\\u");
    writer->WriteChar(hexChars[(unicodeChar >> 0xC) & 0xF]);
    writer->WriteChar(hexChars[(unicodeChar >> 0x8) & 0xF]);
    writer->WriteChar(hexChars[(unicodeChar >> 0x4) & 0xF]);
    writer->WriteChar(hexChars[unicodeChar & 0xF]);
}

void HeapSnapshotJSONSerializer::SerializerSnapshotClosure(StreamWriter *writer)
{
    writer->WriteString("}\n");
}
}  // namespace panda::ecmascript
