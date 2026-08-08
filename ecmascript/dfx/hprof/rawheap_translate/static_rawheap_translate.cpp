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

#include "static_rawheap_translate.h"

#include "securec.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace rawheap_translate {

namespace {
constexpr uint32_t MAX_RECORD_BODY_SIZE = 256U * 1024U * 1024U;
constexpr uint32_t MAX_RECORD_ITEM_COUNT = 64U * 1024U;
constexpr uint32_t MAX_STRING_DATA_SIZE = 64U * 1024U * 1024U;
constexpr uint32_t SKIP_BUFFER_SIZE = 4096U;
constexpr uint16_t UTF16_HIGH_SURROGATE_START = 0xD800U;
constexpr uint16_t UTF16_LOW_SURROGATE_END = 0xDFFFU;
constexpr uint16_t UTF8_ONE_BYTE_LIMIT = 0x80U;
constexpr uint16_t UTF8_TWO_BYTE_LIMIT = 0x800U;
constexpr size_t UNICODE_ESCAPE_PREFIX_SIZE = 2;
constexpr size_t UTF16_HEX_DIGIT_COUNT = 4;
constexpr uint32_t BITS_PER_HEX_DIGIT = 4;
constexpr uint16_t HEX_DIGIT_MASK = 0x0FU;
constexpr uint32_t UTF8_CONTINUATION_SHIFT = 6;
constexpr uint32_t UTF8_THREE_BYTE_SHIFT = 12;
constexpr uint16_t UTF8_CONTINUATION_MASK = 0x3FU;
constexpr uint16_t UTF8_TWO_BYTE_PREFIX = 0xC0U;
constexpr uint16_t UTF8_THREE_BYTE_PREFIX = 0xE0U;
constexpr uint16_t UTF8_CONTINUATION_PREFIX = 0x80U;

constexpr size_t IDENTIFIER_SIZE_OFFSET = STATIC_VERSION_SIZE;
constexpr size_t TIMESTAMP_OFFSET = IDENTIFIER_SIZE_OFFSET + sizeof(uint32_t);
constexpr size_t LANGUAGE_OFFSET = TIMESTAMP_OFFSET + sizeof(uint64_t);
constexpr size_t HEADER_SIZE_OFFSET = LANGUAGE_OFFSET + sizeof(uint8_t);
constexpr size_t RECORD_COUNT_OFFSET = HEADER_SIZE_OFFSET + sizeof(uint32_t);
constexpr size_t FEATURE_FLAGS_OFFSET = RECORD_COUNT_OFFSET + sizeof(uint32_t);

struct StaticSnapshotHeader {
    std::array<char, STATIC_VERSION_SIZE> version {};
    uint32_t identifierSize = 0;
    uint64_t timestamp = 0;
    uint8_t language = 0;
    uint32_t headerSize = 0;
    uint32_t recordCount = 0;
    uint32_t featureFlags = 0;
};

bool ReadStaticSnapshotHeader(FileReader &file, StaticSnapshotHeader &header)
{
    std::array<char, STATIC_HEADER_SIZE> bytes {};
    if (!file.Seek(0) || !file.Read(bytes.data(), bytes.size())) {
        return false;
    }
    std::copy_n(bytes.data(), STATIC_VERSION_SIZE, header.version.data());
    header.identifierSize = ByteToU32(bytes.data() + IDENTIFIER_SIZE_OFFSET);
    header.timestamp = ByteToU64(bytes.data() + TIMESTAMP_OFFSET);
    header.language = static_cast<uint8_t>(bytes[LANGUAGE_OFFSET]);
    header.headerSize = ByteToU32(bytes.data() + HEADER_SIZE_OFFSET);
    header.recordCount = ByteToU32(bytes.data() + RECORD_COUNT_OFFSET);
    header.featureFlags = ByteToU32(bytes.data() + FEATURE_FLAGS_OFFSET);
    return true;
}

bool ValidateStaticSnapshotHeader(const StaticSnapshotHeader &header, bool logError)
{
    auto versionEnd = std::find(header.version.begin(), header.version.end(), '\0');
    Version version;
    bool versionSupported = versionEnd != header.version.end() &&
                            version.Parse(std::string(header.version.begin(), versionEnd)) &&
                            version.GetMajor() == STATIC_SNAPSHOT_MAJOR_VERSION;
    if (!versionSupported) {
        if (logError) {
            LOG_ERROR_ << "unsupported static snapshot version";
        }
        return false;
    }
    if (header.identifierSize != STATIC_IDENTIFIER_SIZE) {
        if (logError) {
            LOG_ERROR_ << "identifierSize mismatch: expected " << STATIC_IDENTIFIER_SIZE << " got " <<
                header.identifierSize;
        }
        return false;
    }
    if (header.language != STATIC_LANGUAGE_STATIC && header.language != STATIC_LANGUAGE_HYBRID) {
        if (logError) {
            LOG_ERROR_ << "unsupported static snapshot language: " << static_cast<uint32_t>(header.language);
        }
        return false;
    }
    if (header.headerSize != STATIC_HEADER_SIZE) {
        if (logError) {
            LOG_ERROR_ << "headerSize mismatch: expected " << STATIC_HEADER_SIZE << " got " << header.headerSize;
        }
        return false;
    }
    if ((header.featureFlags & ~STATIC_SUPPORTED_FEATURE_FLAGS) != 0) {
        if (logError) {
            LOG_ERROR_ << "unsupported static snapshot feature flags: 0x" << std::hex << header.featureFlags <<
                std::dec;
        }
        return false;
    }
    return true;
}

std::string EncodeEtsChar(uint16_t value)
{
    if (value == 0 || (value >= UTF16_HIGH_SURROGATE_START && value <= UTF16_LOW_SURROGATE_END)) {
        constexpr std::string_view HEX_DIGITS = "0123456789ABCDEF";
        std::string escaped = "\\u0000";
        for (size_t i = 0; i < UTF16_HEX_DIGIT_COUNT; ++i) {
            size_t shift = (UTF16_HEX_DIGIT_COUNT - i - 1) * BITS_PER_HEX_DIGIT;
            escaped[UNICODE_ESCAPE_PREFIX_SIZE + i] = HEX_DIGITS[(value >> shift) & HEX_DIGIT_MASK];
        }
        return escaped;
    }
    if (value < UTF8_ONE_BYTE_LIMIT) {
        return std::string(1, static_cast<char>(value));
    }
    if (value < UTF8_TWO_BYTE_LIMIT) {
        return {static_cast<char>(UTF8_TWO_BYTE_PREFIX | (value >> UTF8_CONTINUATION_SHIFT)),
                static_cast<char>(UTF8_CONTINUATION_PREFIX | (value & UTF8_CONTINUATION_MASK))};
    }
    return {static_cast<char>(UTF8_THREE_BYTE_PREFIX | (value >> UTF8_THREE_BYTE_SHIFT)),
            static_cast<char>(UTF8_CONTINUATION_PREFIX | ((value >> UTF8_CONTINUATION_SHIFT) & UTF8_CONTINUATION_MASK)),
            static_cast<char>(UTF8_CONTINUATION_PREFIX | (value & UTF8_CONTINUATION_MASK))};
}

uint64_t ReadLittleEndianValue(const char *data, uint8_t byteSize)
{
    uint64_t value = 0;
    for (uint8_t i = 0; i < byteSize; ++i) {
        value |= static_cast<uint64_t>(static_cast<uint8_t>(data[i])) << (BITS_PER_BYTE * i);
    }
    return value;
}

bool IsSupportedFieldValueType(uint8_t type)
{
    switch (static_cast<StaFieldType>(type)) {
        case StaFieldType::BOOLEAN:
        case StaFieldType::CHAR:
        case StaFieldType::FLOAT:
        case StaFieldType::DOUBLE:
        case StaFieldType::BYTE:
        case StaFieldType::SHORT:
        case StaFieldType::INT:
        case StaFieldType::LONG:
        case StaFieldType::OBJECT:
        case StaFieldType::ARRAY:
        case StaFieldType::TAGGED:
        case StaFieldType::WEAK_OBJECT:
            return true;
        case StaFieldType::UNKNOWN:
            return false;
    }
    return false;
}
}  // namespace

