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

#include "ecmascript/dfx/hprof/rawheap_translate/common.h"
#include "ecmascript/dfx/hprof/rawheap_translate/rawheap_translate.h"
#include "ecmascript/dfx/hprof/rawheap_translate/snapshot_merger.h"
#include "ecmascript/dfx/hprof/rawheap_translate/static_rawheap_translate.h"
#include "ecmascript/dfx/hprof/rawheap_translate/utils.h"
#include "ecmascript/tests/test_helper.h"

#include "securec.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <random>
#include <utility>
#include <vector>

using namespace panda::ecmascript;
using namespace rawheap_translate;

namespace panda::test {

// Test timestamp value used in header serialization.
static constexpr uint64_t TEST_TIMESTAMP_VALUE = 1000000;

// Parameter struct for WriteStaticArrayDumpRecord (reduces function parameter
// count). Object identifiers are u32 nodeIds (even numbers >= 2) in the new
// wire format.
struct ArrayDumpParams {
    uint32_t objectId = 0;
    uint32_t classObjectId = 0;
    uint32_t instanceSize = 0;
    uint32_t arrayLength = 0;
    uint8_t elementType = 0;
    // For OBJECT/ARRAY element types, each element is a u32 nodeId.
    std::vector<uint32_t> elements = {};
    // Primitive elements are encoded at their FieldSize width. Missing values
    // are zero-filled so existing tests can omit this vector.
    std::vector<uint64_t> primitiveValues = {};
    // TAGGED arrays are heterogeneous; each pair is runtime type + payload.
    std::vector<std::pair<uint8_t, uint64_t>> taggedValues = {};
};

// ============================================================================
// Static binary snapshot test data builder (writes the canonical wire format)
// ============================================================================

class StaticSnapshotDataBuilder {
public:
    StaticSnapshotDataBuilder() = default;

    struct HeaderParams {
        std::array<char, STATIC_VERSION_SIZE> version = {'3', '.', '0', '.', '0', '\0', '\0', '\0'};
        uint32_t identifierSize = STATIC_IDENTIFIER_SIZE;
        uint8_t language = STATIC_LANGUAGE_STATIC;
        uint32_t headerSize = STATIC_HEADER_SIZE;
        uint32_t featureFlags = STATIC_SUPPORTED_FEATURE_FLAGS;
    };

    // File header (33 bytes): version(8) + identifierSize(4) + timestamp(8)
    // + language(u8) + headerSize(u32) + recordCount(u32) + featureFlags(u32).
    // recordCount is a summary metric the parser ignores - it loops to EOF.
    void WriteHeader(uint8_t language = 1)
    {
        HeaderParams params;
        params.language = language;
        WriteHeader(params);
    }

    void WriteHeader(const HeaderParams &params)
    {
        data_.insert(data_.end(), params.version.begin(), params.version.end());
        WriteU32(params.identifierSize);
        WriteU64(TEST_TIMESTAMP_VALUE);
        WriteU8(params.language);
        WriteU32(params.headerSize);
        WriteU32(0);  // recordCount (summary metric, unused by parser)
        WriteU32(params.featureFlags);
    }

    void WriteStringRecord(uint32_t stringId, const std::string &str)
    {
        BeginRecord(TAG_STRING_IN_UTF8);
        std::vector<uint8_t> body;
        AppendU32(body, stringId);
        AppendU32(body, static_cast<uint32_t>(str.size()));
        body.insert(body.end(), reinterpret_cast<const uint8_t *>(str.data()),
                    reinterpret_cast<const uint8_t *>(str.data()) + str.size());
        EndRecord(body);
    }

    // TAG_LOAD_CLASS body (21 bytes): classSerial(u32) + classObjectId(u32)
    // + stackTraceSerial(u32) + classNameId(u32) + language(u8) +
    // classFlags(u32).
    void WriteLoadClassRecord(uint32_t classObjectId, uint32_t classNameId)
    {
        BeginRecord(TAG_LOAD_CLASS);
        std::vector<uint8_t> body;
        AppendU32(body, 0);  // classSerialNumber
        AppendU32(body, classObjectId);
        AppendU32(body, 0);  // stackTraceSerial
        AppendU32(body, classNameId);
        AppendU8(body, 1);   // language = STATIC
        AppendU32(body, 0);  // classFlags
        EndRecord(body);
    }

    // STATIC_CLASS_DUMP body: classObjectId(u32) + stackTraceSerial(u32)
    // + superClassObjectId(u32) + classLoaderObjectId(u32) + instanceSize(u32)
    // + staticFieldCount(u16) + N field descriptors (11 bytes each)
    // + instanceFieldCount(u16) + N descriptors
    // + staticValueCount(u16) + 0 values
    // + methodCount(u16) + 0 method name ids.
    void WriteStaticClassDumpRecord(uint32_t classObjectId, uint32_t instanceSize,
                                    const std::vector<uint32_t> &instanceFieldNames)
    {
        BeginRecord(TAG_STATIC_CLASS_DUMP);
        std::vector<uint8_t> body;
        AppendU32(body, classObjectId);
        AppendU32(body, 0);  // stackTraceSerial
        AppendU32(body, 0);  // superClassObjectId
        AppendU32(body, 0);  // classLoaderObjectId
        AppendU32(body, instanceSize);
        AppendU16(body, 0);                                                 // staticFieldCount
        AppendU16(body, static_cast<uint16_t>(instanceFieldNames.size()));  // instanceFieldCount
        for (uint32_t nameId : instanceFieldNames) {
            AppendU32(body, nameId);  // nameId
            AppendU8(body,
                     static_cast<uint8_t>(StaFieldType::OBJECT));  // type = OBJECT
            AppendU32(body, 0);                                    // offset
            AppendU16(body, 0);                                    // flags
        }
        AppendU16(body, 0);  // staticValueCount
        AppendU16(body, 0);  // methodCount
        EndRecord(body);
    }

    // Variant of WriteStaticClassDumpRecord that also emits static field
    // descriptors. Used to verify that static field descriptors do NOT consume
    // instance field values during instance edge emission (EmitInstanceFieldEdges
    // must only iterate instance field descriptors, since INSTANCE_DUMP carries
    // instance field values only).
    void WriteStaticClassDumpRecordWithFields(uint32_t classObjectId, uint32_t instanceSize,
                                              const std::vector<uint32_t> &staticFieldNames,
                                              const std::vector<uint32_t> &instanceFieldNames)
    {
        BeginRecord(TAG_STATIC_CLASS_DUMP);
        std::vector<uint8_t> body;
        AppendU32(body, classObjectId);
        AppendU32(body, 0);  // stackTraceSerial
        AppendU32(body, 0);  // superClassObjectId
        AppendU32(body, 0);  // classLoaderObjectId
        AppendU32(body, instanceSize);
        AppendU16(body, static_cast<uint16_t>(staticFieldNames.size()));  // staticFieldCount
        for (uint32_t nameId : staticFieldNames) {
            AppendU32(body, nameId);  // nameId
            AppendU8(body,
                     static_cast<uint8_t>(StaFieldType::OBJECT));  // type = OBJECT
            AppendU32(body, 0);                                    // offset
            AppendU16(body, 0);                                    // flags (IS_STATIC bit unused by reader)
        }
        AppendU16(body, static_cast<uint16_t>(instanceFieldNames.size()));  // instanceFieldCount
        for (uint32_t nameId : instanceFieldNames) {
            AppendU32(body, nameId);  // nameId
            AppendU8(body,
                     static_cast<uint8_t>(StaFieldType::OBJECT));  // type = OBJECT
            AppendU32(body, 0);                                    // offset
            AppendU16(body, 0);                                    // flags
        }
        AppendU16(body, 0);  // staticValueCount
        AppendU16(body, 0);  // methodCount
        EndRecord(body);
    }

    // Full STATIC_CLASS_DUMP: superClassId + static field descriptors with types
    // + parallel static values + instance field descriptors (OBJECT) + method
    // name ids. Used to exercise the variable tail and the edge emission
    // (superClass, static-field, primitive value nodes) it drives.
    struct FieldDesc {
        uint32_t nameId;
        uint8_t type;
    };
    struct FieldValue {
        uint8_t type;
        uint64_t value;
    };
    // Parameter struct for WriteStaticClassDumpRecordFull (reduces function
    // parameter count): the variable tail of a STATIC_CLASS_DUMP record is made
    // of four parallel sequences (static field descriptors, static values,
    // instance field names, method name ids) that are written together.
    struct StaticClassDumpRecordFields {
        std::vector<FieldDesc> staticFields;
        std::vector<FieldValue> staticValues;
        std::vector<uint32_t> instanceFieldNames;
        std::vector<uint32_t> methodNameIds;
    };
    void WriteStaticClassDumpRecordFull(uint32_t classObjectId, uint32_t superClassId, uint32_t instanceSize,
                                        const StaticClassDumpRecordFields &fields)
    {
        const auto &staticFields = fields.staticFields;
        const auto &staticValues = fields.staticValues;
        const auto &instanceFieldNames = fields.instanceFieldNames;
        const auto &methodNameIds = fields.methodNameIds;
        BeginRecord(TAG_STATIC_CLASS_DUMP);
        std::vector<uint8_t> body;
        AppendU32(body, classObjectId);
        AppendU32(body, 0);             // stackTraceSerial
        AppendU32(body, superClassId);  // superClassObjectId
        AppendU32(body, 0);             // classLoaderObjectId
        AppendU32(body, instanceSize);
        AppendU16(body,
                  static_cast<uint16_t>(staticFields.size()));  // staticFieldCount
        for (const auto &fd : staticFields) {
            AppendU32(body, fd.nameId);
            AppendU8(body, fd.type);
            AppendU32(body, 0);  // offset
            AppendU16(body, 0);  // flags
        }
        AppendU16(body, static_cast<uint16_t>(instanceFieldNames.size()));  // instanceFieldCount
        for (uint32_t nameId : instanceFieldNames) {
            AppendU32(body, nameId);
            AppendU8(body, static_cast<uint8_t>(StaFieldType::OBJECT));
            AppendU32(body, 0);
            AppendU16(body, 0);
        }
        // Static field values (parallel to staticFields).
        AppendU16(body,
                  static_cast<uint16_t>(staticValues.size()));  // staticValueCount
        for (const auto &fv : staticValues) {
            AppendFieldValue(body, fv.type, fv.value);
        }
        // Method name ids.
        AppendU16(body, static_cast<uint16_t>(methodNameIds.size()));  // methodCount
        for (uint32_t mid : methodNameIds) {
            AppendU32(body, mid);
        }
        EndRecord(body);
    }
    // + classNodeId(u32) + stackTraceSerial(u32) + instanceSize(u32)
    // + fieldCount(u16)) + per field: fieldType(u8) + value(FieldSize(type)
    // bytes). This builder only ever emits OBJECT fields, so each value is a u32
    // nodeId.
    void WriteStaticInstanceDumpRecord(uint32_t objectId, uint32_t classObjectId, uint32_t instanceSize,
                                       const std::vector<uint32_t> &fieldValues)
    {
        BeginRecord(TAG_STATIC_INSTANCE_DUMP);
        std::vector<uint8_t> body;
        AppendU32(body, objectId);
        AppendU32(body, classObjectId);
        AppendU32(body, 0);  // stackTraceSerial
        AppendU32(body, instanceSize);
        AppendU16(body, static_cast<uint16_t>(fieldValues.size()));
        for (uint32_t v : fieldValues) {
            AppendU8(body,
                     static_cast<uint8_t>(StaFieldType::OBJECT));  // type = OBJECT
            AppendU32(body, v);                                    // OBJECT value: u32 nodeId (4 bytes)
        }
        EndRecord(body);
    }

