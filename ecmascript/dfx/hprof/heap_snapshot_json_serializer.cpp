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

HeapSnapshotJSONSerializer::~HeapSnapshotJSONSerializer()
{
    if (writer_) {
        delete writer_;
        writer_ = nullptr;
    }
}

bool HeapSnapshotJSONSerializer::Serialize(HeapSnapshot *snapshot, Stream *stream)
{
    // Serialize Node/Edge/String-Table
    LOG_ECMA(INFO) << "HeapSnapshotJSONSerializer::Serialize begin";
    snapshot_ = snapshot;
    ASSERT(snapshot_->GetNodes() != nullptr && snapshot_->GetEdges() != nullptr &&
           snapshot_->GetEcmaStringTable() != nullptr);
    writer_ = new StreamWriter(stream);

    SerializeSnapshotHeader();     // 1.
    SerializeNodes();              // 2.
    SerializeEdges();              // 3.
    SerializeTraceFunctionInfo();  // 4.
    SerializeTraceTree();          // 5.
    SerializeSamples();            // 6.
    SerializeLocations();          // 7.
    SerializeStringTable();        // 8.
    SerializerSnapshotClosure();   // 9.
    writer_->End();

    LOG_ECMA(INFO) << "HeapSnapshotJSONSerializer::Serialize exit";
    return true;
}

void HeapSnapshotJSONSerializer::SerializeSnapshotHeader()
{
    writer_->Write("{\"snapshot\":\n");  // 1.
    writer_->Write("{\"meta\":\n");      // 2.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer_->Write("{\"node_fields\":[\"type\",\"name\",\"id\",\"self_size\",\"edge_count\",\"trace_node_id\",");
    writer_->Write("\"detachedness\"],\n");  // 3.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer_->Write("\"node_types\":[[\"hidden\",\"array\",\"string\",\"object\",\"code\",\"closure\",\"regexp\",");
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer_->Write("\"number\",\"native\",\"synthetic\",\"concatenated string\",\"slicedstring\",\"symbol\",");
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer_->Write("\"bigint\"],\"string\",\"number\",\"number\",\"number\",\"number\",\"number\"],\n");  // 4.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer_->Write("\"edge_fields\":[\"type\",\"name_or_index\",\"to_node\"],\n");  // 5.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer_->Write("\"edge_types\":[[\"context\",\"element\",\"property\",\"internal\",\"hidden\",\"shortcut\",");
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer_->Write("\"weak\"],\"string_or_number\",\"node\"],\n");  // 6.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer_->Write("\"trace_function_info_fields\":[\"function_id\",\"name\",\"script_name\",\"script_id\",");
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer_->Write("\"line\",\"column\"],\n");  // 7.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer_->Write("\"trace_node_fields\":[\"id\",\"function_info_index\",\"count\",\"size\",\"children\"],\n");
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    writer_->Write("\"sample_fields\":[\"timestamp_us\",\"last_assigned_id\"],\n");  // 9.
    // NOLINTNEXTLINE(modernize-raw-string-literal)
    // 10.
    writer_->Write("\"location_fields\":[\"object_index\",\"script_id\",\"line\",\"column\"]},\n\"node_count\":");
    writer_->Write(snapshot_->GetNodeCount());                         // 11.
    writer_->Write(",\n\"edge_count\":");
    writer_->Write(snapshot_->GetEdgeCount());                         // 12.
    writer_->Write(",\n\"trace_function_count\":");
    writer_->Write(snapshot_->GetTrackAllocationsStack().size());   // 13.
    writer_->Write("\n},\n");  // 14.
}

void HeapSnapshotJSONSerializer::SerializeNodes()
{
    const CList<Node *> *nodes = snapshot_->GetNodes();
    const StringHashMap *stringTable = snapshot_->GetEcmaStringTable();
    ASSERT(nodes != nullptr);
    writer_->Write("\"nodes\":[");  // Section Header
    size_t i = 0;
    for (auto *node : *nodes) {
        if (i > 0) {
            writer_->Write(",");  // add comma except first line
        }
        writer_->Write(static_cast<int>(NodeTypeConverter::Convert(node->GetType())));  // 1.
        writer_->Write(",");
        writer_->Write(stringTable->GetStringId(node->GetName()));                      // 2.
        writer_->Write(",");
        writer_->Write(node->GetId());                                                  // 3.
        writer_->Write(",");
        writer_->Write(node->GetSelfSize());                                            // 4.
        writer_->Write(",");
        writer_->Write(node->GetEdgeCount());                                           // 5.
        writer_->Write(",");
        writer_->Write(node->GetStackTraceId());                                        // 6.
        writer_->Write(",");
        if (i == nodes->size() - 1) {  // add comma at last the line
            writer_->Write("0],\n"); // 7. detachedness default
        } else {
            writer_->Write("0\n");  // 7.
        }
        i++;
    }
}