StaticRawheapTranslate::~StaticRawheapTranslate()
{
    // Nodes/edges/string table are owned and freed by the RawHeap base
    // destructor.
}

// ---- Parse (Phase 1: Collect) ----

bool StaticRawheapTranslate::Parse(FileReader &file, uint32_t rawheapFileSize)
{
    file_ = &file;
    parseOk_ = true;
    readingRecord_ = false;
    recordRemaining_ = 0;

    if (!ParseHeader()) {
        LOG_ERROR_ << "failed to parse static snapshot header";
        return false;
    }

    uint64_t fileSize = rawheapFileSize == 0 ? file.GetFileSize() : rawheapFileSize;
    uint64_t offset = header_.headerSize;
    if (offset > fileSize) {
        LOG_ERROR_ << "static snapshot header exceeds file size";
        return false;
    }

    // Each record is parsed inside its declared body boundary. A collector may
    // neither consume bytes from the next record nor leave trailing body bytes.
    while (offset < fileSize) {
        if (!ParseRecord(offset, fileSize)) {
            return false;
        }
    }

    // Two-file (merge) mode: build object nodes + edges only, no synthetic root.
    // Single-file mode: also build the synthetic-root framework (nodes[0..1]).
    BuildGraph(buildRootFramework_);

    file_ = nullptr;  // done with file reading
    return true;
}

bool StaticRawheapTranslate::ParseRecord(uint64_t &offset, uint64_t fileSize)
{
    if (fileSize - offset < STATIC_RECORD_HDR_SIZE) {
        LOG_ERROR_ << "truncated static record header at offset " << offset;
        return false;
    }
    char hdr[STATIC_RECORD_HDR_SIZE];
    if (!file_->Read(hdr, STATIC_RECORD_HDR_SIZE)) {
        LOG_ERROR_ << "failed to read static record header at offset " << offset;
        return false;
    }
    offset += STATIC_RECORD_HDR_SIZE;
    const uint8_t tag = static_cast<uint8_t>(hdr[STATIC_RECORD_HDR_TAG_OFF]);
    const uint32_t length = ByteToU32(hdr + STATIC_RECORD_HDR_LENGTH_OFF);
    const uint32_t count = ByteToU32(hdr + STATIC_RECORD_HDR_COUNT_OFF);
    if (length > MAX_RECORD_BODY_SIZE) {
        LOG_ERROR_ << "record body length " << length << " exceeds safety limit " << MAX_RECORD_BODY_SIZE;
        return false;
    }
    if (length > fileSize - offset) {
        LOG_ERROR_ << "record body length " << length << " exceeds remaining file bytes " << fileSize - offset;
        return false;
    }
    if (count > length) {
        LOG_ERROR_ << "record item count " << count << " exceeds body length " << length;
        return false;
    }
    if (count > MAX_RECORD_ITEM_COUNT) {
        LOG_ERROR_ << "record item count " << count << " exceeds safety limit " << MAX_RECORD_ITEM_COUNT;
        return false;
    }

    readingRecord_ = true;
    recordRemaining_ = length;
    if (!DispatchRecord(tag, length, count)) {
        LOG_ERROR_ << "failed to parse record tag=" << static_cast<int>(tag);
        return false;
    }
    if (!parseOk_) {
        LOG_ERROR_ << "read error during record collection";
        return false;
    }
    if (recordRemaining_ != 0) {
        LOG_ERROR_ << "record tag=" << static_cast<int>(tag) << " left " << recordRemaining_ <<
            " unconsumed body bytes";
        return false;
    }
    readingRecord_ = false;
    offset += length;
    return true;
}

bool StaticRawheapTranslate::Translate()
{
    // Graph (including single-file synthetic root) is built during Parse().
    // Add the trailing primitive nodes the serializer expects.
    AddPrimitiveNodes();
    return true;
}

Node *StaticRawheapTranslate::FindNodeByNodeId(uint32_t nodeId) const
{
    auto it = nodeIdToNode_.find(nodeId);
    return it == nodeIdToNode_.end() ? nullptr : it->second;
}

bool StaticRawheapTranslate::ParseHeader()
{
    StaticSnapshotHeader header;
    if (!ReadStaticSnapshotHeader(*file_, header)) {
        LOG_ERROR_ << "failed to read static snapshot header";
        return false;
    }
    if (!ValidateStaticSnapshotHeader(header, true)) {
        return false;
    }

    header_.identifierSize = header.identifierSize;
    header_.timestamp = header.timestamp;
    header_.language = header.language;
    header_.headerSize = header.headerSize;
    header_.recordCount = header.recordCount;
    header_.featureFlags = header.featureFlags;
    if (!file_->Seek(header_.headerSize)) {
        LOG_ERROR_ << "failed to seek past header";
        return false;
    }
    LOG_INFO_ << "static header: lang=" << static_cast<int>(header_.language) << " headerSize=" << header_.headerSize;
    return true;
}

bool StaticRawheapTranslate::DispatchRecord(uint8_t tag, uint32_t length, uint32_t count)
{
    switch (tag) {
        case TAG_STRING_IN_UTF8:
            return CollectStringItems(length, count);
        case TAG_LOAD_CLASS:
            return CollectLoadClassItems(length, count);
        case TAG_STATIC_CLASS_DUMP:
            return CollectStaticClassDumpItems(length, count);
        case TAG_ROOT_RECORD:
            return CollectRootItems(length, count);
        case TAG_STATIC_INSTANCE_DUMP:
            return CollectInstanceItems(length, count);
        case TAG_STATIC_ARRAY_DUMP:
            return CollectArrayItems(length, count);
        case TAG_STATIC_STRING_DUMP:
            return CollectStaticStringDumpItems(length, count);
        case TAG_XREF_EDGE:
            return CollectXRefItems(length, count);
        case TAG_HEAP_SUMMARY:
            return CollectHeapSummary(length);
        default:
            LOG_INFO_ << "unknown static record tag=" << static_cast<int>(tag) << " length=" << length <<
                " count=" << count << ", skipping";
            return SkipBody(length);
    }
}

bool StaticRawheapTranslate::SkipBody(uint32_t length)
{
    std::array<char, SKIP_BUFFER_SIZE> buffer {};
    uint32_t remaining = length;
    while (remaining > 0) {
        uint32_t chunk = std::min<uint32_t>(remaining, buffer.size());
        if (!ReadBytes(buffer.data(), chunk)) {
            return false;
        }
        remaining -= chunk;
    }
    return true;
}