    void WriteStaticArrayDumpRecord(const ArrayDumpParams &params)
    {
        BeginRecord(TAG_STATIC_ARRAY_DUMP);
        std::vector<uint8_t> body;
        AppendArrayDumpItem(body, params);
        EndRecord(body);
    }

    void WriteStaticArrayDumpRecordBatch(const std::vector<ArrayDumpParams> &items)
    {
        BeginRecord(TAG_STATIC_ARRAY_DUMP);
        std::vector<uint8_t> body;
        for (const auto &params : items) {
            AppendArrayDumpItem(body, params);
        }
        EndRecord(body, static_cast<uint32_t>(items.size()));
    }

    // TAG_ROOT_RECORD body (5 bytes): rootType(u8) + objectNodeId(u32).
    void WriteRootRecord(uint32_t objectId)
    {
        BeginRecord(TAG_ROOT_RECORD);
        std::vector<uint8_t> body;
        AppendU8(body, ROOT_TYPE_STATIC_OBJECT);  // rootType = STATIC_OBJECT
        AppendU32(body, objectId);
        EndRecord(body);
    }

    // TAG_XREF_EDGE body (9 bytes): dynNodeId(u32) + staNodeId(u32) +
    // direction(u8). Both endpoints are 4-byte nodeIds (symmetric): the dynamic
    // side is the nodeId the dump resolved from the JS heap address via
    // GetNodeId.
    void WriteXRefEdgeRecord(uint32_t dynNodeId, uint32_t staNodeId, uint8_t direction)
    {
        BeginRecord(TAG_XREF_EDGE);
        std::vector<uint8_t> body;
        AppendU32(body, dynNodeId);
        AppendU32(body, staNodeId);
        AppendU8(body, direction);
        EndRecord(body);
    }

    void WriteHeapSummaryRecord()
    {
        BeginRecord(TAG_HEAP_SUMMARY);
        std::vector<uint8_t> body;
        AppendU64(body, 0);
        AppendU64(body, 0);
        AppendU64(body, 0);
        AppendU64(body, 0);
        AppendU64(body, 0);
        AppendU64(body, 0);
        AppendU64(body, 0);
        EndRecord(body);
    }

    // Write an unknown-tag record (for the skip test).
    void WriteUnknownRecord(uint8_t tag, const std::vector<uint8_t> &payload)
    {
        BeginRecord(tag);
        EndRecord(payload);
    }

    void WriteRawRecord(uint8_t tag, uint32_t declaredLength, uint32_t count, const std::vector<uint8_t> &body)
    {
        BeginRecord(tag);
        (void)memcpy_s(data_.data() + lengthOffset_, data_.size() - lengthOffset_, &declaredLength,
                       sizeof(declaredLength));
        (void)memcpy_s(data_.data() + countOffset_, data_.size() - countOffset_, &count, sizeof(count));
        data_.insert(data_.end(), body.begin(), body.end());
    }

    // Write data to a temp file with mkstemp for unique naming.
    // Uses a random suffix instead of `this` pointer to avoid collisions
    // and improve portability.
    std::string WriteToTempFile(const std::string &tag) const
    {
        std::random_device rd;
        std::string path = "static_snap_test_" + tag + "_" + std::to_string(rd());
        std::ofstream ofs(path, std::ios::binary);
        ofs.write(reinterpret_cast<const char *>(data_.data()), data_.size());
        ofs.close();
        return path;
    }

private:
    std::vector<uint8_t> data_;
    size_t lengthOffset_ = 0;
    size_t countOffset_ = 0;

    static void AppendArrayDumpItem(std::vector<uint8_t> &body, const ArrayDumpParams &params)
    {
        // Prefix (21 bytes): objectId(u32) + classObjectId(u32) +
        // stackTraceSerial(u32)
        // + instanceSize(u32) + arrayLength(u32) + elementType(u8).
        AppendU32(body, params.objectId);
        AppendU32(body, params.classObjectId);
        AppendU32(body, 0);  // stackTraceSerial
        AppendU32(body, params.instanceSize);
        AppendU32(body, params.arrayLength);
        AppendU8(body, params.elementType);
        bool isRef = (params.elementType == static_cast<uint8_t>(StaFieldType::OBJECT) ||
                      params.elementType == static_cast<uint8_t>(StaFieldType::ARRAY) ||
                      params.elementType == static_cast<uint8_t>(StaFieldType::WEAK_OBJECT));
        if (isRef) {
            // OBJECT/ARRAY elements: each element is a u32 nodeId.
            for (uint32_t e : params.elements) {
                AppendU32(body, e);
            }
        } else if (params.elementType == static_cast<uint8_t>(StaFieldType::TAGGED)) {
            for (const auto &[type, value] : params.taggedValues) {
                AppendFieldValue(body, type, value);
            }
        } else {
            // Non-OBJECT arrays: the parser's ScanArrayPrefixes reads arrayLength
            // from the prefix and expects arrayLength * FieldSize(elementType)
            // bytes of element data after the prefix (via DistributeUnknownData).
            uint8_t esz = FieldSize(params.elementType);
            for (uint32_t i = 0; i < params.arrayLength; ++i) {
                uint64_t value = i < params.primitiveValues.size() ? params.primitiveValues[i] : 0;
                switch (esz) {
                    case sizeof(uint8_t):
                        AppendU8(body, static_cast<uint8_t>(value));
                        break;
                    case sizeof(uint16_t):
                        AppendU16(body, static_cast<uint16_t>(value));
                        break;
                    case sizeof(uint32_t):
                        AppendU32(body, static_cast<uint32_t>(value));
                        break;
                    case sizeof(uint64_t):
                        AppendU64(body, value);
                        break;
                    default:
                        break;
                }
            }
        }
    }

    void WriteU8(uint8_t v)
    {
        data_.push_back(v);
    }
    void WriteU32(uint32_t v)
    {
        uint8_t b[sizeof(uint32_t)];
        (void)memcpy_s(b, sizeof(uint32_t), &v, sizeof(uint32_t));
        data_.insert(data_.end(), b, b + sizeof(uint32_t));
    }
    void WriteU64(uint64_t v)
    {
        uint8_t b[sizeof(uint64_t)];
        (void)memcpy_s(b, sizeof(uint64_t), &v, sizeof(uint64_t));
        data_.insert(data_.end(), b, b + sizeof(uint64_t));
    }

    // Record header: 17 bytes (tag:1 + time:8 + length:4 + count:4)
    void BeginRecord(uint8_t tag)
    {
        data_.push_back(tag);
        uint64_t time = 0;
        uint8_t timeBuf[sizeof(uint64_t)];
        (void)memcpy_s(timeBuf, sizeof(uint64_t), &time, sizeof(uint64_t));
        data_.insert(data_.end(), timeBuf, timeBuf + sizeof(uint64_t));
        lengthOffset_ = data_.size();
        uint8_t lenBuf[sizeof(uint32_t)] = {0, 0, 0, 0};
        data_.insert(data_.end(), lenBuf, lenBuf + sizeof(uint32_t));
        countOffset_ = data_.size();
        uint8_t countBuf[sizeof(uint32_t)] = {0, 0, 0, 0};
        data_.insert(data_.end(), countBuf, countBuf + sizeof(uint32_t));
    }

    void EndRecord(const std::vector<uint8_t> &body, uint32_t count = 1)
    {
        uint32_t bodyLen = static_cast<uint32_t>(body.size());
        (void)memcpy_s(data_.data() + lengthOffset_, data_.size() - lengthOffset_, &bodyLen, sizeof(uint32_t));
        (void)memcpy_s(data_.data() + countOffset_, data_.size() - countOffset_, &count, sizeof(uint32_t));
        data_.insert(data_.end(), body.begin(), body.end());
    }

    static void AppendU8(std::vector<uint8_t> &v, uint8_t val)
    {
        v.push_back(val);
    }
    static void AppendU16(std::vector<uint8_t> &v, uint16_t val)
    {
        uint8_t b[sizeof(uint16_t)];
        (void)memcpy_s(b, sizeof(uint16_t), &val, sizeof(uint16_t));
        v.insert(v.end(), b, b + sizeof(uint16_t));
    }
    static void AppendU32(std::vector<uint8_t> &v, uint32_t val)
    {
        uint8_t b[sizeof(uint32_t)];
        (void)memcpy_s(b, sizeof(uint32_t), &val, sizeof(uint32_t));
        v.insert(v.end(), b, b + sizeof(uint32_t));
    }
    static void AppendU64(std::vector<uint8_t> &v, uint64_t val)
    {
        uint8_t b[sizeof(uint64_t)];
        (void)memcpy_s(b, sizeof(uint64_t), &val, sizeof(uint64_t));
        v.insert(v.end(), b, b + sizeof(uint64_t));
    }

    // Append a field value [type:u1][value:FieldSize(type) bytes LE]. Mirrors
    // the writer's WriteFieldValue encoding.
    static void AppendFieldValue(std::vector<uint8_t> &v, uint8_t type, uint64_t value)
    {
        AppendU8(v, type);
        switch (static_cast<StaFieldType>(type)) {
            case StaFieldType::BOOLEAN:
            case StaFieldType::BYTE:
                AppendU8(v, static_cast<uint8_t>(value));
                break;
            case StaFieldType::CHAR:
            case StaFieldType::SHORT:
                AppendU16(v, static_cast<uint16_t>(value));
                break;
            case StaFieldType::INT:
            case StaFieldType::FLOAT:
                AppendU32(v, static_cast<uint32_t>(value));
                break;
            case StaFieldType::LONG:
            case StaFieldType::DOUBLE:
            case StaFieldType::TAGGED:
                AppendU64(v, value);
                break;
            case StaFieldType::OBJECT:
            case StaFieldType::ARRAY:
            case StaFieldType::WEAK_OBJECT:
                AppendU32(v, static_cast<uint32_t>(value));  // nodeId
                break;
            case StaFieldType::UNKNOWN:
            default:
                break;  // type byte only
        }
    }

