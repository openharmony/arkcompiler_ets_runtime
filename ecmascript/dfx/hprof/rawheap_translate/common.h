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

#ifndef RAWHEAP_TRANSLATE_COMMON_H
#define RAWHEAP_TRANSLATE_COMMON_H

#include <array>
#include <cstdint>
#include <vector>
#include <string>

namespace rawheap_translate {
using JSType = uint8_t;
using NodeType = uint8_t;
using StringKey = size_t;
using StringId = uint32_t;

static constexpr NodeType DEFAULT_NODETYPE = 8;  // 8: means default node type
static constexpr NodeType SYNTHETIC_NODETYPE = 9;  // 9: means SYNTHETIC node type
static constexpr NodeType FRAMEWORK_NODETYPE = 14;
static constexpr NodeType ROOT = 15;
static constexpr NodeType HEAP_NUMBER = 7;
static constexpr NodeType STRING = 2;
// Per-record-tag node types restored by the static rawheap parser. These mirror
// the binary serializer's node_types list (serializer.cpp): array=1, object=3.
// CLASS is appended to that list at index 16 (additive — does not shift the
// existing framework/handle slots). The dynamic parser derives node types from
// metadata instead, but the static side has no JSType, so the record tag that
// populated the record (TAG_STATIC_CLASS_DUMP / INSTANCE_DUMP / ARRAY_DUMP) is
// the only signal available.
static constexpr NodeType ARRAY_NODETYPE = 1;   // 1: "array" in node_types
static constexpr NodeType OBJECT_NODETYPE = 3;  // 3: "object" in node_types
static constexpr NodeType CLOSURE_NODETYPE = 5;  // 5: "closure" in node_types (synthetic method node)
static constexpr NodeType CLASS_NODETYPE = 16;  // 16: "class" appended to node_types
enum class EdgeType { CONTEXT, ELEMENT, PROPERTY, INTERNAL, HIDDEN, SHORTCUT, WEAK, XREF, DEFAULT = PROPERTY };

static constexpr int VIRTUAL_NODE_SIZE = 1; // The virtual node size is fixed at 1

static constexpr int HANDLE_COUNT_ADDR_SIZE = 4; // The root handle count uses 32-bit addresses
static constexpr int ADDRESS_SIZE_V1 = 8;  // V1 uses 64-bit addresses
static constexpr int ADDRESS_SIZE_V2 = 4;  // V2 uses 32-bit addresses

struct Field {
    std::string name = "";
    uint32_t offset = 0;
    uint32_t size = 0;
};

struct MetaData {
    std::string name = "";
    std::string nodeName = "";
    std::string visitType = "";
    std::vector<Field> fields {};
    MetaData *parent {nullptr};
    uint32_t endOffset = 0;
    JSType type = 0;
    NodeType nodeType = DEFAULT_NODETYPE;

    bool IsArray()
    {
        return visitType == "Array";
    }
};

struct BitField {
    Field objectTypeField;
    Field objectBitField1;
    Field hclassLayoutField;
    Field jsObjectPropertiesField;
    Field nativePointerBindingSizeField;
    Field taggedArrayLengthField;
    Field taggedArrayDataField;
    Field dictionaryLengthField;
    Field dictionaryDataField;
};

struct DictionaryLayout {
    uint32_t keyIndex = 0;
    uint32_t valueIndex = 0;
    uint32_t detailIndex = 0;
    uint32_t entrySize = 0;
    uint32_t headerSize = 0;
};

struct TypeRange {
    JSType stringFirst = 0;
    JSType stringLast = 0;
    JSType objectFirst = 0;
    JSType objectLast = 0;
};

struct Node {
    uint64_t nodeId = 0;
    StringId strId = 1;   // 1: for empty string
    uint32_t edgeCount = 0;
    uint32_t index = 0;
    uint32_t size = 0;
    uint32_t nativeSize = 0;
    char *data {nullptr};
    Node *hclass {nullptr};
    NodeType type = DEFAULT_NODETYPE;
    JSType jsType = 0;

    Node(uint32_t nodeIndex) : index(nodeIndex) {}
};

struct Edge {
    Node *to {nullptr};
    uint32_t nameOrIndex = 0;
    EdgeType type = EdgeType::DEFAULT;
    // Source node (set by the static parser so edges can be sorted into the
    // the .heapsnapshot grouping contract: node i owns the next edgeCount[i] edges). V1/V2
    // leave this null (their edges are already inserted in source order).
    Node *from {nullptr};