bool StaticRawheapTranslate::CollectStringItems(uint32_t length, uint32_t count)
{
    // Body contains count string items, each:
    // [stringId:u4][strLen:u4][utf8Data:strLen bytes]
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t stringId = ReadU32();
        uint32_t strLen = ReadU32();
        if (!parseOk_) {
            return false;
        }
        if (strLen > MAX_STRING_DATA_SIZE) {
            LOG_ERROR_ << "string length " << strLen << " exceeds safety limit " << MAX_STRING_DATA_SIZE;
            return false;
        }
        if (strLen > recordRemaining_) {
            LOG_ERROR_ << "string length " << strLen << " exceeds remaining record bytes " << recordRemaining_;
            return false;
        }
        std::string str(strLen, '\0');
        if (strLen > 0 && !ReadBytes(str.data(), strLen)) {
            return false;
        }
        stringTable_[stringId] = str;
    }
    return true;
}

bool StaticRawheapTranslate::CollectLoadClassItems(uint32_t length, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) {
        (void)ReadU32();                   // classSerialNumber
        uint32_t classNodeId = ReadU32();  // classObjectId
        (void)ReadU32();                   // stackTraceSerial
        uint32_t classNameId = ReadU32();  // classNameId
        (void)ReadU8();                    // language
        (void)ReadU32();                   // classFlags
        if (!parseOk_) {
            return false;
        }
        auto &info = classMap_[classNodeId];
        info.classNameId = classNameId;
    }
    return true;
}

bool StaticRawheapTranslate::CollectStaticClassDumpItems(uint32_t length, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t classNodeId = ReadU32();       // classObjectId
        (void)ReadU32();                        // stackTraceSerial
        uint32_t superClassNodeId = ReadU32();  // superClassObjectId
        (void)ReadU32();                        // classLoaderObjectId
        uint32_t instanceSize = ReadU32();      // instanceSize
        if (!parseOk_) {
            return false;
        }
        auto &info = classMap_[classNodeId];
        info.instanceSize = instanceSize;
        info.superClassNodeId = superClassNodeId;
        ReadFieldDescriptors(info.staticFields);
        ReadFieldDescriptors(info.instanceFields);
        ReadStaticValues(info.staticValues);
        ReadMethodNames(info.methodNameIds);
        if (!parseOk_) {
            return false;
        }
    }
    return true;
}

void StaticRawheapTranslate::ReadFieldDescriptor(FieldDef &fd)
{
    fd.nameId = ReadU32();
    fd.type = ReadU8();
    fd.offset = ReadU32();
    fd.flags = ReadU16();
}

void StaticRawheapTranslate::ReadFieldDescriptors(std::vector<FieldDef> &out)
{
    uint16_t cnt = ReadU16();
    out.reserve(cnt);
    for (uint16_t j = 0; j < cnt; ++j) {
        FieldDef fd;
        ReadFieldDescriptor(fd);
        out.push_back(fd);
    }
}

void StaticRawheapTranslate::ReadStaticValues(std::vector<FieldValue> &out)
{
    // Static field values (parallel to staticFields, same order). Each entry
    // is [type:u1][value:variable], identical to INSTANCE_DUMP field values.
    uint16_t cnt = ReadU16();
    out.reserve(cnt);
    for (uint16_t j = 0; j < cnt; ++j) {
        FieldValue fv;
        fv.type = ReadU8();
        if (!IsSupportedFieldValueType(fv.type)) {
            LOG_ERROR_ << "unsupported static field value type " << static_cast<uint32_t>(fv.type);
            parseOk_ = false;
            return;
        }
        fv.value = ReadFieldValue(FieldSize(fv.type));
        out.push_back(fv);
    }
}

void StaticRawheapTranslate::ReadMethodNames(std::vector<uint32_t> &out)
{
    // Declared method name ids (dump string-pool indices).
    uint16_t cnt = ReadU16();
    out.reserve(cnt);
    for (uint16_t j = 0; j < cnt; ++j) {
        out.push_back(ReadU32());
    }
}

bool StaticRawheapTranslate::CollectRootItems(uint32_t length, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) {
        (void)ReadU8();  // rootType
        uint32_t objectNodeId = ReadU32();
        if (!parseOk_) {
            return false;
        }
        roots_.push_back(objectNodeId);
    }
    return true;
}

bool StaticRawheapTranslate::CollectInstanceItems(uint32_t length, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) {
        InstanceRecord rec;
        rec.objectNodeId = ReadU32();
        rec.classNodeId = ReadU32();
        (void)ReadU32();  // stackTraceSerial
        rec.instanceSize = ReadU32();
        uint16_t fieldCount = ReadU16();
        if (!parseOk_) {
            return false;
        }

        rec.values.reserve(fieldCount);
        for (uint16_t j = 0; j < fieldCount; ++j) {
            uint8_t fieldType = ReadU8();
            if (!IsSupportedFieldValueType(fieldType)) {
                LOG_ERROR_ << "unsupported instance field value type " << static_cast<uint32_t>(fieldType);
                return false;
            }
            uint8_t sz = FieldSize(fieldType);
            uint64_t val = ReadFieldValue(sz);
            rec.values.push_back({fieldType, val});
        }
        if (!parseOk_) {
            return false;
        }
        instances_.push_back(std::move(rec));
    }
    return true;
}

bool StaticRawheapTranslate::CollectArrayItems(uint32_t length, uint32_t count)
{
    if (length == 0 || count == 0) {
        return true;
    }
    std::vector<char> bodyBuf(length);
    if (!ReadBytes(bodyBuf.data(), length)) {
        LOG_ERROR_ << "CollectArrayItems: failed to read body";
        return false;
    }
    char *body = bodyBuf.data();

    std::vector<ArrayItemLayout> layouts(count);
    size_t totalKnownData = 0;
    size_t totalUnknownLength = 0;

    if (!ScanArrayPrefixes(body, length, layouts, totalKnownData, totalUnknownLength)) {
        return false;
    }
    if (!DistributeUnknownData(layouts, count, totalKnownData, length, totalUnknownLength)) {
        return false;
    }
    if (!BuildArrayRecords(body, count, length, layouts)) {
        return false;
    }
    return true;
}

bool StaticRawheapTranslate::ScanArrayPrefixes(char *body, uint32_t length,
                                               std::vector<ArrayItemLayout> &layouts, size_t &totalKnownData,
                                               size_t &totalUnknownLength)
{
    size_t pos = 0;
    for (size_t i = 0; i < layouts.size(); ++i) {
        if (pos + STATIC_ARRAY_PREFIX_BODY_SIZE > length) {
            LOG_ERROR_ << "CollectArrayItems: insufficient body for prefix at item " << i;
            return false;
        }
        ArrayItemLayout &lay = layouts[i];
        lay.prefixOff = pos;
        lay.arrayLength = ByteToU32(body + pos + STATIC_ARRAY_LENGTH_OFFSET);
        lay.elementType = static_cast<uint8_t>(body[pos + STATIC_ARRAY_ELEM_TYPE_OFFSET]);
        pos += STATIC_ARRAY_PREFIX_BODY_SIZE;

        if (!ScanArrayDataSize(body, length, pos, lay, totalUnknownLength)) {
            return false;
        }
        if (lay.dataSizeKnown) {
            if (lay.dataSize > length - pos) {
                LOG_ERROR_ << "CollectArrayItems: item " << i << " data exceeds record body";
                return false;
            }
            totalKnownData += lay.dataSize;
            pos += lay.dataSize;
        }
    }
    return true;
}