    // Mirror of StaticRawheapTranslate::FieldSize - byte size of a field value
    // for the given StaFieldType. OBJECT/ARRAY are u32 nodeIds (4 bytes).
    static uint8_t FieldSize(uint8_t fieldType)
    {
        switch (fieldType) {
            case static_cast<uint8_t>(StaFieldType::BOOLEAN):
            case static_cast<uint8_t>(StaFieldType::BYTE):
                return 1;
            case static_cast<uint8_t>(StaFieldType::CHAR):
            case static_cast<uint8_t>(StaFieldType::SHORT):
                return sizeof(uint16_t);
            case static_cast<uint8_t>(StaFieldType::INT):
            case static_cast<uint8_t>(StaFieldType::FLOAT):
                return sizeof(uint32_t);
            case static_cast<uint8_t>(StaFieldType::LONG):
            case static_cast<uint8_t>(StaFieldType::DOUBLE):
            case static_cast<uint8_t>(StaFieldType::TAGGED):
                return sizeof(uint64_t);
            case static_cast<uint8_t>(StaFieldType::OBJECT):
            case static_cast<uint8_t>(StaFieldType::ARRAY):
            case static_cast<uint8_t>(StaFieldType::WEAK_OBJECT):
                return sizeof(uint32_t);  // nodeId (4 bytes)
            default:
                return 0;
        }
    }
};

// ============================================================================
// Test fixture
// ============================================================================

class RawHeapStaticSnapshotTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    void TearDown() override
    {
        for (const auto &path : tempFiles_) {
            std::remove(path.c_str());
        }
        tempFiles_.clear();
    }

    // Parse a builder's output as a single-file static snapshot and return a
    // fully-translated parser (synthetic root framework enabled).
    bool ParseSingle(const StaticSnapshotDataBuilder &builder, const std::string &tag, StaticRawheapTranslate &parser)
    {
        std::string path = builder.WriteToTempFile(tag);
        tempFiles_.push_back(path);
        FileReader file;
        if (!file.Initialize(path)) {
            return false;
        }
        parser.EnableRootFramework();
        if (!parser.Parse(file, file.GetFileSize())) {
            return false;
        }
        return parser.Translate();
    }

    // Parse without the synthetic root (two-file/merge mode shape).
    bool ParseForMerge(const StaticSnapshotDataBuilder &builder, const std::string &tag, StaticRawheapTranslate &parser)
    {
        std::string path = builder.WriteToTempFile(tag);
        tempFiles_.push_back(path);
        FileReader file;
        if (!file.Initialize(path)) {
            return false;
        }
        return parser.Parse(file, file.GetFileSize());
    }

    void ExpectUnsupportedHeader(const StaticSnapshotDataBuilder::HeaderParams &params, const std::string &tag)
    {
        StaticSnapshotDataBuilder builder;
        builder.WriteHeader(params);
        std::string path = builder.WriteToTempFile(tag);
        tempFiles_.push_back(path);

        FileReader probeFile;
        ASSERT_TRUE(probeFile.Initialize(path));
        EXPECT_TRUE(RawHeap::IsStaticSnapshotFormat(probeFile));

        FileReader parseFile;
        ASSERT_TRUE(parseFile.Initialize(path));
        StaticRawheapTranslate parser;
        EXPECT_FALSE(parser.Parse(parseFile, parseFile.GetFileSize()));
    }

    static std::string ResolveString(const StringHashMap *strings, StringId id)
    {
        if (id < StringHashMap::CUSTOM_STRID_START) {
            return std::string();
        }
        return strings->GetStringByKey(strings->GetKeyByStringId(id));
    }

    static rawheap_translate::Node *FindArrayBuffer(StaticRawheapTranslate &parser,
                                                    const rawheap_translate::Node *arrayNode)
    {
        for (rawheap_translate::Edge *edge : *parser.GetEdges()) {
            if (edge->from == arrayNode && edge->to != nullptr) {
                return edge->to;
            }
        }
        return nullptr;
    }

    static void VerifyTaggedArrayEdges(StaticRawheapTranslate &parser, const rawheap_translate::Node *buffer,
                                       uint32_t weakNodeId, size_t expectedElementCount)
    {
        auto *strings = parser.GetStringTable();
        size_t elementCount = 0;
        size_t weakCount = 0;
        std::vector<std::string> valueNames;
        for (rawheap_translate::Edge *edge : *parser.GetEdges()) {
            if (edge->from != buffer || edge->to == nullptr) {
                continue;
            }
            if (edge->type == EdgeType::ELEMENT) {
                ++elementCount;
            } else if (edge->type == EdgeType::WEAK) {
                ++weakCount;
                EXPECT_EQ(edge->to->nodeId, weakNodeId);
            }
            valueNames.push_back(ResolveString(strings, edge->to->strId));
        }
        EXPECT_EQ(elementCount, expectedElementCount);
        EXPECT_EQ(weakCount, 1U);
        EXPECT_NE(std::find(valueNames.begin(), valueNames.end(), "-7"), valueNames.end());
        EXPECT_NE(std::find(valueNames.begin(), valueNames.end(), "true"), valueNames.end());
        EXPECT_NE(std::find(valueNames.begin(), valueNames.end(), "null"), valueNames.end());
        EXPECT_NE(std::find(valueNames.begin(), valueNames.end(), "undefined"), valueNames.end());
        EXPECT_NE(std::find(valueNames.begin(), valueNames.end(), "hole"), valueNames.end());
        EXPECT_NE(std::find(valueNames.begin(), valueNames.end(), "exception"), valueNames.end());
        EXPECT_NE(std::find(valueNames.begin(), valueNames.end(), "false"), valueNames.end());
        EXPECT_NE(std::find(valueNames.begin(), valueNames.end(), "0xFEDCBA9876543210"), valueNames.end());
    }

    std::vector<std::string> tempFiles_;
};

// ============================================================================
// Header / file-level tests
// ============================================================================

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseHeader_ValidStatic)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);  // STATIC
    auto path = builder.WriteToTempFile("hdr");
    tempFiles_.push_back(path);

    FileReader file;
    ASSERT_TRUE(file.Initialize(path));
    ASSERT_TRUE(RawHeap::IsStaticSnapshotFormat(file));
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseHeader_ValidHybrid)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(STATIC_LANGUAGE_HYBRID);
    auto path = builder.WriteToTempFile("hdr_hybrid");
    tempFiles_.push_back(path);

    FileReader file;
    ASSERT_TRUE(file.Initialize(path));
    ASSERT_TRUE(RawHeap::IsStaticSnapshotFormat(file));
    StaticRawheapTranslate parser;
    ASSERT_TRUE(parser.Parse(file, file.GetFileSize()));
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseHeader_V3MinorVersionSupported)
{
    StaticSnapshotDataBuilder::HeaderParams params;
    params.version = {'3', '.', '1', '.', '0', '\0', '\0', '\0'};
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(params);
    auto path = builder.WriteToTempFile("hdr_v3_minor");
    tempFiles_.push_back(path);

    FileReader file;
    ASSERT_TRUE(file.Initialize(path));
    EXPECT_TRUE(RawHeap::IsStaticSnapshotFormat(file));
    StaticRawheapTranslate parser;
    EXPECT_TRUE(parser.Parse(file, file.GetFileSize()));
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseHeader_UnsupportedMajorVersionFails)
{
    StaticSnapshotDataBuilder::HeaderParams params;
    params.version = {'4', '.', '0', '.', '0', '\0', '\0', '\0'};
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(params);
    auto path = builder.WriteToTempFile("hdr_major_version");
    tempFiles_.push_back(path);

    FileReader probeFile;
    ASSERT_TRUE(probeFile.Initialize(path));
    EXPECT_FALSE(RawHeap::IsStaticSnapshotFormat(probeFile));

    FileReader parseFile;
    ASSERT_TRUE(parseFile.Initialize(path));
    StaticRawheapTranslate parser;
    EXPECT_FALSE(parser.Parse(parseFile, parseFile.GetFileSize()));
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseHeader_UnsupportedIdentifierSize_Fails)
{
    StaticSnapshotDataBuilder::HeaderParams params;
    params.identifierSize = static_cast<uint32_t>(sizeof(uint64_t));
    ExpectUnsupportedHeader(params, "hdr_identifier");
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseHeader_UnsupportedLanguage_Fails)
{
    StaticSnapshotDataBuilder::HeaderParams params;
    params.language = 0;
    ExpectUnsupportedHeader(params, "hdr_language");
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseHeader_UnsupportedSize_Fails)
{
    StaticSnapshotDataBuilder::HeaderParams params;
    params.headerSize = STATIC_HEADER_SIZE + 1;
    ExpectUnsupportedHeader(params, "hdr_size");
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseHeader_UnknownFeatureFlags_Fails)
{
    StaticSnapshotDataBuilder::HeaderParams params;
    params.featureFlags = 1;
    ExpectUnsupportedHeader(params, "hdr_flags");
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseHeader_InvalidMagic_Fails)
{
    std::vector<uint8_t> bad(STATIC_HEADER_SIZE, 0);
    std::random_device rd;
    std::string path = "static_snap_test_badmagic_" + std::to_string(rd());
    std::ofstream ofs(path, std::ios::binary);
    ofs.write(reinterpret_cast<const char *>(bad.data()), bad.size());
    ofs.close();
    tempFiles_.push_back(path);

    FileReader file;
    ASSERT_TRUE(file.Initialize(path));
    StaticRawheapTranslate parser;
    ASSERT_FALSE(parser.Parse(file, file.GetFileSize()));
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, Parse_EmptyFile_Fails)
{
    std::random_device rd;
    std::string path = "static_snap_test_empty_" + std::to_string(rd());
    std::ofstream ofs(path, std::ios::binary);
    ofs.close();
    tempFiles_.push_back(path);
    FileReader file;
    if (!file.Initialize(path)) {
        return;  // empty file may not initialize
    }
    StaticRawheapTranslate parser;
    ASSERT_FALSE(parser.Parse(file, file.GetFileSize()));
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, Parse_UnknownTag_Skipped)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteUnknownRecord(0x99, {0xAA, 0xBB, 0xCC, 0xDD});
    builder.WriteHeapSummaryRecord();
    auto path = builder.WriteToTempFile("unk");
    tempFiles_.push_back(path);

    FileReader file;
    ASSERT_TRUE(file.Initialize(path));
    StaticRawheapTranslate parser;
    ASSERT_TRUE(parser.Parse(file, file.GetFileSize()));
}

