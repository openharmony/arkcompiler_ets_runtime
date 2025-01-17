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

#include "ecmascript/dfx/hprof/rawheap_translate/serializer.h"

namespace rawheap_translate {
bool StreamWriter::Initialize(const std::string &filePath)
{
    if (filePath.empty() || filePath.size() > PATH_MAX) {
        LOG_ERROR("filename is illegal!");
        return false;
    }

    fileStream_.open(filePath, std::ios::out);
    if (fileStream_.fail()) {
        LOG_ERROR("FileStream: open file failed");
        return false;
    }

    return true;
}

void StreamWriter::WriteString(const std::string &str)
{
    MaybeWriteChunk();
    auto len = str.size();
    if (len == 0) {
        return;
    }
    const char *cur = str.c_str();
    const char *end = cur + len;
    while (cur < end) {
        int dstSize = chunkSize_ - current_;
        int writeSize = std::min(static_cast<int>(end - cur), dstSize);
        if (writeSize <= 0) {
            break;
        }
        if (memcpy_s(chunk_.data() + current_, dstSize, cur, writeSize) != EOK) {
            LOG_ERROR("StreamWriter::WriteString: memcpy_s failed!");
            break;
        }
        cur += writeSize;
        current_ += writeSize;
        MaybeWriteChunk();
    }
}

void StreamWriter::EndOfStream()
{
    if (current_ > 0) {
        std::string str(chunk_.data(), current_);
        fileStream_ << str;
        current_ = 0;
    }
    if (fileStream_.is_open()) {
        fileStream_.close();
    }
}

bool HeapSnapshotJSONSerializer::Serialize(RawHeapTranslate *snapshot, StreamWriter *writer)
{
    // Serialize Node/Edge/String-Table
    LOG_INFO("HeapSnapshotJSONSerializer::Serialize begin");

    SerializeSnapshotHeader(snapshot, writer);     // 1.
    SerializeNodes(snapshot, writer);              // 2.
    SerializeEdges(snapshot, writer);              // 3.

    writer->WriteString("\"trace_function_infos\":[],");  // 4.
    writer->WriteString("\"trace_tree\":[],");
    writer->WriteString("\"samples\":[],");
    writer->WriteString("\"locations\":[],\n");

    SerializeStringTable(snapshot, writer);        // 8.
    SerializerSnapshotClosure(writer);   // 9.

    LOG_INFO("HeapSnapshotJSONSerializer::Serialize exit");
    return true;
}

void HeapSnapshotJSONSerializer::SerializeSnapshotHeader(RawHeapTranslate *snapshot, StreamWriter *writer)
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
    writer->WriteString("\"bigint\"],\"string\",\"number\",\"number\",\"number\",\"number\",\"number\"],\n");  // 4.
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
    writer->WriteNumber(0);   // 13.
    writer->WriteString("\n},\n");  // 14.
}

void HeapSnapshotJSONSerializer::SerializeNodes(RawHeapTranslate *snapshot, StreamWriter *writer)
{
    auto nodes = snapshot->GetNodes();
    writer->WriteString("\"nodes\":[");  // Section Header
    size_t i = 0;
    for (auto node : nodes) {
        if (i > 0) {
            writer->WriteChar(',');  // add comma except first line
        }
        writer->WriteNumber(node->type);  // 1.
        writer->WriteChar(',');
        writer->WriteNumber(node->strId);                      // 2.
        writer->WriteChar(',');
        writer->WriteNumber(node->nodeId);                                                  // 3.
        writer->WriteChar(',');
        writer->WriteNumber(node->size);                                            // 4.
        writer->WriteChar(',');
        writer->WriteNumber(node->edgeCount);                                           // 5.
        writer->WriteChar(',');
        writer->WriteNumber(0);                                        // 6.
        writer->WriteChar(',');
        writer->WriteChar('0');                                                              // 7.detachedness default 0
        writer->WriteChar(',');
        writer->WriteNumber(node->nativeSize);
        if (i == nodes.size() - 1) {    // add comma at last the line
            writer->WriteString("],\n"); // 7. detachedness default
        } else {
            writer->WriteString("\n");   // 7.
        }
        i++;
    }
}

void HeapSnapshotJSONSerializer::SerializeEdges(RawHeapTranslate *snapshot, StreamWriter *writer)
{
    auto edges = snapshot->GetEdges();
    writer->WriteString("\"edges\":[");
    size_t i = 0;
    for (auto edge : edges) {
        if (i > 0) {  // add comma except the first line
            writer->WriteChar(',');
        }
        writer->WriteNumber(static_cast<int>(edge->type));          // 1.
        writer->WriteChar(',');
        writer->WriteNumber(static_cast<int>(edge->nameOrIndex));  // 2. Use StringId
        writer->WriteChar(',');

        if (i == edges.size() - 1) {  // add comma at last the line
            writer->WriteNumber(edge->to->index * NODE_FIELD_COUNT);  // 3.
            writer->WriteString("],\n");
        } else {
            writer->WriteNumber(edge->to->index * NODE_FIELD_COUNT);    // 3.
            writer->WriteChar('\n');
        }
        i++;
    }
}

void HeapSnapshotJSONSerializer::SerializeStringTable(RawHeapTranslate *snapshot, StreamWriter *writer)
{
    auto stringTable = snapshot->GetEcmaStringTable();
    writer->WriteString("\"strings\":[\"<dummy>\",\n");
    writer->WriteString("\"\",\n");
    writer->WriteString("\"GC roots\",\n");
    // StringId Range from 3
    size_t capcity = stringTable->GetCapcity();
    if (capcity <= 0) {
        return;
    }
    size_t i = 0;
    for (auto key : stringTable->GetOrderedKeyStorage()) {
        if (i == capcity - 1) {
            writer->WriteChar('\"');
            SerializeString(stringTable->GetStringByKey(key).c_str(), writer); // No Comma for the last line
            writer->WriteString("\"\n");
        } else {
            writer->WriteChar('\"');
            SerializeString(stringTable->GetStringByKey(key).c_str(), writer);
            writer->WriteString("\",\n");
        }
        i++;
    }
    writer->WriteString("]\n");
}

void HeapSnapshotJSONSerializer::SerializeString(const char *str, StreamWriter *writer)
{
    if (str == nullptr || writer == nullptr) {
        return;
    }
    const char *s = str;
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
        } else {
            writer->WriteChar(*s);
            s++;
        }
    }
}

void HeapSnapshotJSONSerializer::SerializerSnapshotClosure(StreamWriter *writer)
{
    writer->WriteString("}\n");
}

}  // namespace rawheap_translate