void HeapSnapshotJSONSerializer::SerializeEdges()
{
    const CList<Edge *> *edges = snapshot_->GetEdges();
    const StringHashMap *stringTable = snapshot_->GetEcmaStringTable();
    ASSERT(edges != nullptr);
    writer_->Write("\"edges\":[");
    size_t i = 0;
    for (auto *edge : *edges) {
        StringId nameOrIndex = edge->GetType() == EdgeType::ELEMENT ?
            edge->GetIndex() : stringTable->GetStringId(edge->GetName());
        if (i > 0) {  // add comma except the first line
            writer_->Write(",");
        }
        writer_->Write(static_cast<int>(edge->GetType()));          // 1.
        writer_->Write(",");
        writer_->Write(nameOrIndex);  // 2. Use StringId
        writer_->Write(",");

        if (i == edges->size() - 1) {  // add comma at last the line
            writer_->Write(edge->GetTo()->GetIndex() * Node::NODE_FIELD_COUNT);  // 3.
            writer_->Write("],\n");
        } else {
            writer_->Write(edge->GetTo()->GetIndex() * Node::NODE_FIELD_COUNT);    // 3.
            writer_->Write("\n");
        }
        i++;
    }
}

void HeapSnapshotJSONSerializer::SerializeTraceFunctionInfo()
{
    const CVector<FunctionInfo> trackAllocationsStack = snapshot_->GetTrackAllocationsStack();
    const StringHashMap *stringTable = snapshot_->GetEcmaStringTable();

    writer_->Write("\"trace_function_infos\":[");  // Empty
    size_t i = 0;

    for (const auto &info : trackAllocationsStack) {
        if (i > 0) {  // add comma except the first line
            writer_->Write(",");
        }
        writer_->Write(info.functionId);
        writer_->Write(",");
        CString functionName(info.functionName.c_str());
        writer_->Write(stringTable->GetStringId(&functionName));
        writer_->Write(",");
        CString scriptName(info.scriptName.c_str());
        writer_->Write(stringTable->GetStringId(&scriptName));
        writer_->Write(",");
        writer_->Write(info.scriptId);
        writer_->Write(",");
        writer_->Write(info.lineNumber);
        writer_->Write(",");
        writer_->Write(info.columnNumber);
        writer_->Write("\n");
        i++;
    }
    writer_->Write("],\n");
}

void HeapSnapshotJSONSerializer::SerializeTraceTree()
{
    writer_->Write("\"trace_tree\":[");
    TraceTree* tree = snapshot_->GetTraceTree();
    if ((tree != nullptr) && (snapshot_->trackAllocations())) {
        SerializeTraceNode(tree->GetRoot());
    }
    writer_->Write("],\n");
}

void HeapSnapshotJSONSerializer::SerializeTraceNode(TraceNode* node)
{
    if (node == nullptr) {
        return;
    }

    writer_->Write(node->GetId());
    writer_->Write(",");
    writer_->Write(node->GetNodeIndex());
    writer_->Write(",");
    writer_->Write(node->GetTotalCount());
    writer_->Write(",");
    writer_->Write(node->GetTotalSize());
    writer_->Write(",[");

    int i = 0;
    for (TraceNode* child : node->GetChildren()) {
        if (i > 0) {
            writer_->Write(",");
        }
        SerializeTraceNode(child);
        i++;
    }
    writer_->Write("]");
}

void HeapSnapshotJSONSerializer::SerializeSamples()
{
    writer_->Write("\"samples\":[");
    const CVector<TimeStamp> &timeStamps = snapshot_->GetTimeStamps();
    if (!timeStamps.empty()) {
        auto firstTimeStamp = timeStamps[0];
        bool isFirst = true;
        for (auto timeStamp : timeStamps) {
            if (!isFirst) {
                writer_->Write("\n, ");
            } else {
                isFirst = false;
            }
            writer_->Write(timeStamp.GetTimeStamp() - firstTimeStamp.GetTimeStamp());
            writer_->Write(", ");
            writer_->Write(timeStamp.GetLastSequenceId());
        }
    }
    writer_->Write("],\n");
}

void HeapSnapshotJSONSerializer::SerializeLocations()
{
    writer_->Write("\"locations\":[],\n");
}

void HeapSnapshotJSONSerializer::SerializeStringTable()
{
    const StringHashMap *stringTable = snapshot_->GetEcmaStringTable();
    ASSERT(stringTable != nullptr);
    writer_->Write("\"strings\":[\"<dummy>\",\n");
    writer_->Write("\"\",\n");
    writer_->Write("\"GC roots\",\n");
    // StringId Range from 3
    size_t capcity = stringTable->GetCapcity();
    size_t i = 0;
    for (auto key : stringTable->GetOrderedKeyStorage()) {
        if (i == capcity - 1) {
            writer_->Write("\"");
            writer_->Write(*(stringTable->GetStringByKey(key)));  // No Comma for the last line
            writer_->Write("\"\n");
        } else {
            writer_->Write("\"");
            writer_->Write(*(stringTable->GetStringByKey(key)));
            writer_->Write("\",\n");
        }
        i++;
    }
    writer_->Write("]\n");
}

void HeapSnapshotJSONSerializer::SerializerSnapshotClosure()
{
    writer_->Write("}\n");
}
}  // namespace panda::ecmascript