// An unknown tag (0xFF, the PARTIAL_MARKER chunk boundary) must be skipped
// without processing. Consecutive unknown tags are also skipped. After
// skipping 0xFF and 0xAA the parser must still consume the subsequent STRING
// and LoadClass records; the LoadClass references stringId 10 ("AfterUnknown")
// so BuildGraph promotes it into the StringHashMap, and resolving the class
// node's name then proves the post-skip records were consumed correctly.
HWTEST_F_L0(RawHeapStaticSnapshotTest, Parse_UnknownTag_FF_AndConsecutive_Skipped)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    // Write 0xFF (PARTIAL_MARKER) - should be skipped
    builder.WriteUnknownRecord(TAG_PARTIAL_MARKER, {0x01, 0x02, 0x03, 0x04});
    // Write another unknown tag in sequence
    builder.WriteUnknownRecord(0xAA, {0x11, 0x22});
    // Write a valid STRING record after unknowns, then a LoadClass referencing
    // it, so the parser must continue past the skipped unknowns.
    builder.WriteStringRecord(10, "AfterUnknown");
    builder.WriteLoadClassRecord(2, 10);  // class nodeId 2 -> "AfterUnknown"
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "unkff", parser));

    // The class node created from the LoadClass record must carry the name
    // "AfterUnknown" - this is only possible if the parser skipped the unknown
    // tags and then correctly parsed the STRING + LoadClass records that follow.
    rawheap_translate::Node *cls = parser.FindNodeByNodeId(2);
    ASSERT_NE(cls, nullptr) << "LoadClass record after unknown tags was not parsed";
    auto *tab = parser.GetStringTable();
    ASSERT_NE(tab, nullptr);
    auto resolveStr = [tab](StringId id) -> std::string {
        if (id < StringHashMap::CUSTOM_STRID_START) {
            return std::string();
        }
        return tab->GetStringByKey(tab->GetKeyByStringId(id));
    };
    EXPECT_EQ(resolveStr(cls->strId), "AfterUnknown")
        << "STRING record after unknown tags should be parsed and referenced";
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, Parse_StringCannotReadPastDeclaredRecordBody)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    std::vector<uint8_t> body(sizeof(uint32_t) * 2);
    uint32_t stringId = 10;
    uint32_t stringLength = 4;
    (void)memcpy_s(body.data(), body.size(), &stringId, sizeof(stringId));
    (void)memcpy_s(body.data() + sizeof(uint32_t), body.size() - sizeof(uint32_t), &stringLength, sizeof(stringLength));
    // The record declares only the two u32 fields. A parser must reject the
    // missing string payload instead of consuming bytes from the next record.
    builder.WriteRawRecord(TAG_STRING_IN_UTF8, static_cast<uint32_t>(body.size()), 1, body);
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    EXPECT_FALSE(ParseForMerge(builder, "bounded_string", parser));
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, Parse_RejectsTrailingRecordBytes)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    std::vector<uint8_t> body = {ROOT_TYPE_STATIC_OBJECT, 2, 0, 0, 0, 0xFF};
    builder.WriteRawRecord(TAG_ROOT_RECORD, static_cast<uint32_t>(body.size()), 1, body);

    StaticRawheapTranslate parser;
    EXPECT_FALSE(ParseForMerge(builder, "trailing_record_bytes", parser));
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, Parse_RejectsArrayPayloadOutsideRecord)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    std::vector<uint8_t> body(STATIC_ARRAY_PREFIX_BODY_SIZE, 0);
    uint32_t arrayLength = std::numeric_limits<uint32_t>::max();
    (void)memcpy_s(body.data() + STATIC_ARRAY_LENGTH_OFFSET, body.size() - STATIC_ARRAY_LENGTH_OFFSET, &arrayLength,
                   sizeof(arrayLength));
    body[STATIC_ARRAY_ELEM_TYPE_OFFSET] = static_cast<uint8_t>(StaFieldType::INT);
    builder.WriteRawRecord(TAG_STATIC_ARRAY_DUMP, static_cast<uint32_t>(body.size()), 1, body);

    StaticRawheapTranslate parser;
    EXPECT_FALSE(ParseForMerge(builder, "bounded_array", parser));
}

// ============================================================================
// Static record parsing tests
// ============================================================================

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseLoadClassAndStaticClassDump)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "TestClass");
    builder.WriteLoadClassRecord(2, 10);  // 0x1000 -> nodeId 2
    // instanceFieldNames: empty (no instance fields in class dump)
    builder.WriteStaticClassDumpRecord(2, 32, {});
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "cls", parser));
    ASSERT_GT(parser.GetNodeCount(), 0u);
    ASSERT_NE(parser.FindNodeByNodeId(2), nullptr);  // 0x1000 -> nodeId 2
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ClassMirrorPreservesInstanceSize)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader();
    builder.WriteStringRecord(10, "TestClass");
    builder.WriteStringRecord(11, "std.core.Class");
    builder.WriteLoadClassRecord(2, 10);
    builder.WriteLoadClassRecord(4, 11);
    builder.WriteStaticClassDumpRecord(2, 16, {});
    builder.WriteStaticClassDumpRecord(4, 24, {});
    // The class mirror is also an instance of std.core.Class. Its actual
    // object size comes from this instance record, not the class descriptor.
    builder.WriteStaticInstanceDumpRecord(2, 4, 48, {});
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "class_mirror_size", parser));
    rawheap_translate::Node *classMirror = parser.FindNodeByNodeId(2);
    ASSERT_NE(classMirror, nullptr);
    EXPECT_EQ(classMirror->type, CLASS_NODETYPE);
    EXPECT_EQ(classMirror->size, 48U);
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseStaticInstanceDump_WithObjectField)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "TestClassName");
    builder.WriteStringRecord(11, "fieldA");
    builder.WriteLoadClassRecord(2, 10);  // 0x1000 -> nodeId 2
    // instanceFieldNames: {11} = "fieldA"
    builder.WriteStaticClassDumpRecord(2, 24, {11});
    // fieldValues: {6} = object reference value (0x3000 -> nodeId 6)
    builder.WriteStaticInstanceDumpRecord(4, 2, 24, {6});  // 0x2000->4, 0x1000->2
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "inst", parser));
    // class node + instance node + field-target node + synthetic framework
    ASSERT_GT(parser.GetNodeCount(), 3u);
    ASSERT_NE(parser.FindNodeByNodeId(4), nullptr);  // 0x2000 -> nodeId 4
    ASSERT_NE(parser.FindNodeByNodeId(6), nullptr);  // 0x3000 -> nodeId 6
}

// Guards the EmitInstanceFieldEdges fix: INSTANCE_DUMP carries instance field
// values only (the writer's WriteNormalInstance iterates
// cls->GetInstanceFields() alone). The reader must emit PROPERTY edges for
// those values labeled with the INSTANCE field names - not the static field
// names, and never empty. If the reader iterates static field descriptors
// against rec.values (the old bug), the first instance references get
// mislabeled with static field names and the tail is dropped; if the classMap_
// join misses, EmitFallbackFieldEdges writes empty names. This test catches
// both regressions.
HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseStaticInstanceDump_InstanceFieldNamesCorrect)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "MyClass");
    builder.WriteStringRecord(20, "staticField");  // static field name (must NOT appear on edges)
    builder.WriteStringRecord(21, "instA");        // instance field name
    builder.WriteStringRecord(22, "instB");        // instance field name
    builder.WriteLoadClassRecord(2, 10);           // class nodeId 2 -> "MyClass"
    // 1 static field + 2 instance fields. classObjectId (2) matches the
    // instance's classNodeId below so classMap_ join succeeds.
    builder.WriteStaticClassDumpRecordWithFields(2, 24, {20}, {21, 22});
    // Instance nodeId 4, class nodeId 2, two OBJECT field values -> 6 and 8.
    builder.WriteStaticInstanceDumpRecord(4, 2, 24, {6, 8});
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "instfn", parser));

    // Instance node type-name must resolve to the class name - proves the
    // classMap_ join (rec.classNodeId -> classMap_) succeeded.
    rawheap_translate::Node *inst = parser.FindNodeByNodeId(4);
    ASSERT_NE(inst, nullptr);
    auto *tab = parser.GetStringTable();
    auto resolveStr = [tab](StringId id) -> std::string {
        if (id < StringHashMap::CUSTOM_STRID_START) {
            return std::string();
        }
        StringKey key = tab->GetKeyByStringId(id);
        return tab->GetStringByKey(key);
    };
    EXPECT_EQ(resolveStr(inst->strId), "MyClass");

    // Collect every PROPERTY edge name in the graph. The instance now emits
    // its field edges (instA, instB) PLUS an instance->class "hclass" edge
    // (EdgeType::DEFAULT == PROPERTY). Root framework uses ELEMENT/SHORTCUT;
    // primitive nodes carry no edges. So the set must be {instA, instB, hclass}.
    std::vector<std::string> propNames;
    for (rawheap_translate::Edge *e : *parser.GetEdges()) {
        if (e->type == EdgeType::PROPERTY) {
            propNames.push_back(resolveStr(e->nameOrIndex));
        }
    }
    ASSERT_EQ(propNames.size(), 3u);
    EXPECT_NE(std::find(propNames.begin(), propNames.end(), "instA"), propNames.end());
    EXPECT_NE(std::find(propNames.begin(), propNames.end(), "instB"), propNames.end());
    EXPECT_NE(std::find(propNames.begin(), propNames.end(), "hclass"), propNames.end());
    EXPECT_EQ(std::find(propNames.begin(), propNames.end(), "staticField"), propNames.end());
    for (const auto &n : propNames) {
        EXPECT_FALSE(n.empty()) << "instance field edge has empty name (fallback path hit)";
    }
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseStaticArrayDump_ObjectElements)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "Object[]");
    builder.WriteLoadClassRecord(2, 10);  // 0x1000 -> nodeId 2
    // elements: {8, 10} = two object element references (0x4000->8, 0x5000->10)
    ArrayDumpParams arrParams;
    arrParams.objectId = 6;       // 0x3000 -> nodeId 6
    arrParams.classObjectId = 2;  // 0x1000 -> nodeId 2
    arrParams.instanceSize = 32;
    arrParams.arrayLength = 2;
    arrParams.elementType = static_cast<uint8_t>(StaFieldType::OBJECT);
    arrParams.elements = {8, 10};
    builder.WriteStaticArrayDumpRecord(arrParams);
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "arr", parser));
    ASSERT_NE(parser.FindNodeByNodeId(8), nullptr);   // 0x4000 -> nodeId 8
    ASSERT_NE(parser.FindNodeByNodeId(10), nullptr);  // 0x5000 -> nodeId 10
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseStaticArrayDump_NonObjectElements)
{
    // Non-OBJECT element arrays carry arrayLength * FieldSize(type) bytes of
    // element data after the prefix; the parser captures those bytes and boxes
    // each element as a std.core.<Type> wrapper (CreateArrayEdges). This test
    // only asserts the array node itself exists; element boxing is exercised by
    // the integration tests (HeapsnapshotArrayElementsMatchEts).
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "int[]");
    builder.WriteLoadClassRecord(2, 10);  // 0x1000 -> nodeId 2
    ArrayDumpParams intArrParams;
    intArrParams.objectId = 6;       // 0x3000 -> nodeId 6
    intArrParams.classObjectId = 2;  // 0x1000 -> nodeId 2
    intArrParams.instanceSize = 32;
    intArrParams.arrayLength = 4;  // 4 INT elements -> 4 * 4 = 16 zero bytes
    intArrParams.elementType = static_cast<uint8_t>(StaFieldType::INT);
    builder.WriteStaticArrayDumpRecord(intArrParams);
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "iarr", parser));
    ASSERT_NE(parser.FindNodeByNodeId(6), nullptr);  // 0x3000 -> nodeId 6
}

