/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS of ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RAWHEAP_TRANSLATE_STATIC_RAWHEAP_TRANSLATE_H
#define RAWHEAP_TRANSLATE_STATIC_RAWHEAP_TRANSLATE_H

#include "rawheap_translate.h"
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace rawheap_translate {

/** @brief Per-item layout computed during CollectArrayItems Pass 1. */
struct ArrayItemLayout {
    size_t prefixOff = 0;
    uint32_t arrayLength = 0;
    uint8_t elementType = 0;
    size_t dataSize = 0;
    bool dataSizeKnown = false;
};

/**
 * @brief Parser for the static binary snapshot format (ArkTS-Sta / ETS side).
 *
 * The static file is self-describing: it carries its own string pool
 * (STRING_IN_UTF8), class descriptors (LOAD_CLASS + STATIC_CLASS_DUMP) and
 * structured field values (STATIC_INSTANCE_DUMP, STATIC_ARRAY_DUMP), so unlike
 * V1/V2 it does not depend on an external metadata JSON.
 *
 * Counterpart writer: runtime_core/static_core/runtime/tooling/hprof/static_dump.h;
 * shared wire format in rawheap_translate/common.h and
 * runtime_core/static_core/plugins/ets/runtime/tooling/hprof/session/dump_format.h.
 *
 * Parsing is two-phase:
 *   1. Collect  - read every RecordHeader + body, store raw records into
 *                 containers. No graph is built yet.
 *   2. Build     - once all records (including class descriptors, which may
 *                 appear after the instances that reference them) are known,
 *                 walk instances / arrays / roots and create Node + Edge.
 *
 * Parse() does both phases. Translate() only runs in single-file mode: it
 * creates the synthetic root + StaticRoot group + primitive nodes. In two-file
 * (merge) mode the SnapshotMerger consumes the collected graph directly and
 * never calls Translate().
 *
 * recordCount in the file header is a summary metric, NOT the on-disk record
 * count - the main loop is EOF-driven (see the field-level note on Header::recordCount).
 */
class StaticRawheapTranslate : public RawHeap {
public:
    StaticRawheapTranslate() = default;
    ~StaticRawheapTranslate() override;

    bool Parse(FileReader &file, uint32_t rawheapFileSize) override;
    bool Translate() override;

    /**
     * @brief Enable the synthetic-root framework (single-file output mode).
     *
     * When set, Parse() prepends a SyntheticRoot + StaticRoot group to the
     * built graph so the result is a standalone .heapsnapshot. Leave it unset
     * (default) when the parser feeds a SnapshotMerger in two-file mode.
     */
    void EnableRootFramework() { buildRootFramework_ = true; }

    /** Collected GC roots (object nodeIds), for the merger. */
    const std::vector<uint32_t> &GetRoots() const { return roots_; }

    /** Collected XRef records, for the merger. */
    struct XRefRecord {
        uint32_t dynNodeId;   // dynamic-side nodeId (4 bytes on disk)
        uint32_t staNodeId;   // nodeId in static heap
        uint8_t direction;
    };
    const std::vector<XRefRecord> &GetXRefs() const { return xrefs_; }

    /**
     * @brief Look up a Node by its nodeId.
     * Used by the merger to resolve XRef staNodeId values that point into
     * the static side. Returns nullptr if not found.
     */
    Node *FindNodeByNodeId(uint32_t nodeId) const;

private:
    struct Header {
        uint32_t identifierSize = 0;
        uint64_t timestamp = 0;
        uint8_t  language = 0;
        uint32_t headerSize = 0;
        uint32_t recordCount = 0;  // summary metric only
        uint32_t featureFlags = 0;
    };

    struct FieldDef {
        uint32_t nameId = 0;
        uint8_t type = 0;
        uint32_t offset = 0;
        uint16_t flags = 0;
    };

    // A single field value read from STATIC_INSTANCE_DUMP or STATIC_CLASS_DUMP's
    // static-value section. For OBJECT/ARRAY, `value` holds a nodeId (uint32_t
    // cast to uint64_t).
    struct FieldValue {
        uint8_t type = 0;
        uint64_t value = 0;
    };

    struct ClassInfo {
        uint32_t classNameId = 0;
        uint32_t instanceSize = 0;
        uint32_t superClassNodeId = 0;          // superclass classObjectId (0 if none)
        std::vector<FieldDef> staticFields;
        std::vector<FieldValue> staticValues;   // parallel to staticFields (same order)
        std::vector<uint32_t> methodNameIds;    // declared method-name string-pool ids
        std::vector<FieldDef> instanceFields;
    };

    struct InstanceRecord {
        uint32_t objectNodeId = 0;
        uint32_t classNodeId = 0;
        uint32_t instanceSize = 0;
        std::vector<FieldValue> values;
    };

    struct ArrayRecord {
        uint32_t objectNodeId = 0;
        uint32_t classNodeId = 0;
        uint32_t instanceSize = 0;
        uint32_t length = 0;
        uint8_t elementType = 0;
        std::vector<uint32_t> elements;  // populated only for OBJECT/ARRAY (nodeIds)
        // TAGGED arrays carry one runtime-typed FieldValue per element because
        // their payload widths vary (OBJECT=u32, TAGGED=u64, BOOLEAN=u8, ...).
        std::vector<FieldValue> taggedValues;
        // Raw LE element bytes for primitive arrays (length * FieldSize); empty for
        // OBJECT/ARRAY/TAGGED and unknown types. CreateArrayEdges boxes each into a wrapper.
        std::vector<char> primData;
    };

    // A string object dumped via TAG_STATIC_STRING_DUMP. The UTF-8 content
    // becomes the node's name so string values are visible in the .heapsnapshot
    // (a plain INSTANCE_DUMP has no per-instance name field).
    struct StringInstanceRecord {
        uint32_t objectNodeId = 0;
        uint32_t classNodeId = 0;
        uint32_t instanceSize = 0;
        std::string content;
    };

    Header header_;
    bool buildRootFramework_ {false};
    bool parseOk_ {true};         // set to false on any read failure
    bool readingRecord_ {false};  // enables record-body bounds in ReadBytes
    uint32_t recordRemaining_ {0};
    FileReader *file_ {nullptr};  // set during Parse(), used by primitive readers

    std::unordered_map<uint32_t, std::string> stringTable_;
    std::unordered_map<uint32_t, ClassInfo> classMap_;
    std::vector<InstanceRecord> instances_;
    std::vector<ArrayRecord> arrays_;
    std::vector<StringInstanceRecord> stringInstances_;
    std::vector<uint32_t> roots_;
    std::vector<XRefRecord> xrefs_;
    std::unordered_map<uint32_t, Node *> nodeIdToNode_;
    uint32_t valueNodeCounter_ {0};  // synthetic nodeId generator for value nodes

    // Phase 1: Collect (record reading).
    bool ParseHeader();
    bool ParseRecord(uint64_t &offset, uint64_t fileSize);
    bool DispatchRecord(uint8_t tag, uint32_t length, uint32_t count);
    bool SkipBody(uint32_t length);

    // Record collectors (batched: each processes count items from the body).
    bool CollectStringItems(uint32_t length, uint32_t count);
    bool CollectLoadClassItems(uint32_t length, uint32_t count);
    bool CollectStaticClassDumpItems(uint32_t length, uint32_t count);
    // CollectStaticClassDumpItems helpers (one section each, single responsibility).
    void ReadFieldDescriptor(FieldDef &fd);
    void ReadFieldDescriptors(std::vector<FieldDef> &out);
    void ReadStaticValues(std::vector<FieldValue> &out);
    void ReadMethodNames(std::vector<uint32_t> &out);
    bool CollectRootItems(uint32_t length, uint32_t count);
    bool CollectInstanceItems(uint32_t length, uint32_t count);
    bool CollectArrayItems(uint32_t length, uint32_t count);
    bool CollectStaticStringDumpItems(uint32_t length, uint32_t count);
    bool ScanArrayPrefixes(char *body, uint32_t length, std::vector<struct ArrayItemLayout> &layouts,
                            size_t &totalKnownData, size_t &totalUnknownLength);
    bool ScanArrayDataSize(const char *body, uint32_t length, size_t dataOffset,
                           ArrayItemLayout &layout, size_t &totalUnknownLength);
    bool DistributeUnknownData(std::vector<struct ArrayItemLayout> &layouts,
                                uint32_t count, size_t totalKnownData,
                                uint32_t length, size_t totalUnknownLength);
    bool BuildArrayRecords(char *body, uint32_t count,
                           uint32_t bodyLength, const std::vector<struct ArrayItemLayout> &layouts);
    bool BuildArrayRecord(char *body, uint32_t bodyLength, uint32_t itemIndex,
                          const ArrayItemLayout &layout, ArrayRecord &record);
    bool ReadTaggedArrayValues(const char *body, size_t dataOffset, size_t dataSize, ArrayRecord &record);
    bool CollectXRefItems(uint32_t length, uint32_t count);
    bool CollectHeapSummary(uint32_t length);

    // Phase 2: Build (graph construction from collected records).
    void BuildGraph(bool withRootFramework);
    // classMap_ keys in ascending nodeId order (deterministic output).
    std::vector<uint32_t> SortedClassNodeIds() const;
    void CreateClassNodes();
    void CreateInstanceNodes();
    void CreateArrayNodes();
    void CreateStringNodes();  // STRING-typed nodes from TAG_STATIC_STRING_DUMP
    void CreateRootEdges(Node *syntheticRoot, Node *staticRoot);
    // Emits a class node's superclass edge (INTERNAL "superClass") AND its static
    // field edges (PROPERTY) in one pass, so a class's edges stay contiguous in
    // the flat edge vector - required by the .heapsnapshot grouping contract
    // (node i's edges are the next edgeCount[i] edges). Splitting them across
    // phases would interleave other nodes' edges between them.
    void CreateClassEdges();
    // CreateClassEdges helpers.
    void EmitSuperClassEdge(Node *classNode, const ClassInfo &info);
    void EmitStaticFieldEdges(Node *classNode, const ClassInfo &info);
    bool EmitFieldValueEdge(Node *from, const FieldValue &value, StringId nameId);
    // Emit one PROPERTY edge per declared method name (class -> synthetic
    // "closure" node, edge name = method name). The static dumper records
    // method-name string ids in CLASS_DUMP's methodNameId[]; without this path
    // they would be read into ClassInfo::methodNameIds but never reach the
    // .heapsnapshot (no edge references them, so the strings are never promoted
    // into the strings table).
    void EmitMethodNameEdges(Node *classNode, const ClassInfo &info);
    void CreateInstanceEdges();
    void EmitInstanceFieldEdges(Node *node, const InstanceRecord &rec,
                                 const ClassInfo &info);
    void EmitFallbackFieldEdges(Node *node, const InstanceRecord &rec);
    void CreateArrayEdges();
    // CreateArrayEdges helpers.
    Node *CreateArrayBufferNode(Node *arrayNode, StringId bufferNameId);
    void EmitArrayRefElementEdges(Node *buffer, const ArrayRecord &rec);
    void EmitTaggedArrayElementEdges(Node *buffer, const ArrayRecord &rec);
    void EmitArrayBoxedPrimitiveEdges(Node *buffer, const ArrayRecord &rec,
                                      StringId valueNameId);
    // Emit the string node's hclass edge (string -> std.core.String class node),
    // mirroring the instance->class edge so string nodes are graph-reachable
    // and the String class node is connected to its instances.
    void CreateStringEdges();

    // Emit (or reuse) a synthetic value node for a primitive field
    // value. Returns the target node for an edge. nodeId lives in a high
    // synthetic range (0x40000000+) to avoid colliding with real heap addrs.
    Node *GetOrCreateValueNode(uint8_t fieldType, uint64_t value);
    // Emit a synthetic "closure" node representing a declared method, named
    // after the method. One node per method-name occurrence (each function is
    // its own node). Shares the value-node synthetic-id range.
    Node *GetOrCreateMethodNode(StringId nameId);
    // Stringify a primitive field value for its synthetic value node's name.
    std::string MakeValueNodeName(uint8_t fieldType, uint64_t value) const;
    std::string MakeTaggedValueNodeName(uint64_t value) const;

    Node *GetOrCreateNode(uint32_t nodeId);
    std::string GetString(uint32_t stringId) const;
    StringId GetOrCreateStringId(uint32_t stringId);

    // Bounded reader for a record body. Header reads use the same helper with
    // readingRecord_ disabled.
    bool ReadBytes(char *buffer, uint32_t size);

    // Primitive readers using ReadBytes (little-endian). On read failure, set
    // parseOk_ = false and return 0.
    uint8_t ReadU8();
    uint16_t ReadU16();
    uint32_t ReadU32();
    uint64_t ReadU64();
    // Read a field value of known byte size from file_ (little-endian pack).
    uint64_t ReadFieldValue(uint8_t byteSize);
    static uint8_t FieldSize(uint8_t fieldType);
};

}  // namespace rawheap_translate
#endif  // RAWHEAP_TRANSLATE_STATIC_RAWHEAP_TRANSLATE_H