    Edge(Node *node, uint32_t index, EdgeType edgeType) : to(node), nameOrIndex(index), type(edgeType) {}
    Edge(Node *fromNode, Node *toNode, uint32_t index, EdgeType edgeType)
        : to(toNode), nameOrIndex(index), type(edgeType), from(fromNode) {}
};

static constexpr uint8_t ZERO_VALUE = 0x02U;       // 0000 0010
static constexpr uint8_t HOLE_VALUE = 0x12U;       // 0001 0010
static constexpr uint8_t NULL_VALUE = 0x22U;       // 0010 0010
static constexpr uint8_t EXCP_VALUE = 0x32U;       // 0011 0010
static constexpr uint8_t UNDF_VALUE = 0x42U;       // 0100 0010
static constexpr uint8_t TRUE_VALUE = 0x52U;       // 0101 0010
static constexpr uint8_t FALS_VALUE = 0x62U;       // 0110 0010
static constexpr uint8_t INTL_VALUE = 0x04U;       // 0000 0100
static constexpr uint8_t DOUB_VALUE = 0x06U;       // 0000 0110

// Sentinel the dumper writes as the globalRef group count when track-global-ref
// is disabled. Translator treats it as "tracking off, no payload follows".
// Shared between rawheap_dump.cpp (writer) and rawheap_translate.cpp (reader).
constexpr uint32_t GLOBAL_REF_TRACK_OFF_MARK = 0xFFFFFFFFU;
// ---- Static binary snapshot format constants ----
//
// IMPORTANT: These constants mirror the authoritative definitions in
// plugins/ets/runtime/tooling/hprof/session/dump_format.h. The rawheap_translate
// module is an offline CLI tool that cannot depend on the ETS runtime implementation,
// so it must maintain its own copies.  When updating any value here,
// you MUST also update the corresponding value in dump_format.h (and
// vice versa) to keep the two in sync.  A mismatch will produce a
// format that the parser/writer on the other side cannot correctly
// interpret.
//
// Naming conventions:
//   TAG_*        → same name and value as ark::tooling::hprof::TAG_*
//   StaFieldType → mirrors ark::tooling::hprof::FieldType (same numeric values,
//                  different name because this module does not use the
//                  ark::tooling::hprof namespace)
//   XREF_*       → same name and value as ark::tooling::hprof::XREF_DIR_*
//   STATIC_*     → mirrors ark::tooling::hprof::HYBRID_DUMP_* / *_BODY_SIZE

// Record tags
static constexpr uint8_t TAG_STRING_IN_UTF8    = 0x01;
static constexpr uint8_t TAG_LOAD_CLASS        = 0x02;
static constexpr uint8_t TAG_STATIC_CLASS_DUMP = 0x0B;
static constexpr uint8_t TAG_ROOT_RECORD       = 0x10;
static constexpr uint8_t TAG_STATIC_INSTANCE_DUMP = 0x14;
static constexpr uint8_t TAG_STATIC_ARRAY_DUMP  = 0x15;
static constexpr uint8_t TAG_STATIC_STRING_DUMP = 0x16;  // string objects with UTF-8 content
static constexpr uint8_t TAG_XREF_EDGE         = 0x30;
static constexpr uint8_t TAG_HEAP_SUMMARY      = 0xFE;
static constexpr uint8_t TAG_PARTIAL_MARKER    = 0xFF;

// Root types (within TAG_ROOT_RECORD body)
static constexpr uint8_t ROOT_TYPE_STATIC_OBJECT = 0x00;

// Field types (mirrors ark::tooling::hprof::FieldType numeric values)
enum class StaFieldType : uint8_t {
    UNKNOWN = 0x00, BOOLEAN = 0x01, CHAR = 0x02, FLOAT = 0x03,
    DOUBLE  = 0x04, BYTE   = 0x05, SHORT = 0x06, INT   = 0x07,
    LONG    = 0x08, OBJECT = 0x09, ARRAY = 0x0A, TAGGED = 0x0B,
    WEAK_OBJECT = 0x0C,
};

// Static runtime coretypes::TaggedValue special payloads. Tagged heap objects
// and primitives are normalized by the writer to OBJECT/WEAK_OBJECT or their
// concrete primitive type; TAGGED normally carries one of these raw markers.
// Unknown raw markers are retained as stable hexadecimal synthetic values.
inline constexpr uint64_t STATIC_TAGGED_HOLE = 0x00ULL;
inline constexpr uint64_t STATIC_TAGGED_NULL = 0x02ULL;
inline constexpr uint64_t STATIC_TAGGED_FALSE = 0x06ULL;
inline constexpr uint64_t STATIC_TAGGED_TRUE = 0x07ULL;
inline constexpr uint64_t STATIC_TAGGED_UNDEFINED = 0x0AULL;
inline constexpr uint64_t STATIC_TAGGED_EXCEPTION = 0x12ULL;

// XRef direction
static constexpr uint8_t XREF_DYN_TO_STA = 0;
static constexpr uint8_t XREF_STA_TO_DYN = 1;
static constexpr uint8_t XREF_BIDIR      = 2;

// Header / record layout sizes
//
// The header starts with an 8-byte version string matching the V1/V2 convention.
// Old V1/V2 tools that encounter a V3 file read "3.0.0" as the version,
// try ParseRawheap, see VERSION(2,0,0) < Version(3,0,0) → gracefully exit.
// New tools route every compatible 3.x.x version to the static parser.
static constexpr size_t   STATIC_VERSION_SIZE      = 8;    // "3.0.0\0\0\0"
static constexpr uint32_t STATIC_HEADER_SIZE       = 33;   // version(8)+id(4)+ts(8)+lang(1)+hdr(4)+rec(4)+flags(4)
static constexpr uint32_t STATIC_IDENTIFIER_SIZE   = 4;    // 4-byte object identifier (u32 nodeId, even numbers)
inline constexpr uint8_t STATIC_LANGUAGE_STATIC = 1;
inline constexpr uint8_t STATIC_LANGUAGE_HYBRID = 2;
inline constexpr uint32_t STATIC_SUPPORTED_FEATURE_FLAGS = 0;
static constexpr int STATIC_SNAPSHOT_MAJOR_VERSION = 3;
static constexpr size_t   STATIC_RECORD_HDR_SIZE   = 17;  // tag(1)+time(8)+length(4)+count(4)

// Offsets within the record header (STATIC_RECORD_HDR_SIZE bytes).
static constexpr size_t STATIC_RECORD_HDR_TAG_OFF    = 0;   // tag: u8
static constexpr size_t STATIC_RECORD_HDR_TIME_OFF   = 1;   // timestamp: u64 (8 bytes)
static constexpr size_t STATIC_RECORD_HDR_LENGTH_OFF = 9;   // body length: u32 (4 bytes)
static constexpr size_t STATIC_RECORD_HDR_COUNT_OFF  = 13;  // item count: u32 (4 bytes)
static constexpr size_t   STATIC_ROOT_BODY_SIZE    = 5;   // rootType(1)+objNodeId(4)
static constexpr size_t   STATIC_XREF_BODY_SIZE    = 9;   // dynNodeId(4)+staNodeId(4)+dir(1)
// objNodeId(4)+classNodeId(4)+stackTrace(4)+instSize(4)+arrayLen(4)+elemType(1)
static constexpr size_t STATIC_ARRAY_PREFIX_BODY_SIZE = 21;

// STATIC_STRING_DUMP fixed prefix: objId(4)+classObjId(4)+instSize(4)+valueLen(4) = 16,
// followed by valueLen bytes of UTF-8 content.
static constexpr size_t STATIC_STRING_PREFIX_BODY_SIZE = 16;
static constexpr size_t STATIC_STRING_OBJADDR_OFF = 0;
static constexpr size_t STATIC_STRING_CLASSOBJ_OFF = STATIC_IDENTIFIER_SIZE;            // 4
static constexpr size_t STATIC_STRING_INSTSIZE_OFF = 2 * STATIC_IDENTIFIER_SIZE;       // 8
static constexpr size_t STATIC_STRING_VALUELEN_OFF = 3 * STATIC_IDENTIFIER_SIZE;       // 12

// Field offsets within the array prefix body (relative to the start of the prefix).
static constexpr size_t STATIC_ARRAY_CLASS_OFFSET = STATIC_IDENTIFIER_SIZE;  // 4: classNodeId field offset
static constexpr size_t STATIC_ARRAY_INSTSIZE_OFFSET =
    2 * STATIC_IDENTIFIER_SIZE + sizeof(uint32_t);  // 12: instanceSize field offset
static constexpr size_t STATIC_ARRAY_LENGTH_OFFSET =
    2 * STATIC_IDENTIFIER_SIZE + 2 * sizeof(uint32_t);  // 16: arrayLen field offset
static constexpr size_t STATIC_ARRAY_ELEM_TYPE_OFFSET =
    STATIC_ARRAY_LENGTH_OFFSET + sizeof(uint32_t);  // 20: elemType field offset

// Bit-shift helpers for byte-to-integer conversion.
static constexpr size_t BITS_PER_BYTE = 8;

// Version emitted by the current static/hybrid writer. It uses the same 8-byte
// convention as V1/V2. The writer emits 3.0.0, while the reader accepts every
// compatible 3.x.x version.
inline constexpr std::array<char, STATIC_VERSION_SIZE> STATIC_SNAPSHOT_VERSION = {
    '3', '.', '0', '.', '0', '\0', '\0', '\0'
};

}  // namespace rawheap_translate
#endif  // RAWHEAP_TRANSLATE_COMMON_H