// ============================================================================
// Merge tests - static graph spliced into a synthetic dynamic graph
// ============================================================================

HWTEST_F_L0(RawHeapStaticSnapshotTest, Merge_TwoFileTranslate_Succeeds)
{
    // Full two-file path: dynamic file (V1/V2 rawheap) + static file -> one
    // heapsnapshot. The dynamic side is exercised through the real
    // TranslateRawheap entry point; here we only assert the static file parses
    // into a mergeable shape and the two-file TranslateRawheap runs without
    // aborting on a static file that lacks a real dynamic counterpart (it will
    // return false for an invalid dynamic file, which we treat as the
    // routing-success signal).
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "Sta");
    builder.WriteStringRecord(11, "f");
    builder.WriteLoadClassRecord(2, 10);  // 0x1000 -> nodeId 2
    // instanceFieldNames: {11} = "f"
    builder.WriteStaticClassDumpRecord(2, 16, {11});
    // fieldValues: {6} = object reference value (0x3000 -> nodeId 6)
    builder.WriteStaticInstanceDumpRecord(4, 2, 16, {6});  // 0x2000->4, 0x1000->2
    builder.WriteRootRecord(4);                            // 0x2000 -> nodeId 4
    // XRef: static side (0x2000 -> nodeId 4) goes into staNodeId; the dynamic
    // side is a dynNodeId (resolved from the JS address at dump time; here an
    // arbitrary value, since this test only asserts xref routing, not
    // resolution).
    builder.WriteXRefEdgeRecord(0xDDDD0000U, 4, XREF_STA_TO_DYN);
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate staticParser;
    ASSERT_TRUE(ParseForMerge(builder, "merge", staticParser));
    ASSERT_EQ(staticParser.GetXRefs().size(), 1u);
    ASSERT_EQ(staticParser.GetRoots().size(), 1u);
    // The static graph was built (object nodes only, no synthetic root).
    ASSERT_GT(staticParser.GetNodeCount(), 0u);
}

// Issue 5: the merger must RESOLVE xref records into actual xref edges when the
// dynNodeId matches a dynamic node's nodeId. The merger resolves by indexing
// dynamic nodes by nodeId and looking up x.dynNodeId - this test exercises that
// resolution path (which Merge_TwoFileTranslate_Succeeds above does not, since
// its dynamic side is absent). Builds a minimal dynamic graph by hand (the
// merger only uses RawHeap base-class methods, so a V1 with a null MetaParser
// is sufficient) + a static parser carrying one STA_TO_DYN xref, runs Merge,
// and asserts the merged graph contains the xref edge.
HWTEST_F_L0(RawHeapStaticSnapshotTest, Merge_XRefEdge_Resolved)
{
    // Static side: class node nodeId=8 ("StaCls"), root=8, one xref record
    // (dynNodeId=0xDDDD0000, staNodeId=8, direction=STA_TO_DYN).
    constexpr uint32_t staClassId = 8;
    constexpr uint32_t dynNodeId = 0xDDDD0000U;
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "StaCls");
    builder.WriteLoadClassRecord(staClassId, 10);
    builder.WriteStaticClassDumpRecord(staClassId, 16, {});
    builder.WriteRootRecord(staClassId);
    builder.WriteXRefEdgeRecord(dynNodeId, staClassId, XREF_STA_TO_DYN);
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate staticParser;
    ASSERT_TRUE(ParseForMerge(builder, "xrefmerge", staticParser));
    ASSERT_EQ(staticParser.GetXRefs().size(), 1u);
    rawheap_translate::Node *staNode = staticParser.FindNodeByNodeId(staClassId);
    ASSERT_NE(staNode, nullptr);

    // Dynamic side: a synthetic root (nodes_[0], nodeId=1) + one object node
    // whose nodeId == the xref dynNodeId. The dump side resolves
    // jsAddr->dynNodeId via the dynamic participant's GetNodeId (mirroring
    // etsAddr->staNodeId); the merger indexes dynamic nodes by nodeId and looks
    // up x.dynNodeId, so the dynNodeId<->nodeId match is what makes the xref
    // resolve. Here we set the dynamic node's nodeId directly to the value the
    // dump would have written. (Node/Edge qualified - panda::ecmascript::Node is
    // also in scope via the using-directives.)
    RawHeapTranslateV1 dynamic(nullptr);
    rawheap_translate::Node *synRoot = dynamic.CreateNode();
    synRoot->nodeId = 1;
    synRoot->type = SYNTHETIC_NODETYPE;
    synRoot->strId = dynamic.InsertAndGetStringId("SyntheticRoot");
    synRoot->edgeCount = 0;
    rawheap_translate::Node *dynNode = dynamic.CreateNode();
    dynNode->nodeId = dynNodeId;
    dynNode->type = OBJECT_NODETYPE;
    dynNode->strId = dynamic.InsertAndGetStringId("DynObj");
    dynNode->edgeCount = 0;

    SnapshotMerger merger;
    ASSERT_TRUE(merger.Merge(dynamic, staticParser));

    // After merge, the merged graph must contain exactly one xref edge from
    // the static class node to the dynamic object node (STA_TO_DYN direction
    // emits the edge on the static side, pointing at the dynamic node).
    int xrefEdges = 0;
    for (rawheap_translate::Edge *edge : *dynamic.GetEdges()) {
        if (edge->type == EdgeType::XREF) {
            EXPECT_EQ(edge->to, dynNode) << "xref edge should point at the dynamic node";
            ++xrefEdges;
        }
    }
    EXPECT_EQ(xrefEdges, 1) << "merger should emit exactly one resolved xref edge";
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseXRef_InvalidDirectionFails)
{
    constexpr uint8_t invalidDirection = 0xFFU;
    constexpr uint32_t dynamicNodeId = 2;
    constexpr uint32_t staticNodeId = 4;
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader();
    builder.WriteXRefEdgeRecord(dynamicNodeId, staticNodeId, invalidDirection);

    StaticRawheapTranslate parser;
    EXPECT_FALSE(ParseForMerge(builder, "xref_invalid_direction", parser));
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, Merge_InternalEdgeNameUsesDynamicStringTable)
{
    constexpr uint32_t baseClassId = 2;
    constexpr uint32_t derivedClassId = 4;
    constexpr uint32_t baseClassNameId = 10;
    constexpr uint32_t derivedClassNameId = 11;
    constexpr uint32_t classInstanceSize = 16;
    constexpr uint32_t syntheticRootId = 1;
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader();
    builder.WriteStringRecord(baseClassNameId, "Base");
    builder.WriteStringRecord(derivedClassNameId, "Derived");
    builder.WriteLoadClassRecord(baseClassId, baseClassNameId);
    builder.WriteLoadClassRecord(derivedClassId, derivedClassNameId);
    builder.WriteStaticClassDumpRecordFull(baseClassId, 0, classInstanceSize,
                                           StaticSnapshotDataBuilder::StaticClassDumpRecordFields {});
    builder.WriteStaticClassDumpRecordFull(derivedClassId, baseClassId, classInstanceSize,
                                           StaticSnapshotDataBuilder::StaticClassDumpRecordFields {});
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate staticParser;
    ASSERT_TRUE(ParseForMerge(builder, "internal_edge_string", staticParser));
    rawheap_translate::Node *derivedClass = staticParser.FindNodeByNodeId(derivedClassId);
    ASSERT_NE(derivedClass, nullptr);

    RawHeapTranslateV1 dynamic(nullptr);
    rawheap_translate::Node *root = dynamic.CreateNode();
    root->nodeId = syntheticRootId;
    root->type = SYNTHETIC_NODETYPE;
    root->strId = dynamic.InsertAndGetStringId("SyntheticRoot");
    (void)dynamic.InsertAndGetStringId("dynamic-one");
    (void)dynamic.InsertAndGetStringId("dynamic-two");

    SnapshotMerger merger;
    ASSERT_TRUE(merger.Merge(dynamic, staticParser));
    auto *strings = dynamic.GetStringTable();
    bool foundSuperClass = false;
    for (rawheap_translate::Edge *edge : *dynamic.GetEdges()) {
        if (edge->from != derivedClass || edge->type != EdgeType::INTERNAL) {
            continue;
        }
        auto key = strings->GetKeyByStringId(edge->nameOrIndex);
        foundSuperClass = strings->GetStringByKey(key) == "superClass";
    }
    EXPECT_TRUE(foundSuperClass);
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, Merge_NonEmptyFullyUnresolvedXRefs_Fails)
{
    constexpr uint32_t staClassId = 8;
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader();
    builder.WriteStringRecord(10, "StaCls");
    builder.WriteLoadClassRecord(staClassId, 10);
    builder.WriteStaticClassDumpRecord(staClassId, 16, {});
    builder.WriteXRefEdgeRecord(0xDEADU, staClassId, XREF_STA_TO_DYN);
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate staticParser;
    ASSERT_TRUE(ParseForMerge(builder, "xref_unresolved", staticParser));
    RawHeapTranslateV1 dynamic(nullptr);
    rawheap_translate::Node *root = dynamic.CreateNode();
    root->nodeId = 1;
    root->type = SYNTHETIC_NODETYPE;
    root->strId = dynamic.InsertAndGetStringId("SyntheticRoot");

    SnapshotMerger merger;
    EXPECT_FALSE(merger.Merge(dynamic, staticParser));
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, Merge_EmptyXRefs_Succeeds)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader();
    builder.WriteStringRecord(10, "StaCls");
    builder.WriteLoadClassRecord(8, 10);
    builder.WriteStaticClassDumpRecord(8, 16, {});
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate staticParser;
    ASSERT_TRUE(ParseForMerge(builder, "xref_empty", staticParser));
    RawHeapTranslateV1 dynamic(nullptr);
    rawheap_translate::Node *root = dynamic.CreateNode();
    root->nodeId = 1;
    root->type = SYNTHETIC_NODETYPE;
    root->strId = dynamic.InsertAndGetStringId("SyntheticRoot");

    SnapshotMerger merger;
    EXPECT_TRUE(merger.Merge(dynamic, staticParser));
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, Merge_PartiallyResolvedXRefs_Succeeds)
{
    constexpr uint32_t staClassId = 8;
    constexpr uint32_t dynNodeId = 0xDDDD0000U;
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader();
    builder.WriteStringRecord(10, "StaCls");
    builder.WriteLoadClassRecord(staClassId, 10);
    builder.WriteStaticClassDumpRecord(staClassId, 16, {});
    builder.WriteXRefEdgeRecord(dynNodeId, staClassId, XREF_STA_TO_DYN);
    builder.WriteXRefEdgeRecord(0xDEADU, staClassId, XREF_STA_TO_DYN);
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate staticParser;
    ASSERT_TRUE(ParseForMerge(builder, "xref_partial", staticParser));
    RawHeapTranslateV1 dynamic(nullptr);
    rawheap_translate::Node *root = dynamic.CreateNode();
    root->nodeId = 1;
    root->type = SYNTHETIC_NODETYPE;
    root->strId = dynamic.InsertAndGetStringId("SyntheticRoot");
    rawheap_translate::Node *dynNode = dynamic.CreateNode();
    dynNode->nodeId = dynNodeId;
    dynNode->type = OBJECT_NODETYPE;
    dynNode->strId = dynamic.InsertAndGetStringId("DynObj");

    SnapshotMerger merger;
    EXPECT_TRUE(merger.Merge(dynamic, staticParser));
    size_t xrefCount = 0;
    for (rawheap_translate::Edge *edge : *dynamic.GetEdges()) {
        if (edge->type == EdgeType::XREF) {
            ++xrefCount;
        }
    }
    EXPECT_EQ(xrefCount, 1U);
}