bool StaticRawheapTranslate::ScanArrayDataSize(const char *body, uint32_t length, size_t dataOffset,
                                               ArrayItemLayout &layout, size_t &totalUnknownLength)
{
    bool isReference = layout.elementType == static_cast<uint8_t>(StaFieldType::OBJECT) ||
                       layout.elementType == static_cast<uint8_t>(StaFieldType::ARRAY) ||
                       layout.elementType == static_cast<uint8_t>(StaFieldType::WEAK_OBJECT);
    if (layout.elementType == static_cast<uint8_t>(StaFieldType::TAGGED)) {
        size_t scanOffset = dataOffset;
        for (uint32_t index = 0; index < layout.arrayLength; ++index) {
            if (scanOffset >= length) {
                LOG_ERROR_ << "CollectArrayItems: tagged element type exceeds record body";
                return false;
            }
            uint8_t valueType = static_cast<uint8_t>(body[scanOffset++]);
            if (!IsSupportedFieldValueType(valueType)) {
                LOG_ERROR_ << "CollectArrayItems: unsupported tagged element type " <<
                    static_cast<uint32_t>(valueType);
                return false;
            }
            uint8_t valueSize = FieldSize(valueType);
            if (valueSize > length - scanOffset) {
                LOG_ERROR_ << "CollectArrayItems: tagged element payload exceeds record body";
                return false;
            }
            scanOffset += valueSize;
        }
        layout.dataSize = scanOffset - dataOffset;
        layout.dataSizeKnown = true;
        return true;
    }

    uint8_t elementSize = FieldSize(layout.elementType);
    if (isReference) {
        layout.dataSize = static_cast<size_t>(layout.arrayLength) * sizeof(uint32_t);
        layout.dataSizeKnown = true;
    } else if (elementSize > 0 && layout.arrayLength > 0) {
        layout.dataSize = static_cast<size_t>(layout.arrayLength) * elementSize;
        layout.dataSizeKnown = true;
    } else if (layout.arrayLength > 0) {
        totalUnknownLength += layout.arrayLength;
    } else {
        layout.dataSizeKnown = true;
    }
    return true;
}

bool StaticRawheapTranslate::DistributeUnknownData(std::vector<ArrayItemLayout> &layouts, uint32_t count,
                                                   size_t totalKnownData, uint32_t length, size_t totalUnknownLength)
{
    size_t prefixBytes = static_cast<size_t>(STATIC_ARRAY_PREFIX_BODY_SIZE) * count;
    if (prefixBytes > length || totalKnownData > length - prefixBytes) {
        LOG_ERROR_ << "CollectArrayItems: array layout exceeds record body";
        return false;
    }
    size_t totalUnknownData = length - prefixBytes - totalKnownData;
    for (uint32_t i = 0; i < count; ++i) {
        if (!layouts[i].dataSizeKnown && layouts[i].arrayLength > 0 && totalUnknownLength > 0) {
            layouts[i].dataSize = totalUnknownData * layouts[i].arrayLength / totalUnknownLength;
            layouts[i].dataSizeKnown = true;
        }
    }
    return true;
}

bool StaticRawheapTranslate::BuildArrayRecords(char *body, uint32_t count, uint32_t bodyLength,
                                               const std::vector<ArrayItemLayout> &layouts)
{
    for (uint32_t i = 0; i < count; ++i) {
        ArrayRecord rec;
        if (!BuildArrayRecord(body, bodyLength, i, layouts[i], rec)) {
            return false;
        }
        arrays_.push_back(std::move(rec));
    }
    return true;
}

bool StaticRawheapTranslate::BuildArrayRecord(char *body, uint32_t bodyLength, uint32_t itemIndex,
                                              const ArrayItemLayout &layout, ArrayRecord &record)
{
    size_t prefixOffset = layout.prefixOff;
    if (prefixOffset > bodyLength || bodyLength - prefixOffset < STATIC_ARRAY_PREFIX_BODY_SIZE) {
        LOG_ERROR_ << "CollectArrayItems: invalid prefix offset at item " << itemIndex;
        return false;
    }
    record.objectNodeId = ByteToU32(body + prefixOffset);
    record.classNodeId = ByteToU32(body + prefixOffset + STATIC_ARRAY_CLASS_OFFSET);
    record.instanceSize = ByteToU32(body + prefixOffset + STATIC_ARRAY_INSTSIZE_OFFSET);
    record.length = layout.arrayLength;
    record.elementType = layout.elementType;

    size_t dataOffset = prefixOffset + STATIC_ARRAY_PREFIX_BODY_SIZE;
    if (layout.dataSize > bodyLength - dataOffset) {
        LOG_ERROR_ << "CollectArrayItems: invalid data range at item " << itemIndex;
        return false;
    }
    bool isReference = record.elementType == static_cast<uint8_t>(StaFieldType::OBJECT) ||
                       record.elementType == static_cast<uint8_t>(StaFieldType::ARRAY) ||
                       record.elementType == static_cast<uint8_t>(StaFieldType::WEAK_OBJECT);
    if (record.elementType == static_cast<uint8_t>(StaFieldType::TAGGED) && record.length > 0) {
        return ReadTaggedArrayValues(body, dataOffset, layout.dataSize, record);
    }
    if (isReference && record.length > 0) {
        record.elements.reserve(record.length);
        for (uint32_t index = 0; index < record.length; ++index) {
            record.elements.push_back(ByteToU32(body + dataOffset + index * sizeof(uint32_t)));
        }
    } else if (record.length > 0 && layout.dataSizeKnown && layout.dataSize > 0 &&
               FieldSize(record.elementType) > 0) {
        const char *data = body + dataOffset;
        record.primData.assign(data, data + layout.dataSize);
    }
    return true;
}

bool StaticRawheapTranslate::ReadTaggedArrayValues(const char *body, size_t dataOffset, size_t dataSize,
                                                   ArrayRecord &record)
{
    size_t valueOffset = dataOffset;
    size_t dataEnd = dataOffset + dataSize;
    record.taggedValues.reserve(record.length);
    for (uint32_t index = 0; index < record.length; ++index) {
        if (valueOffset >= dataEnd) {
            LOG_ERROR_ << "CollectArrayItems: tagged element type exceeds item data";
            return false;
        }
        uint8_t valueType = static_cast<uint8_t>(body[valueOffset++]);
        if (!IsSupportedFieldValueType(valueType)) {
            LOG_ERROR_ << "CollectArrayItems: unsupported tagged element type " << static_cast<uint32_t>(valueType);
            return false;
        }
        uint8_t valueSize = FieldSize(valueType);
        if (valueSize > dataEnd - valueOffset) {
            LOG_ERROR_ << "CollectArrayItems: tagged element payload exceeds item data";
            return false;
        }
        uint64_t value = ReadLittleEndianValue(body + valueOffset, valueSize);
        valueOffset += valueSize;
        record.taggedValues.push_back({valueType, value});
    }
    if (valueOffset != dataEnd) {
        LOG_ERROR_ << "CollectArrayItems: tagged array has trailing element data";
        return false;
    }
    return true;
}

