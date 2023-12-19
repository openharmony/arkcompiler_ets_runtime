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
#include "ecmascript/dfx/hprof/string_hashmap.h"

namespace panda::ecmascript {

bool HeapSnapshotJSONSerializer::Serialize(HeapSnapshot *snapshot, Stream *stream)
{
    // Serialize Node/Edge/String-Table
    LOG_ECMA(INFO) << "HeapSnapshotJSONSerializer::Serialize begin";
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

void HeapSnapshotJSONSerializer::SerializeSnapshotHeader(HeapSnapshot *snapshot, StreamWriter *writer)
{
    writer->Write("{\"snapshot\":\n");  // 1.
    writer->Write("{\"meta\":\n");      // 2.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->Write("{\"node_fields\":[\"type\",\"name\",\"id\",\"self_size\",\"edge_count\",\"trace_node_id\",");
    writer->Write("\"detachedness\"],\n");  // 3.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->Write("\"node_types\":[[\"hidden\",\"array\",\"string\",\"object\",\"code\",\"closure\",\"regexp\",");
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->Write("\"number\",\"native\",\"synthetic\",\"concatenated string\",\"slicedstring\",\"symbol\",");
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->Write("\"bigint\"],\"string\",\"number\",\"number\",\"number\",\"number\",\"number\"],\n");  // 4.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->Write("\"edge_fields\":[\"type\",\"name_or_index\",\"to_node\"],\n");  // 5.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->Write("\"edge_types\":[[\"context\",\"element\",\"property\",\"internal\",\"hidden\",\"shortcut\",");
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->Write("\"weak\"],\"string_or_number\",\"node\"],\n");  // 6.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->Write("\"trace_function_info_fields\":[\"function_id\",\"name\",\"script_name\",\"script_id\",");
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->Write("\"line\",\"column\"],\n");  // 7.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->Write("\"trace_node_fields\":[\"id\",\"function_info_index\",\"count\",\"size\",\"children\"],\n");
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer->Write("\"sample_fields\":[\"timestamp_us\",\"last_assigned_id\"],\n");  // 9.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    // 10.
    writer->Write("\"location_fields\":[\"object_index\",\"script_id\",\"line\",\"column\"]},\n\"node_count\":");
    writer->Write(snapshot->GetNodeCount());                         // 11.
    writer->Write(",\n\"edge_count\":");
    writer->Write(snapshot->GetEdgeCount());                         // 12.
    writer->Write(",\n\"trace_function_count\":");
    writer->Write(snapshot->GetTrackAllocationsStack().size());   // 13.
    writer->Write("\n},\n");  // 14.
}

void HeapSnapshotJSONSerializer::SerializeNodes(HeapSnapshot *snapshot, StreamWriter *writer)
{
    const CList<Node *> *nodes = snapshot->GetNodes();
    const StringHashMap *stringTable = snapshot->GetEcmaStringTable();
    ASSERT(nodes != nullptr);
    writer->Write("\"nodes\":[");  // Section Header
    size_t i = 0;
    for (auto *node : *nodes) {
        if (i > 0) {
            writer->Write(",");  // add comma except first line
        }
        writer->Write(static_cast<int>(node->GetType()));  // 1.
        writer->Write(",");
        writer->Write(stringTable->GetStringId(node->GetName()));                      // 2.
        writer->Write(",");
        writer->Write(node->GetId());                                                  // 3.
        writer->Write(",");
        writer->Write(node->GetSelfSize());                                            // 4.
        writer->Write(",");
        writer->Write(node->GetEdgeCount());                                           // 5.
        writer->Write(",");
        writer->Write(node->GetStackTraceId());                                        // 6.
        writer->Write(",");
        if (i == nodes->size() - 1) {  // add comma at last the line
            writer->Write("0],\n"); // 7. detachedness default
        } else {
            writer->Write("0\n");  // 7.
        }
        i++;
    }
}

void HeapSnapshotJSONSerializer::SerializeEdges(HeapSnapshot *snapshot, StreamWriter *writer)
{
    const CList<Edge *> *edges = snapshot->GetEdges();
    const StringHashMap *stringTable = snapshot->GetEcmaStringTable();
    ASSERT(edges != nullptr);
    writer->Write("\"edges\":[");
    size_t i = 0;
    for (auto *edge : *edges) {
        StringId nameOrIndex = edge->GetType() == EdgeType::ELEMENT ?
            edge->GetIndex() : stringTable->GetStringId(edge->GetName());
        if (i > 0) {  // add comma except the first line
            writer->Write(",");
        }
        writer->Write(static_cast<int>(edge->GetType()));          // 1.
        writer->Write(",");
        writer->Write(nameOrIndex);  // 2. Use StringId
        writer->Write(",");

        if (i == edges->size() - 1) {  // add comma at last the line
            writer->Write(edge->GetTo()->GetIndex() * Node::NODE_FIELD_COUNT);  // 3.
            writer->Write("],\n");
        } else {
            writer->Write(edge->GetTo()->GetIndex() * Node::NODE_FIELD_COUNT);    // 3.
            writer->Write("\n");
        }
        i++;
    }
}

void HeapSnapshotJSONSerializer::SerializeTraceFunctionInfo(HeapSnapshot *snapshot, StreamWriter *writer)
{
    const CVector<FunctionInfo> trackAllocationsStack = snapshot->GetTrackAllocationsStack();
    const StringHashMap *stringTable = snapshot->GetEcmaStringTable();

    writer->Write("\"trace_function_infos\":[");  // Empty
    size_t i = 0;

    for (const auto &info : trackAllocationsStack) {
        if (i > 0) {  // add comma except the first line
            writer->Write(",");
        }
        writer->Write(info.functionId);
        writer->Write(",");
        CString functionName(info.functionName.c_str());
        writer->Write(stringTable->GetStringId(&functionName));
        writer->Write(",");
        CString scriptName(info.scriptName.c_str());
        writer->Write(stringTable->GetStringId(&scriptName));
        writer->Write(",");
        writer->Write(info.scriptId);
        writer->Write(",");
        writer->Write(info.lineNumber);
        writer->Write(",");
        writer->Write(info.columnNumber);
        writer->Write("\n");
        i++;
    }
    writer->Write("],\n");
}

void HeapSnapshotJSONSerializer::SerializeTraceTree(HeapSnapshot *snapshot, StreamWriter *writer)
{
    writer->Write("\"trace_tree\":[");
    TraceTree* tree = snapshot->GetTraceTree();
    if ((tree != nullptr) && (snapshot->trackAllocations())) {
        SerializeTraceNode(tree->GetRoot(), writer);
    }
    writer->Write("],\n");
}

void HeapSnapshotJSONSerializer::SerializeTraceNode(TraceNode* node, StreamWriter *writer)
{
    if (node == nullptr) {
        return;
    }

    writer->Write(node->GetId());
    writer->Write(",");
    writer->Write(node->GetNodeIndex());
    writer->Write(",");
    writer->Write(node->GetTotalCount());
    writer->Write(",");
    writer->Write(node->GetTotalSize());
    writer->Write(",[");

    int i = 0;
    for (TraceNode* child : node->GetChildren()) {
        if (i > 0) {
            writer->Write(",");
        }
        SerializeTraceNode(child, writer);
        i++;
    }
    writer->Write("]");
}

void HeapSnapshotJSONSerializer::SerializeSamples(HeapSnapshot *snapshot, StreamWriter *writer)
{
    writer->Write("\"samples\":[");
    const CVector<TimeStamp> &timeStamps = snapshot->GetTimeStamps();
    if (!timeStamps.empty()) {
        auto firstTimeStamp = timeStamps[0];
        bool isFirst = true;
        for (auto timeStamp : timeStamps) {
            if (!isFirst) {
                writer->Write("\n, ");
            } else {
                isFirst = false;
            }
            writer->Write(timeStamp.GetTimeStamp() - firstTimeStamp.GetTimeStamp());
            writer->Write(", ");
            writer->Write(timeStamp.GetLastSequenceId());
        }
    }
    writer->Write("],\n");
}

void HeapSnapshotJSONSerializer::SerializeLocations(StreamWriter *writer)
{
    writer->Write("\"locations\":[],\n");
}

void HeapSnapshotJSONSerializer::SerializeStringTable(HeapSnapshot *snapshot, StreamWriter *writer)
{
    const StringHashMap *stringTable = snapshot->GetEcmaStringTable();
    ASSERT(stringTable != nullptr);
    writer->Write("\"strings\":[\"<dummy>\",\n");
    writer->Write("\"\",\n");
    writer->Write("\"GC roots\",\n");
    // StringId Range from 3
    size_t capcity = stringTable->GetCapcity();
    size_t i = 0;
    for (auto key : stringTable->GetOrderedKeyStorage()) {
        if (i == capcity - 1) {
            writer->Write("\"");
            SerializeString(stringTable->GetStringByKey(key), writer); // No Comma for the last line
            writer->Write("\"\n");
        } else {
            writer->Write("\"");
            SerializeString(stringTable->GetStringByKey(key), writer);
            writer->Write("\",\n");
        }
        i++;
    }
    writer->Write("]\n");
}

void HeapSnapshotJSONSerializer::SerializeString(CString *str, StreamWriter *writer)
{
    const char *s = str->c_str();
    while (*s != '\0') {
        if (*s > ASCII_US && *s < ASCII_DEL) {
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
    writer->Write("\\u");
    writer->WriteChar(hexChars[(unicodeChar >> 0xC) & 0xF]);
    writer->WriteChar(hexChars[(unicodeChar >> 0x8) & 0xF]);
    writer->WriteChar(hexChars[(unicodeChar >> 0x4) & 0xF]);
    writer->WriteChar(hexChars[unicodeChar & 0xF]);
}

void HeapSnapshotJSONSerializer::SerializerSnapshotClosure(StreamWriter *writer)
{
    writer->Write("}\n");
}
}  // namespace panda::ecmascript