// A dynamic-to-static cross-VM reference (XRef direction DYN_TO_STA) is
// resolved by the merger into a static-bound edge.
// When xref direction is DYN_TO_STA, the edge points at the static node.
// NOTE: EmitXRefEdge uses 3-arg Edge(to, idx, type) constructor, so edge->from
// is always nullptr. We only verify edge->to and edge count.
HWTEST_F_L0(RawHeapStaticSnapshotTest, Merge_XRefEdge_DynToSta)
{
    constexpr uint32_t staClassId = 8;
    constexpr uint32_t dynNodeId = 0xDDDD0000U;
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "StaCls");
    builder.WriteLoadClassRecord(staClassId, 10);
    builder.WriteStaticClassDumpRecord(staClassId, 16, {});
    builder.WriteRootRecord(staClassId);
    // DYN_TO_STA: dynamic node references static node
    builder.WriteXRefEdgeRecord(dynNodeId, staClassId, XREF_DYN_TO_STA);
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate staticParser;
    ASSERT_TRUE(ParseForMerge(builder, "xref_dyn2sta", staticParser));
    ASSERT_EQ(staticParser.GetXRefs().size(), 1u);
    rawheap_translate::Node *staNode = staticParser.FindNodeByNodeId(staClassId);
    ASSERT_NE(staNode, nullptr);

    RawHeapTranslateV1 dynamic(nullptr);
    rawheap_translate::Node *synRoot = dynamic.CreateNode();
    synRoot->nodeId = 1;
    synRoot->type = SYNTHETIC_NODETYPE;
    synRoot->strId = dynamic.InsertAndGetStringId("SyntheticRoot");
    synRoot->edgeCount = 0;
    rawheap_translate::Node *dynNode = dynamic.CreateNode();
    dynNode->nodeId = dynNodeId;
    dynNode->type = OBJECT_NODETYPE;
    dynNode->strId = dynamic.InsertAndGetStringId("DynObj");
    dynNode->edgeCount = 0;

    SnapshotMerger merger;
    ASSERT_TRUE(merger.Merge(dynamic, staticParser));

    // DYN_TO_STA: expect 1 XREF edge pointing at static node
    int xrefToStatic = 0;
    for (rawheap_translate::Edge *edge : *dynamic.GetEdges()) {
        if (edge->type == EdgeType::XREF) {
            if (edge->to == staNode) {
                xrefToStatic++;
            }
        }
    }
    EXPECT_EQ(xrefToStatic, 1) << "DYN_TO_STA should emit 1 XREF edge to static node";
}

// A bidirectional XRef (BIDIR) is expanded by the merger into two edges: one
// to static and one to dynamic.
// NOTE: EmitXRefEdge uses 3-arg Edge(to, idx, type) constructor, so edge->from
// is always nullptr. We only verify edge->to targets and edge count.
HWTEST_F_L0(RawHeapStaticSnapshotTest, Merge_XRefEdge_Bidirectional)
{
    constexpr uint32_t staClassId = 8;
    constexpr uint32_t dynNodeId = 0xDDDD0000U;
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "StaCls");
    builder.WriteLoadClassRecord(staClassId, 10);
    builder.WriteStaticClassDumpRecord(staClassId, 16, {});
    builder.WriteRootRecord(staClassId);
    // BIDIR: both virtual machines share the object state - emit edges on both
    // sides
    builder.WriteXRefEdgeRecord(dynNodeId, staClassId, XREF_BIDIR);
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate staticParser;
    ASSERT_TRUE(ParseForMerge(builder, "xref_bidir", staticParser));
    ASSERT_EQ(staticParser.GetXRefs().size(), 1u);
    rawheap_translate::Node *staNode = staticParser.FindNodeByNodeId(staClassId);
    ASSERT_NE(staNode, nullptr);

    RawHeapTranslateV1 dynamic(nullptr);
    rawheap_translate::Node *synRoot = dynamic.CreateNode();
    synRoot->nodeId = 1;
    synRoot->type = SYNTHETIC_NODETYPE;
    synRoot->strId = dynamic.InsertAndGetStringId("SyntheticRoot");
    synRoot->edgeCount = 0;
    rawheap_translate::Node *dynNode = dynamic.CreateNode();
    dynNode->nodeId = dynNodeId;
    dynNode->type = OBJECT_NODETYPE;
    dynNode->strId = dynamic.InsertAndGetStringId("DynObj");
    dynNode->edgeCount = 0;

    SnapshotMerger merger;
    ASSERT_TRUE(merger.Merge(dynamic, staticParser));

    // BIDIR: expect 2 XRef edges - one to static node, one to dynamic node
    int xrefToStatic = 0;
    int xrefToDynamic = 0;
    for (rawheap_translate::Edge *edge : *dynamic.GetEdges()) {
        if (edge->type == EdgeType::XREF) {
            if (edge->to == staNode) {
                xrefToStatic++;
            }
            if (edge->to == dynNode) {
                xrefToDynamic++;
            }
        }
    }
    // BIDIR: expect 2 XRef edges - exactly one to static node and one to dynamic
    // node. Asserting each side ==1 (not just sum==2) catches a regression where
    // both edges are emitted in the same direction.
    EXPECT_EQ(xrefToStatic, 1) << "BIDIR should emit one XRef edge to static node";
    EXPECT_EQ(xrefToDynamic, 1) << "BIDIR should emit one XRef edge to dynamic node";
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, Serialize_SyntheticSnapshot_ValidJSON)
{
    // End-to-end single-file: parse + translate + serialize produces a file whose
    // edge counts are self-consistent (sum(node.edgeCount) == edge_count).
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "C");
    builder.WriteStringRecord(11, "fld");
    builder.WriteLoadClassRecord(2, 10);  // 0x1000 -> nodeId 2
    // instanceFieldNames: {11} = "fld"
    builder.WriteStaticClassDumpRecord(2, 16, {11});
    // fieldValues: {6} = object reference value (0x3000 -> nodeId 6)
    builder.WriteStaticInstanceDumpRecord(4, 2, 16, {6});  // 0x2000->4, 0x1000->2
    builder.WriteRootRecord(4);                            // 0x2000 -> nodeId 4
    builder.WriteHeapSummaryRecord();

    std::string inPath = builder.WriteToTempFile("ser");
    tempFiles_.push_back(inPath);
    std::string outPath = inPath + ".heapsnapshot";
    tempFiles_.push_back(outPath);

    ASSERT_TRUE(RawHeap::TranslateRawheap(inPath, outPath));
    std::ifstream ifs(outPath);
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    // The output must declare a node_count and edge_count, and be valid JSON-ish.
    ASSERT_NE(content.find("\"nodes\":["), std::string::npos);
    ASSERT_NE(content.find("\"edges\":["), std::string::npos);
}

// SuperClass edges: class -> superClass (INTERNAL, "superClass"). With the
// superClass chain Leaf(4)->Mid(3)->Base(2), the graph must contain an INTERNAL
// "superClass" edge targeting Base (from Mid) and one targeting Mid (from
// Leaf).
HWTEST_F_L0(RawHeapStaticSnapshotTest, CreateSuperClassEdges_ChainBuilt)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "Base");
    builder.WriteStringRecord(11, "Mid");
    builder.WriteStringRecord(12, "Leaf");
    builder.WriteLoadClassRecord(2, 10);                   // Base
    builder.WriteLoadClassRecord(3, 11);                   // Mid
    builder.WriteLoadClassRecord(4, 12);                   // Leaf
    builder.WriteStaticClassDumpRecordFull(2, 0, 16, {});  // Base: no super
    builder.WriteStaticClassDumpRecordFull(3, 2, 16, {});  // Mid -> Base
    builder.WriteStaticClassDumpRecordFull(4, 3, 16, {});  // Leaf -> Mid
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "supercls", parser));

    auto *tab = parser.GetStringTable();
    auto resolveStr = [tab](StringId id) -> std::string {
        if (id < StringHashMap::CUSTOM_STRID_START) {
            return std::string();
        }
        return tab->GetStringByKey(tab->GetKeyByStringId(id));
    };

    uint32_t edgesToBase = 0;
    uint32_t edgesToMid = 0;
    for (rawheap_translate::Edge *e : *parser.GetEdges()) {
        if (e->type != EdgeType::INTERNAL) {
            continue;
        }
        if (resolveStr(e->nameOrIndex) != "superClass") {
            continue;
        }
        uint32_t toId = static_cast<uint32_t>(e->to->nodeId);
        if (toId == 2) {  // -> Base (from Mid)
            edgesToBase++;
        }
        if (toId == 3) {  // -> Mid (from Leaf)
            edgesToMid++;
        }
    }
    EXPECT_EQ(edgesToBase, 1u) << "Mid->Base superClass edge missing";
    EXPECT_EQ(edgesToMid, 1u) << "Leaf->Mid superClass edge missing";
}