bool StaticRawheapTranslate::CollectStaticStringDumpItems(uint32_t length, uint32_t count)
{
    // Body per item:
    // [objId:u4][classObjId:u4][instSize:u4][valueLen:u4][valueBytes:valueLen]
    for (uint32_t i = 0; i < count; ++i) {
        StringInstanceRecord rec;
        rec.objectNodeId = ReadU32();
        rec.classNodeId = ReadU32();
        rec.instanceSize = ReadU32();
        uint32_t valueLen = ReadU32();
        if (!parseOk_) {
            return false;
        }
        if (valueLen > MAX_STRING_DATA_SIZE) {
            LOG_ERROR_ << "static string length " << valueLen << " exceeds safety limit " << MAX_STRING_DATA_SIZE;
            return false;
        }
        if (valueLen > recordRemaining_) {
            LOG_ERROR_ << "static string length " << valueLen << " exceeds remaining record bytes " << recordRemaining_;
            return false;
        }
        if (valueLen > 0) {
            std::string s(valueLen, '\0');
            if (!ReadBytes(s.data(), valueLen)) {
                return false;
            }
            rec.content = std::move(s);
        }
        stringInstances_.push_back(std::move(rec));
    }
    return true;
}

bool StaticRawheapTranslate::CollectXRefItems(uint32_t length, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) {
        XRefRecord xref;
        xref.dynNodeId = ReadU32();  // dynamic-side nodeId (4 bytes)
        xref.staNodeId = ReadU32();
        xref.direction = ReadU8();
        if (!parseOk_) {
            return false;
        }
        if (xref.direction != XREF_DYN_TO_STA && xref.direction != XREF_STA_TO_DYN && xref.direction != XREF_BIDIR) {
            LOG_ERROR_ << "unsupported XRef direction " << static_cast<uint32_t>(xref.direction);
            return false;
        }
        xrefs_.push_back(xref);
    }
    return true;
}

bool StaticRawheapTranslate::CollectHeapSummary(uint32_t length)
{
    // Heap summary is informational only; not used for graph construction.
    return SkipBody(length);
}

// ---- Build (Phase 2: graph construction from collected records) ----

void StaticRawheapTranslate::BuildGraph(bool withRootFramework)
{
    Node *syntheticRoot = nullptr;
    Node *staticRoot = nullptr;
    if (withRootFramework) {
        syntheticRoot = CreateNode();
        staticRoot = CreateNode();
    }

    CreateClassNodes();
    CreateInstanceNodes();
    CreateArrayNodes();
    CreateStringNodes();

    // Mark roots. Preserve a node's semantic type (class/object/array) - root
    // status is conveyed by the StaticRoot->node ELEMENT edges, not by type.
    // Overwriting class nodes to ROOT would erase their "class" identity.
    for (uint32_t nodeId : roots_) {
        Node *node = GetOrCreateNode(nodeId);
        if (node->type != CLASS_NODETYPE && node->type != OBJECT_NODETYPE && node->type != ARRAY_NODETYPE &&
            node->type != STRING) {
            node->type = ROOT;
        }
    }

    if (withRootFramework) {
        CreateRootEdges(syntheticRoot, staticRoot);
    }
    CreateClassEdges();     // class -> superClass + static fields (contiguous)
    CreateInstanceEdges();  // instance -> fields + instance -> class (PROPERTY)
    CreateArrayEdges();
    CreateStringEdges();  // string node -> hclass (std.core.String)
    // Edges were inserted in phase order; re-sort by source node index so the
    // flat vector satisfies the .heapsnapshot grouping contract (node i owns the
    // next edgeCount[i] edges). Every edge above carries edge->from for this.
    SortEdgesByFrom();
}