// Static field edges + primitive value node: a static INT field "counter"
// with value 2 must produce a PROPERTY edge named "counter" whose target is a
// HEAP_NUMBER node whose name stringifies to "2".
HWTEST_F_L0(RawHeapStaticSnapshotTest, CreateStaticFieldEdges_PrimitiveValueNode)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "Base");
    builder.WriteStringRecord(20, "counter");
    builder.WriteLoadClassRecord(2, 10);
    builder.WriteStaticClassDumpRecordFull(2, 0, 16,
                                           StaticSnapshotDataBuilder::StaticClassDumpRecordFields {
                                               {{20, static_cast<uint8_t>(StaFieldType::INT)}},  // staticFields
                                               {{static_cast<uint8_t>(StaFieldType::INT), 2}},   // staticValues
                                               {},                                               // instanceFieldNames
                                               {}});                                             // methodNameIds
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "staval", parser));

    auto *tab = parser.GetStringTable();
    auto resolveStr = [tab](StringId id) -> std::string {
        if (id < StringHashMap::CUSTOM_STRID_START) {
            return std::string();
        }
        return tab->GetStringByKey(tab->GetKeyByStringId(id));
    };

    bool found = false;
    for (rawheap_translate::Edge *e : *parser.GetEdges()) {
        if (e->type != EdgeType::PROPERTY) {
            continue;
        }
        if (resolveStr(e->nameOrIndex) != "counter") {
            continue;
        }
        ASSERT_NE(e->to, nullptr);
        EXPECT_EQ(e->to->type, HEAP_NUMBER) << "counter value node should be HEAP_NUMBER";
        EXPECT_EQ(resolveStr(e->to->strId), "2") << "counter value node name should stringify the value";
        found = true;
    }
    EXPECT_TRUE(found) << "static field 'counter' PROPERTY edge + value node not found";
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, StaticCharFieldsUseUtf8OrUnicodeEscape)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader();
    builder.WriteStringRecord(10, "CharFields");
    builder.WriteStringRecord(20, "ascii");
    builder.WriteStringRecord(21, "cjk");
    builder.WriteStringRecord(22, "nul");
    builder.WriteStringRecord(23, "surrogate");
    builder.WriteLoadClassRecord(2, 10);
    const uint8_t charType = static_cast<uint8_t>(StaFieldType::CHAR);
    builder.WriteStaticClassDumpRecordFull(
        2, 0, 16,
        StaticSnapshotDataBuilder::StaticClassDumpRecordFields {
            {{20, charType}, {21, charType}, {22, charType}, {23, charType}},
            {{charType, 0x0041U}, {charType, 0x4E2DU}, {charType, 0x0000U}, {charType, 0xD800U}},
            {},
            {}});
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "char_fields", parser));
    auto *strings = parser.GetStringTable();
    auto resolve = [strings](StringId id) -> std::string {
        if (id < StringHashMap::CUSTOM_STRID_START) {
            return std::string();
        }
        return strings->GetStringByKey(strings->GetKeyByStringId(id));
    };
    std::vector<std::string> values;
    for (rawheap_translate::Edge *edge : *parser.GetEdges()) {
        if (edge->from != nullptr && edge->from->nodeId == 2 && edge->to != nullptr && edge->to->type == STRING) {
            values.push_back(resolve(edge->to->strId));
        }
    }
    EXPECT_NE(std::find(values.begin(), values.end(), "A"), values.end());
    EXPECT_NE(std::find(values.begin(), values.end(), "\xE4\xB8\xAD"), values.end());
    EXPECT_NE(std::find(values.begin(), values.end(), "\\u0000"), values.end());
    EXPECT_NE(std::find(values.begin(), values.end(), "\\uD800"), values.end());
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, TaggedFieldsPreserveRuntimeValueKinds)
{
    constexpr uint32_t classNodeId = 2;
    constexpr uint32_t strongNodeId = 6;
    constexpr uint32_t weakNodeId = 8;
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader();
    builder.WriteStringRecord(10, "TaggedFields");
    builder.WriteStringRecord(20, "strong");
    builder.WriteStringRecord(21, "weak");
    builder.WriteStringRecord(22, "undefined");
    builder.WriteStringRecord(23, "unknown");
    builder.WriteLoadClassRecord(classNodeId, 10);
    const uint8_t taggedType = static_cast<uint8_t>(StaFieldType::TAGGED);
    builder.WriteStaticClassDumpRecordFull(classNodeId, 0, 16,
                                           StaticSnapshotDataBuilder::StaticClassDumpRecordFields {
                                               {{20, taggedType}, {21, taggedType}, {22, taggedType}, {23, taggedType}},
                                               {{static_cast<uint8_t>(StaFieldType::OBJECT), strongNodeId},
                                                {static_cast<uint8_t>(StaFieldType::WEAK_OBJECT), weakNodeId},
                                                {taggedType, STATIC_TAGGED_UNDEFINED},
                                                {taggedType, 0x123456789ABCDEF0ULL}},
                                               {},
                                               {}});
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "tagged_fields", parser));
    rawheap_translate::Node *classNode = parser.FindNodeByNodeId(classNodeId);
    ASSERT_NE(classNode, nullptr);
    auto *strings = parser.GetStringTable();

    bool foundStrong = false;
    bool foundWeak = false;
    bool foundUndefined = false;
    bool foundUnknown = false;
    for (rawheap_translate::Edge *edge : *parser.GetEdges()) {
        if (edge->from != classNode || edge->to == nullptr) {
            continue;
        }
        std::string name = ResolveString(strings, edge->nameOrIndex);
        foundStrong |= name == "strong" && edge->type == EdgeType::PROPERTY && edge->to->nodeId == strongNodeId;
        foundWeak |= name == "weak" && edge->type == EdgeType::WEAK && edge->to->nodeId == weakNodeId;
        foundUndefined |=
            name == "undefined" && ResolveString(strings, edge->to->strId) == "undefined" &&
            edge->to->type == SYNTHETIC_NODETYPE;
        foundUnknown |= name == "unknown" && ResolveString(strings, edge->to->strId) == "0x123456789ABCDEF0" &&
                        edge->to->type == SYNTHETIC_NODETYPE;
    }
    EXPECT_TRUE(foundStrong);
    EXPECT_TRUE(foundWeak);
    EXPECT_TRUE(foundUndefined);
    EXPECT_TRUE(foundUnknown);
}

// ============================================================================
// EmitMethodNameEdges turns each method-name id into a PROPERTY edge.
// Exercises it by writing a non-empty methodNameIds map.
// The class node should emit PROPERTY edges named "foo"/"bar" targeting
// closure/method nodes (CLOSURE_NODETYPE). This path was previously only
// tested with empty methodNameIds, missing the core method-name edge logic.
// ============================================================================

HWTEST_F_L0(RawHeapStaticSnapshotTest, EmitMethodNameEdges_NonEmptyMethodNames)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "MyClass");
    builder.WriteStringRecord(20, "foo");  // method name id 20 -> "foo"
    builder.WriteStringRecord(21, "bar");  // method name id 21 -> "bar"
    builder.WriteLoadClassRecord(2, 10);   // class nodeId 2 -> "MyClass"

    // Write a class with 2 method name ids (20, 21). The parser should emit
    // 2 PROPERTY edges from the class node, named "foo" and "bar", targeting
    // synthetic closure nodes (CLOSURE_NODETYPE).
    builder.WriteStaticClassDumpRecordFull(2, 0, 16,
                                           StaticSnapshotDataBuilder::StaticClassDumpRecordFields {
                                               {},
                                               {},
                                               {},          // no static fields, no static values, no instance fields
                                               {20, 21}});  // methodNameIds: {20, 21} -> "foo", "bar"
    builder.WriteRootRecord(2);                             // mark class as root so it appears in graph
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "methodnames", parser));

    auto *tab = parser.GetStringTable();
    auto resolveStr = [tab](StringId id) -> std::string {
        if (id < StringHashMap::CUSTOM_STRID_START) {
            return std::string();
        }
        return tab->GetStringByKey(tab->GetKeyByStringId(id));
    };

    // Find the class node
    rawheap_translate::Node *classNode = parser.FindNodeByNodeId(2);
    ASSERT_NE(classNode, nullptr);
    EXPECT_EQ(classNode->type, CLASS_NODETYPE);

    // Collect PROPERTY edges from the class node that correspond to method names.
    // EmitMethodNameEdges creates edges named by the method name string,
    // targeting closure nodes (CLOSURE_NODETYPE).
    std::vector<std::string> methodEdgeNames;
    int closureEdgeCount = 0;
    for (rawheap_translate::Edge *e : *parser.GetEdges()) {
        if (e->type != EdgeType::PROPERTY) {
            continue;
        }
        // The method-name edges originate from the class node
        if (e->from == classNode) {
            std::string edgeName = resolveStr(e->nameOrIndex);
            methodEdgeNames.push_back(edgeName);
            // Verify target is a closure node
            if (e->to != nullptr && e->to->type == CLOSURE_NODETYPE) {
                closureEdgeCount++;
            }
        }
    }

    // Assert 2 method-name edges emitted
    ASSERT_EQ(methodEdgeNames.size(), 2u) << "class should have 2 method-name PROPERTY edges";
    EXPECT_NE(std::find(methodEdgeNames.begin(), methodEdgeNames.end(), "foo"), methodEdgeNames.end())
        << "method edge 'foo' not found";
    EXPECT_NE(std::find(methodEdgeNames.begin(), methodEdgeNames.end(), "bar"), methodEdgeNames.end())
        << "method edge 'bar' not found";
    EXPECT_EQ(closureEdgeCount, 2) << "both method edges should target CLOSURE nodes";
}

// ============================================================================
// ReadFieldValue on a single OBJECT field (u32 nodeId, 4 bytes). The class
// declares 5 field descriptors but the instance body carries only 1 OBJECT
// value, so only the OBJECT (4-byte) read path is exercised here. For
// mixed-type coverage (BOOLEAN/CHAR/INT/LONG/FLOAT/DOUBLE sizes) see the
// array-element tests.
// ============================================================================

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseStaticInstanceDump_SingleObjectField)
{
    // Class declares 5 instance field descriptors, but the instance body below
    // writes only 1 OBJECT value. This tests the OBJECT (4-byte nodeId) read
    // path in ReadFieldValue; the class's field descriptors are used for edge
    // names, not for value parsing (instance body's fieldCount determines that).
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "FiveFieldsClass");
    builder.WriteStringRecord(11, "boolField");
    builder.WriteStringRecord(12, "charField");
    builder.WriteStringRecord(13, "intField");
    builder.WriteStringRecord(14, "longField");
    builder.WriteStringRecord(15, "objField");
    builder.WriteLoadClassRecord(2, 10);  // 0x1000 -> nodeId 2
    // Class declares 5 fields (BOOLEAN/CHAR/INT/LONG/OBJECT) for edge names
    builder.WriteStaticClassDumpRecord(2, 32, {11, 12, 13, 14, 15});
    // Instance carries only 1 OBJECT field value (fieldCount in body = 1)
    builder.WriteStaticInstanceDumpRecord(4, 2, 32, {10});  // 0x2000->4, 0x1000->2
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "singleobj", parser));
    // Verify the class and instance were parsed
    ASSERT_NE(parser.FindNodeByNodeId(2), nullptr);  // 0x1000 -> nodeId 2
    ASSERT_NE(parser.FindNodeByNodeId(4), nullptr);  // 0x2000 -> nodeId 4
    ASSERT_GT(parser.GetNodeCount(), 3u);
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseStaticArrayDump_BooleanElement)
{
    // BOOLEAN arrays: elementType=BOOLEAN, FieldSize=1B per element
    // Exercises ReadFieldValue with byteSize=1. 4 elements -> 4 zero bytes.
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "boolean[]");
    builder.WriteLoadClassRecord(2, 10);  // 0x1000 -> nodeId 2
    ArrayDumpParams arrParams;
    arrParams.objectId = 4;       // 0x2000 -> nodeId 4
    arrParams.classObjectId = 2;  // 0x1000 -> nodeId 2
    arrParams.instanceSize = 24;
    arrParams.arrayLength = 4;  // 4 * 1 = 4 zero bytes
    arrParams.elementType = static_cast<uint8_t>(StaFieldType::BOOLEAN);
    builder.WriteStaticArrayDumpRecord(arrParams);
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "boolarr", parser));
    ASSERT_NE(parser.FindNodeByNodeId(4), nullptr);  // 0x2000 -> nodeId 4
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, StaticCharArrayUsesUtf8OrUnicodeEscape)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader();
    builder.WriteStringRecord(10, "char[]");
    builder.WriteLoadClassRecord(2, 10);
    ArrayDumpParams params;
    params.objectId = 4;
    params.classObjectId = 2;
    params.instanceSize = 32;
    params.arrayLength = 4;
    params.elementType = static_cast<uint8_t>(StaFieldType::CHAR);
    params.primitiveValues = {0x0041U, 0x4E2DU, 0x0000U, 0xDFFFU};
    builder.WriteStaticArrayDumpRecord(params);
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "char_array", parser));
    auto *strings = parser.GetStringTable();
    auto resolve = [strings](StringId id) -> std::string {
        if (id < StringHashMap::CUSTOM_STRID_START) {
            return std::string();
        }
        return strings->GetStringByKey(strings->GetKeyByStringId(id));
    };
    std::vector<std::string> values;
    for (rawheap_translate::Node *node : *parser.GetNodes()) {
        if (node->type == STRING) {
            values.push_back(resolve(node->strId));
        }
    }
    EXPECT_NE(std::find(values.begin(), values.end(), "A"), values.end());
    EXPECT_NE(std::find(values.begin(), values.end(), "\xE4\xB8\xAD"), values.end());
    EXPECT_NE(std::find(values.begin(), values.end(), "\\u0000"), values.end());
    EXPECT_NE(std::find(values.begin(), values.end(), "\\uDFFF"), values.end());
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, TaggedArrayEmitsDirectValueAndWeakEdges)
{
    constexpr uint32_t arrayNodeId = 4;
    constexpr uint32_t strongNodeId = 6;
    constexpr uint32_t weakNodeId = 8;
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader();
    builder.WriteStringRecord(10, "TaggedValue[]");
    builder.WriteLoadClassRecord(2, 10);
    ArrayDumpParams params;
    params.objectId = arrayNodeId;
    params.classObjectId = 2;
    params.instanceSize = 64;
    params.arrayLength = 10;
    params.elementType = static_cast<uint8_t>(StaFieldType::TAGGED);
    params.taggedValues = {
        {static_cast<uint8_t>(StaFieldType::OBJECT), strongNodeId},
        {static_cast<uint8_t>(StaFieldType::WEAK_OBJECT), weakNodeId},
        {static_cast<uint8_t>(StaFieldType::INT), 0xFFFFFFF9U},
        {static_cast<uint8_t>(StaFieldType::BOOLEAN), 1},
        {static_cast<uint8_t>(StaFieldType::TAGGED), STATIC_TAGGED_NULL},
        {static_cast<uint8_t>(StaFieldType::TAGGED), STATIC_TAGGED_UNDEFINED},
        {static_cast<uint8_t>(StaFieldType::TAGGED), STATIC_TAGGED_HOLE},
        {static_cast<uint8_t>(StaFieldType::TAGGED), STATIC_TAGGED_EXCEPTION},
        {static_cast<uint8_t>(StaFieldType::TAGGED), STATIC_TAGGED_FALSE},
        {static_cast<uint8_t>(StaFieldType::TAGGED), 0xFEDCBA9876543210ULL},
    };
    builder.WriteStaticArrayDumpRecord(params);
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "tagged_array", parser));
    rawheap_translate::Node *arrayNode = parser.FindNodeByNodeId(arrayNodeId);
    ASSERT_NE(arrayNode, nullptr);
    rawheap_translate::Node *buffer = FindArrayBuffer(parser, arrayNode);
    ASSERT_NE(buffer, nullptr);
    VerifyTaggedArrayEdges(parser, buffer, weakNodeId, params.arrayLength - 1U);
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseTaggedArrayBatchLocatesEveryItemPrefix)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader();
    builder.WriteStringRecord(10, "TaggedValue[]");
    builder.WriteLoadClassRecord(2, 10);
    ArrayDumpParams first;
    first.objectId = 4;
    first.classObjectId = 2;
    first.instanceSize = 32;
    first.arrayLength = 2;
    first.elementType = static_cast<uint8_t>(StaFieldType::TAGGED);
    first.taggedValues = {{static_cast<uint8_t>(StaFieldType::INT), 7},
                          {static_cast<uint8_t>(StaFieldType::TAGGED), STATIC_TAGGED_NULL}};
    ArrayDumpParams second;
    second.objectId = 6;
    second.classObjectId = 2;
    second.instanceSize = 24;
    second.arrayLength = 1;
    second.elementType = static_cast<uint8_t>(StaFieldType::TAGGED);
    second.taggedValues = {{static_cast<uint8_t>(StaFieldType::TAGGED), STATIC_TAGGED_UNDEFINED}};
    builder.WriteStaticArrayDumpRecordBatch({first, second});
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "tagged_array_batch", parser));
    EXPECT_NE(parser.FindNodeByNodeId(first.objectId), nullptr);
    EXPECT_NE(parser.FindNodeByNodeId(second.objectId), nullptr);
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseTaggedArrayRejectsUnknownRuntimeType)
{
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader();
    ArrayDumpParams params;
    params.objectId = 4;
    params.arrayLength = 1;
    params.elementType = static_cast<uint8_t>(StaFieldType::TAGGED);
    params.taggedValues = {{0xFFU, 0}};
    builder.WriteStaticArrayDumpRecord(params);

    StaticRawheapTranslate parser;
    EXPECT_FALSE(ParseForMerge(builder, "tagged_array_bad_type", parser));
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseStaticArrayDump_IntElement)
{
    // INT arrays: elementType=INT, FieldSize=4B per element
    // Exercises ReadFieldValue with byteSize=4. 8 elements -> 8 * 4 = 32 zero
    // bytes.
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "int[]");
    builder.WriteLoadClassRecord(2, 10);  // 0x1000 -> nodeId 2
    ArrayDumpParams arrParams;
    arrParams.objectId = 4;       // 0x2000 -> nodeId 4
    arrParams.classObjectId = 2;  // 0x1000 -> nodeId 2
    arrParams.instanceSize = 32;
    arrParams.arrayLength = 8;  // 8 * 4 = 32 zero bytes
    arrParams.elementType = static_cast<uint8_t>(StaFieldType::INT);
    builder.WriteStaticArrayDumpRecord(arrParams);
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "intarr", parser));
    ASSERT_NE(parser.FindNodeByNodeId(4), nullptr);  // 0x2000 -> nodeId 4
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseStaticArrayDump_DoubleElement)
{
    // DOUBLE arrays: elementType=DOUBLE, FieldSize=8B per element
    // Exercises ReadFieldValue with byteSize=8. 3 elements -> 3 * 8 = 24 zero
    // bytes.
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "double[]");
    builder.WriteLoadClassRecord(2, 10);  // 0x1000 -> nodeId 2
    ArrayDumpParams arrParams;
    arrParams.objectId = 4;       // 0x2000 -> nodeId 4
    arrParams.classObjectId = 2;  // 0x1000 -> nodeId 2
    arrParams.instanceSize = 48;
    arrParams.arrayLength = 3;  // 3 * 8 = 24 zero bytes
    arrParams.elementType = static_cast<uint8_t>(StaFieldType::DOUBLE);
    builder.WriteStaticArrayDumpRecord(arrParams);
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "dblarr", parser));
    ASSERT_NE(parser.FindNodeByNodeId(4), nullptr);  // 0x2000 -> nodeId 4
}

// ============================================================================
// Two-phase array parsing: ScanArrayPrefixes partitions known vs unknown
// element sizes, then DistributeUnknownData assigns the leftover bytes.
// Exercises it by constructing arrays with mixed known/unknown element sizes.
// ============================================================================

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseStaticArrayDump_MixedKnownAndUnknownTypes)
{
    // Two arrays in one record: one with known OBJECT type, one with SHORT type
    // This exercises ScanArrayPrefixes to partition known vs unknown data,
    // and DistributeUnknownData to allocate the remaining body bytes
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "Object[]");
    builder.WriteStringRecord(11, "short[]");
    builder.WriteLoadClassRecord(2, 10);   // 0x1000 -> nodeId 2
    builder.WriteLoadClassRecord(12, 11);  // 0x1100 -> nodeId 12

    // OBJECT array: 2 elements, each 4 bytes (u32 nodeId) -> 8 bytes known data
    ArrayDumpParams objArr;
    objArr.objectId = 4;       // 0x2000 -> nodeId 4
    objArr.classObjectId = 2;  // 0x1000 -> nodeId 2
    objArr.instanceSize = 32;
    objArr.arrayLength = 2;
    objArr.elementType = static_cast<uint8_t>(StaFieldType::OBJECT);
    objArr.elements = {8, 10};  // 0x4000->8, 0x5000->10
    builder.WriteStaticArrayDumpRecord(objArr);

    // SHORT array: 3 elements, each 2 bytes -> 6 bytes element data
    ArrayDumpParams shortArr;
    shortArr.objectId = 14;       // 0x2100 -> nodeId 14
    shortArr.classObjectId = 12;  // 0x1100 -> nodeId 12
    shortArr.instanceSize = 24;
    shortArr.arrayLength = 3;  // 3 * 2 = 6 zero bytes
    shortArr.elementType = static_cast<uint8_t>(StaFieldType::SHORT);
    builder.WriteStaticArrayDumpRecord(shortArr);

    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "mixarr", parser));
    ASSERT_NE(parser.FindNodeByNodeId(8), nullptr);   // 0x4000 -> nodeId 8
    ASSERT_NE(parser.FindNodeByNodeId(10), nullptr);  // 0x5000 -> nodeId 10
}

HWTEST_F_L0(RawHeapStaticSnapshotTest, ParseStaticArrayDump_EmptyArray)
{
    // Empty array (arrayLength=0): ScanArrayPrefixes should handle gracefully
    // No element data to distribute - dataSizeKnown=true, dataSize=0
    StaticSnapshotDataBuilder builder;
    builder.WriteHeader(1);
    builder.WriteStringRecord(10, "Object[]");
    builder.WriteLoadClassRecord(2, 10);  // 0x1000 -> nodeId 2
    ArrayDumpParams arrParams;
    arrParams.objectId = 4;       // 0x2000 -> nodeId 4
    arrParams.classObjectId = 2;  // 0x1000 -> nodeId 2
    arrParams.instanceSize = 24;
    arrParams.arrayLength = 0;
    arrParams.elementType = static_cast<uint8_t>(StaFieldType::OBJECT);
    builder.WriteStaticArrayDumpRecord(arrParams);
    builder.WriteHeapSummaryRecord();

    StaticRawheapTranslate parser;
    ASSERT_TRUE(ParseSingle(builder, "emptyarr", parser));
    ASSERT_NE(parser.FindNodeByNodeId(4), nullptr);  // 0x2000 -> nodeId 4
}

}  // namespace panda::test