std::vector<uint32_t> StaticRawheapTranslate::SortedClassNodeIds() const
{
    // Iterate classMap_ in deterministic order (sorted by nodeId) so that
    // the same input always produces the same output ordering.
    std::vector<uint32_t> ids;
    ids.reserve(classMap_.size());
    for (const auto &kv : classMap_) {
        ids.push_back(kv.first);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

void StaticRawheapTranslate::CreateClassNodes()
{
    for (uint32_t nodeId : SortedClassNodeIds()) {
        auto it = classMap_.find(nodeId);
        Node *node = GetOrCreateNode(nodeId);
        node->strId = GetOrCreateStringId(it->second.classNameId);
        // These nodes come from TAG_STATIC_CLASS_DUMP / LOAD_CLASS records -
        // restore the CLASS node type rather than collapsing to DEFAULT.
        node->type = CLASS_NODETYPE;
    }
}

void StaticRawheapTranslate::CreateInstanceNodes()
{
    for (auto &rec : instances_) {
        // A class mirror object is dumped both as a STATIC_CLASS_DUMP (keyed by
        // its classObjectId) and as a STATIC_INSTANCE_DUMP (an instance of the
        // metaclass std.core.Class). Skip the instance view so the class node
        // created by CreateClassNodes (name = the class name, type = CLASS) is
        // not overwritten with metaclass-instance data.
        if (classMap_.find(rec.objectNodeId) != classMap_.end()) {
            GetOrCreateNode(rec.objectNodeId)->size = rec.instanceSize;
            continue;
        }
        Node *node = GetOrCreateNode(rec.objectNodeId);
        node->size = rec.instanceSize;
        // These nodes come from TAG_STATIC_INSTANCE_DUMP records - restore
        // the OBJECT node type rather than leaving the default.
        node->type = OBJECT_NODETYPE;
        auto classIt = classMap_.find(rec.classNodeId);
        if (classIt != classMap_.end()) {
            node->strId = GetOrCreateStringId(classIt->second.classNameId);
            if (node->size == 0) {
                node->size = classIt->second.instanceSize;
            }
        }
    }
}

void StaticRawheapTranslate::CreateArrayNodes()
{
    for (auto &rec : arrays_) {
        Node *node = GetOrCreateNode(rec.objectNodeId);
        node->size = rec.instanceSize;
        // These nodes come from TAG_STATIC_ARRAY_DUMP records - restore the
        // ARRAY node type rather than leaving the default.
        node->type = ARRAY_NODETYPE;
        auto classIt = classMap_.find(rec.classNodeId);
        if (classIt != classMap_.end()) {
            node->strId = GetOrCreateStringId(classIt->second.classNameId);
        }
    }
}

void StaticRawheapTranslate::CreateStringNodes()
{
    // String objects arrive via TAG_STATIC_STRING_DUMP (not INSTANCE_DUMP), so
    // they are not in instances_ and would otherwise have no node at all.
    // Create a STRING-typed node named by the content - this is the only path
    // that makes the actual string value visible in the .heapsnapshot.
    for (auto &rec : stringInstances_) {
        Node *node = GetOrCreateNode(rec.objectNodeId);
        node->size = rec.instanceSize;
        node->type = STRING;
        node->strId = InsertAndGetStringId(rec.content);
    }
}

void StaticRawheapTranslate::CreateRootEdges(Node *syntheticRoot, Node *staticRoot)
{
    syntheticRoot->nodeId = 1;  // 1: root node id
    syntheticRoot->type = SYNTHETIC_NODETYPE;
    syntheticRoot->strId = InsertAndGetStringId("SyntheticRoot");
    syntheticRoot->edgeCount = 0;
    staticRoot->nodeId = 0;
    staticRoot->type = ROOT;
    staticRoot->strId = InsertAndGetStringId("StaticRoot[" + std::to_string(roots_.size()) + ']');
    staticRoot->size = VIRTUAL_NODE_SIZE;
    staticRoot->edgeCount = 0;

    StringId subrootStrId = InsertAndGetStringId("-subroot-");
    InsertEdge(syntheticRoot, staticRoot, subrootStrId,
               EdgeType::SHORTCUT);  // syntheticRoot -> StaticRoot
    syntheticRoot->edgeCount++;
    uint32_t index = 0;
    for (uint32_t nodeId : roots_) {
        Node *root = FindNodeByNodeId(nodeId);
        if (root == nullptr) {
            continue;
        }
        InsertEdge(staticRoot, root, index++, EdgeType::ELEMENT);
        staticRoot->edgeCount++;
    }
}

void StaticRawheapTranslate::CreateClassEdges()
{
    // Emit a class node's superclass edge AND its static field edges in one pass
    // so a class's edges stay contiguous in the flat edge vector - required by
    // the .heapsnapshot grouping contract (node i owns the next edgeCount[i]
    // edges).
    for (uint32_t nodeId : SortedClassNodeIds()) {
        Node *classNode = FindNodeByNodeId(nodeId);
        if (classNode == nullptr) {
            continue;
        }
        const auto &info = classMap_[nodeId];
        EmitSuperClassEdge(classNode, info);
        EmitStaticFieldEdges(classNode, info);
        EmitMethodNameEdges(classNode, info);
    }
}

void StaticRawheapTranslate::EmitSuperClassEdge(Node *classNode, const ClassInfo &info)
{
    // class -> superClass (INTERNAL, "superClass"). Without this edge every
    // class node except the root-reachable ones is orphaned; the superclass
    // chain is what makes the class subgraph connected.
    if (info.superClassNodeId == 0 || info.superClassNodeId == classNode->nodeId) {
        return;  // no superclass / self-loop guard
    }
    InsertEdge(classNode, GetOrCreateNode(info.superClassNodeId), InsertAndGetStringId("superClass"),
               EdgeType::INTERNAL);
    classNode->edgeCount++;
}

void StaticRawheapTranslate::EmitStaticFieldEdges(Node *classNode, const ClassInfo &info)
{
    // class -> static field value (PROPERTY, field name). staticValues parallels
    // staticFields (same order - see EmitInstanceFieldEdges).
    // OBJECT/ARRAY values are nodeIds into the heap; primitives get a
    // synthetic value node.
    size_t n = std::min(info.staticFields.size(), info.staticValues.size());
    for (size_t i = 0; i < n; ++i) {
        const auto &val = info.staticValues[i];
        StringId nameId = GetOrCreateStringId(info.staticFields[i].nameId);
        if (EmitFieldValueEdge(classNode, val, nameId)) {
            classNode->edgeCount++;
        }
    }
}

bool StaticRawheapTranslate::EmitFieldValueEdge(Node *from, const FieldValue &value, StringId nameId)
{
    bool isStrongRef = value.type == static_cast<uint8_t>(StaFieldType::OBJECT) ||
                       value.type == static_cast<uint8_t>(StaFieldType::ARRAY);
    bool isWeakRef = value.type == static_cast<uint8_t>(StaFieldType::WEAK_OBJECT);
    if ((isStrongRef || isWeakRef) && value.value == 0) {
        return false;
    }
    if (isStrongRef || isWeakRef) {
        EdgeType edgeType = isWeakRef ? EdgeType::WEAK : EdgeType::PROPERTY;
        InsertEdge(from, GetOrCreateNode(static_cast<uint32_t>(value.value)), nameId, edgeType);
        return true;
    }
    InsertEdge(from, GetOrCreateValueNode(value.type, value.value), nameId, EdgeType::PROPERTY);
    return true;
}

void StaticRawheapTranslate::EmitMethodNameEdges(Node *classNode, const ClassInfo &info)
{
    // class -> method (PROPERTY, method name). The static dumper writes every
    // declared method's name into the dump string pool and records its id in
    // CLASS_DUMP's methodNameId[] (ReadMethodNames -> info.methodNameIds).
    // GetOrCreateStringId is the bridge that promotes a dump string-pool id
    // into the .heapsnapshot strings table - calling it here (on the edge name)
    // is what makes method names appear in the output at all. Without this
    // path the ids are read into ClassInfo but never referenced, so the
    // strings never leave the binary. Each method becomes a synthetic
    // "closure" node named after the method; the edge name is the method name.
    for (uint32_t methodNameId : info.methodNameIds) {
        StringId nameId = GetOrCreateStringId(methodNameId);
        Node *methodNode = GetOrCreateMethodNode(nameId);
        InsertEdge(classNode, methodNode, nameId, EdgeType::PROPERTY);
        classNode->edgeCount++;
    }
}
void StaticRawheapTranslate::CreateInstanceEdges()
{
    // Instance field edges (PROPERTY). Field values map to descriptors in order:
    // first the static-field descriptors, then the instance-field descriptors.
    for (auto &rec : instances_) {
        Node *node = FindNodeByNodeId(rec.objectNodeId);
        if (node == nullptr) {
            continue;
        }
        // instance -> class edge ("hclass", PROPERTY). Mirrors the hidden-class
        // edge and makes class nodes reachable from their instances.
        if (rec.classNodeId != 0 && rec.classNodeId != rec.objectNodeId) {
            InsertEdge(node, GetOrCreateNode(rec.classNodeId), InsertAndGetStringId("hclass"), EdgeType::DEFAULT);
            node->edgeCount++;
        }
        auto classIt = classMap_.find(rec.classNodeId);
        if (classIt == classMap_.end()) {
            EmitFallbackFieldEdges(node, rec);
            continue;
        }
        EmitInstanceFieldEdges(node, rec, classIt->second);
    }
}

void StaticRawheapTranslate::EmitInstanceFieldEdges(Node *node, const InstanceRecord &rec, const ClassInfo &info)
{
    // INSTANCE_DUMP carries instance field values only. The values follow the
    // instance-field descriptors serialized in CLASS_DUMP, including inherited
    // fields, in the same order. Static field values are serialized separately
    // in CLASS_DUMP and therefore are not part of rec.values.
    // A class mirror's metaclass has an instance field literally named
    // "superClass" pointing at the superclass mirror. EmitSuperClassEdge
    // already emits that relationship as an [internal] superClass edge (from
    // CLASS_DUMP.superClassId), so emitting it again here would create a
    // duplicate [property] superClass edge. Skip it for class nodes only.
    bool isClassNode = (node->type == CLASS_NODETYPE);
    size_t idx = 0;
    for (size_t i = 0; i < info.instanceFields.size() && idx < rec.values.size(); ++i, ++idx) {
        const auto &val = rec.values[idx];
        StringId nameId = GetOrCreateStringId(info.instanceFields[i].nameId);
        if (isClassNode && GetString(info.instanceFields[i].nameId) == "superClass") {
            continue;  // deduplicated by EmitSuperClassEdge
        }
        if (EmitFieldValueEdge(node, val, nameId)) {
            node->edgeCount++;
        }
    }
}

void StaticRawheapTranslate::EmitFallbackFieldEdges(Node *node, const InstanceRecord &rec)
{
    LOG_INFO_ << "CreateInstanceEdges: instance nodeId 0x" << std::hex << rec.objectNodeId << " classNodeId 0x" <<
        rec.classNodeId << std::dec << " not found in classMap, using fallback type name";
    node->strId = InsertAndGetStringId("Object");
    for (auto &val : rec.values) {
        bool isStrongRef = val.type == static_cast<uint8_t>(StaFieldType::OBJECT) ||
                           val.type == static_cast<uint8_t>(StaFieldType::ARRAY);
        bool isWeakRef = val.type == static_cast<uint8_t>(StaFieldType::WEAK_OBJECT);
        if (val.value == 0 || (!isStrongRef && !isWeakRef)) {
            continue;
        }
        InsertEdge(node, GetOrCreateNode(static_cast<uint32_t>(val.value)), InsertAndGetStringId(""),
                   isWeakRef ? EdgeType::WEAK : EdgeType::PROPERTY);
        node->edgeCount++;
    }
}

// Boxed-wrapper class name for a primitive element type. ETS primitive arrays
// store raw values (no boxed objects in the heap); the snapshot boxes each
// element so array contents are visible, like the dynamic side.
static const char *BoxedWrapperName(StaFieldType t)
{
    switch (t) {
        case StaFieldType::BOOLEAN:
            return "std.core.Boolean";
        case StaFieldType::BYTE:
            return "std.core.Byte";
        case StaFieldType::CHAR:
            return "std.core.Char";
        case StaFieldType::SHORT:
            return "std.core.Short";
        case StaFieldType::INT:
            return "std.core.Int";
        case StaFieldType::LONG:
            return "std.core.Long";
        case StaFieldType::FLOAT:
            return "std.core.Float";
        case StaFieldType::DOUBLE:
            return "std.core.Double";
        default:
            return "std.core.Object";
    }
}

// Read one little-endian primitive of `esz` bytes from a captured byte buffer.
// Returns 0 for unsupported widths (caller skips boxing when esz is 0).
static uint64_t ReadPrimitiveFromBuffer(const char *data, uint32_t index, uint8_t esz)
{
    char *p = const_cast<char *>(data + static_cast<size_t>(index) * esz);
    switch (esz) {
        case sizeof(uint8_t):
            return static_cast<uint8_t>(p[0]);
        case sizeof(uint16_t):
            return ByteToU16(p);
        case sizeof(uint32_t):
            return ByteToU32(p);
        case sizeof(uint64_t):
            return ByteToU64(p);
        default:
            return 0;
    }
}

void StaticRawheapTranslate::CreateArrayEdges()
{
    // Each array owns a synthetic "buffer" sub-node carrying its ELEMENT edges:
    // OBJECT/ARRAY elements point at heap nodes, primitives are boxed into
    // std.core.<Type> wrappers. Name ids are shared across all arrays.
    StringId bufferNameId = InsertAndGetStringId("buffer");
    StringId valueNameId = InsertAndGetStringId("value");
    for (auto &rec : arrays_) {
        Node *node = FindNodeByNodeId(rec.objectNodeId);
        if (node == nullptr) {
            continue;
        }
        Node *buffer = CreateArrayBufferNode(node, bufferNameId);
        bool isRef = (rec.elementType == static_cast<uint8_t>(StaFieldType::OBJECT) ||
                      rec.elementType == static_cast<uint8_t>(StaFieldType::ARRAY) ||
                      rec.elementType == static_cast<uint8_t>(StaFieldType::WEAK_OBJECT));
        if (rec.elementType == static_cast<uint8_t>(StaFieldType::TAGGED)) {
            EmitTaggedArrayElementEdges(buffer, rec);
        } else if (isRef) {
            EmitArrayRefElementEdges(buffer, rec);
        } else {
            EmitArrayBoxedPrimitiveEdges(buffer, rec, valueNameId);
        }
    }
}

Node *StaticRawheapTranslate::CreateArrayBufferNode(Node *arrayNode, StringId bufferNameId)
{
    // The buffer sub-node owns the array's ELEMENT edges; shares the array's
    // class name (e.g. "int[]") to retain its identity.
    uint32_t bufSynId = 0x40000000 + valueNodeCounter_++;
    Node *buffer = GetOrCreateNode(bufSynId);
    buffer->nodeId = bufSynId;
    buffer->type = ARRAY_NODETYPE;
    buffer->strId = arrayNode->strId;
    buffer->size = 0;
    InsertEdge(arrayNode, buffer, bufferNameId, EdgeType::PROPERTY);
    arrayNode->edgeCount++;
    return buffer;
}

void StaticRawheapTranslate::EmitArrayRefElementEdges(Node *buffer, const ArrayRecord &rec)
{
    // One ELEMENT edge per non-null nodeId; index advances over nulls so
    // positions match the source array ordering.
    uint32_t index = 0;
    for (uint32_t elemNodeId : rec.elements) {
        if (elemNodeId != 0) {
            InsertEdge(buffer, GetOrCreateNode(elemNodeId), index, EdgeType::ELEMENT);
            buffer->edgeCount++;
        }
        ++index;
    }
}

void StaticRawheapTranslate::EmitTaggedArrayElementEdges(Node *buffer, const ArrayRecord &rec)
{
    uint32_t index = 0;
    for (const auto &value : rec.taggedValues) {
        bool isStrongRef = value.type == static_cast<uint8_t>(StaFieldType::OBJECT) ||
                           value.type == static_cast<uint8_t>(StaFieldType::ARRAY);
        bool isWeakRef = value.type == static_cast<uint8_t>(StaFieldType::WEAK_OBJECT);
        if ((isStrongRef || isWeakRef) && value.value == 0) {
            ++index;
            continue;
        }
        if (isStrongRef) {
            InsertEdge(buffer, GetOrCreateNode(static_cast<uint32_t>(value.value)), index, EdgeType::ELEMENT);
        } else if (isWeakRef) {
            StringId nameId = InsertAndGetStringId(std::to_string(index));
            InsertEdge(buffer, GetOrCreateNode(static_cast<uint32_t>(value.value)), nameId, EdgeType::WEAK);
        } else {
            InsertEdge(buffer, GetOrCreateValueNode(value.type, value.value), index, EdgeType::ELEMENT);
        }
        buffer->edgeCount++;
        ++index;
    }
}

void StaticRawheapTranslate::EmitArrayBoxedPrimitiveEdges(Node *buffer, const ArrayRecord &rec, StringId valueNameId)
{
    // Box each primitive element into a std.core.<Type> wrapper with a "value"
    // edge to a synthetic number/string node. No-op when there is no payload.
    uint8_t esz = FieldSize(rec.elementType);
    if (esz == 0 || rec.length == 0 || rec.primData.size() < static_cast<size_t>(rec.length) * esz) {
        return;
    }
    StringId wrapperNameId = InsertAndGetStringId(BoxedWrapperName(static_cast<StaFieldType>(rec.elementType)));
    uint32_t index = 0;
    for (uint32_t j = 0; j < rec.length; ++j) {
        uint64_t val = ReadPrimitiveFromBuffer(rec.primData.data(), j, esz);
        uint32_t wSynId = 0x40000000 + valueNodeCounter_++;
        Node *wrapper = GetOrCreateNode(wSynId);
        wrapper->nodeId = wSynId;
        wrapper->type = OBJECT_NODETYPE;
        wrapper->strId = wrapperNameId;
        wrapper->size = 0;
        Node *valueNode = GetOrCreateValueNode(rec.elementType, val);
        InsertEdge(buffer, wrapper, index, EdgeType::ELEMENT);
        buffer->edgeCount++;
        InsertEdge(wrapper, valueNode, valueNameId, EdgeType::PROPERTY);
        wrapper->edgeCount++;
        ++index;
    }
}

void StaticRawheapTranslate::CreateStringEdges()
{
    // string -> class ("hclass", PROPERTY). Mirrors CreateInstanceEdges'
    // instance->class edge so string nodes are connected to the std.core.String
    // class node (and through it to the rest of the graph). The string's value
    // is already carried as the node name, so no further edges are needed.
    for (auto &rec : stringInstances_) {
        Node *node = FindNodeByNodeId(rec.objectNodeId);
        if (node == nullptr) {
            continue;
        }
        if (rec.classNodeId != 0 && rec.classNodeId != rec.objectNodeId) {
            InsertEdge(node, GetOrCreateNode(rec.classNodeId), InsertAndGetStringId("hclass"), EdgeType::DEFAULT);
            node->edgeCount++;
        }
    }
}

Node *StaticRawheapTranslate::GetOrCreateValueNode(uint8_t fieldType, uint64_t value)
{
    // Synthetic nodeId in a high range so it cannot collide with real heap
    // addresses (which are even, low u32 nodeIds). The merger's duplicate-nodeId
    // detection is the backstop if a collision ever occurs.
    uint32_t synId = 0x40000000 + valueNodeCounter_++;
    Node *node = GetOrCreateNode(synId);
    node->nodeId = synId;
    if (fieldType == static_cast<uint8_t>(StaFieldType::CHAR)) {
        node->type = STRING;
    } else if (fieldType == static_cast<uint8_t>(StaFieldType::TAGGED)) {
        node->type = SYNTHETIC_NODETYPE;
    } else {
        node->type = HEAP_NUMBER;
    }
    node->strId = InsertAndGetStringId(MakeValueNodeName(fieldType, value));
    node->size = 0;
    return node;
}

Node *StaticRawheapTranslate::GetOrCreateMethodNode(StringId nameId)
{
    // Synthetic "closure" node representing a declared method, named after the
    // method. nodeId lives in the same high synthetic range (0x40000000+) as
    // value nodes so it cannot collide with real heap addresses. One node per
    // method-name occurrence (one node per method-name occurrence, each function
    // is its own node); the counter is shared with GetOrCreateValueNode so ids
    // stay unique.
    uint32_t synId = 0x40000000 + valueNodeCounter_++;
    Node *node = GetOrCreateNode(synId);
    node->nodeId = synId;
    node->type = CLOSURE_NODETYPE;
    node->strId = nameId;
    node->size = 0;
    return node;
}

std::string StaticRawheapTranslate::MakeValueNodeName(uint8_t fieldType, uint64_t value) const
{
    auto ft = static_cast<StaFieldType>(fieldType);
    switch (ft) {
        case StaFieldType::BOOLEAN:
            return (value != 0) ? "true" : "false";
        case StaFieldType::CHAR:
            return EncodeEtsChar(static_cast<uint16_t>(value));
        case StaFieldType::TAGGED:
            return MakeTaggedValueNodeName(value);
        case StaFieldType::BYTE:
            return std::to_string(static_cast<int8_t>(value));
        case StaFieldType::SHORT:
            return std::to_string(static_cast<int16_t>(value));
        case StaFieldType::INT:
            return std::to_string(static_cast<int32_t>(value));
        case StaFieldType::LONG:
            return std::to_string(static_cast<int64_t>(value));
        case StaFieldType::FLOAT: {
            float f;
            (void)memcpy_s(&f, sizeof(f), &value, sizeof(f));
            return std::to_string(f);
        }
        case StaFieldType::DOUBLE: {
            double d;
            (void)memcpy_s(&d, sizeof(d), &value, sizeof(d));
            return std::to_string(d);
        }
        default:
            return std::to_string(value);  // UNKNOWN / anything else: raw bits
    }
}

std::string StaticRawheapTranslate::MakeTaggedValueNodeName(uint64_t value) const
{
    switch (value) {
        case STATIC_TAGGED_HOLE:
            return "hole";
        case STATIC_TAGGED_NULL:
            return "null";
        case STATIC_TAGGED_FALSE:
            return "false";
        case STATIC_TAGGED_TRUE:
            return "true";
        case STATIC_TAGGED_UNDEFINED:
            return "undefined";
        case STATIC_TAGGED_EXCEPTION:
            return "exception";
        default: {
            constexpr int taggedHexWidth = sizeof(uint64_t) * BITS_PER_BYTE / BITS_PER_HEX_DIGIT;
            std::ostringstream stream;
            stream << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(taggedHexWidth) << value;
            return stream.str();
        }
    }
}

Node *StaticRawheapTranslate::GetOrCreateNode(uint32_t nodeId)
{
    auto it = nodeIdToNode_.find(nodeId);
    if (it != nodeIdToNode_.end()) {
        return it->second;
    }
    Node *node = CreateNode();
    node->nodeId = nodeId;
    nodeIdToNode_.emplace(nodeId, node);
    return node;
}

std::string StaticRawheapTranslate::GetString(uint32_t stringId) const
{
    auto it = stringTable_.find(stringId);
    return it == stringTable_.end() ? std::string() : it->second;
}

StringId StaticRawheapTranslate::GetOrCreateStringId(uint32_t stringId)
{
    return InsertAndGetStringId(GetString(stringId));
}

// ---- Primitive readers (use internal file_ pointer) ----

bool StaticRawheapTranslate::ReadBytes(char *buffer, uint32_t size)
{
    if (readingRecord_ && size > recordRemaining_) {
        LOG_ERROR_ << "record read of " << size << " bytes exceeds remaining " << recordRemaining_;
        parseOk_ = false;
        return false;
    }
    if (!file_->Read(buffer, size)) {
        parseOk_ = false;
        return false;
    }
    if (readingRecord_) {
        recordRemaining_ -= size;
    }
    return true;
}

uint8_t StaticRawheapTranslate::ReadU8()
{
    char buf[1] = {0};
    if (!ReadBytes(buf, 1)) {
        return 0;
    }
    return static_cast<uint8_t>(buf[0]);
}

uint16_t StaticRawheapTranslate::ReadU16()
{
    char buf[sizeof(uint16_t)] = {0};
    if (!ReadBytes(buf, sizeof(uint16_t))) {
        return 0;
    }
    return ByteToU16(buf);
}

uint32_t StaticRawheapTranslate::ReadU32()
{
    char buf[sizeof(uint32_t)] = {0};
    if (!ReadBytes(buf, sizeof(uint32_t))) {
        return 0;
    }
    return ByteToU32(buf);
}

uint64_t StaticRawheapTranslate::ReadU64()
{
    char buf[sizeof(uint64_t)] = {0};
    if (!ReadBytes(buf, sizeof(uint64_t))) {
        return 0;
    }
    return ByteToU64(buf);
}

uint64_t StaticRawheapTranslate::ReadFieldValue(uint8_t byteSize)
{
    if (byteSize == 0) {
        return 0;
    }
    char buf[sizeof(uint64_t)] = {0};  // max field size is 8 bytes (LONG/DOUBLE)
    if (!ReadBytes(buf, byteSize)) {
        return 0;
    }
    uint64_t val = 0;
    for (uint8_t b = 0; b < byteSize; ++b) {
        val |= static_cast<uint64_t>(static_cast<uint8_t>(buf[b])) << (BITS_PER_BYTE * b);
    }
    return val;
}

uint8_t StaticRawheapTranslate::FieldSize(uint8_t fieldType)
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
            return sizeof(uint32_t);  // nodeId (4 bytes) instead of address (8 bytes)
        case static_cast<uint8_t>(StaFieldType::UNKNOWN):
        default:
            return 0;
    }
}

}  // namespace rawheap_translate
