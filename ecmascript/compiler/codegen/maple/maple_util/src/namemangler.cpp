/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "namemangler.h"
#include <regex>
#include <cassert>
#include <map>

namespace namemangler {
#ifdef __MRT_DEBUG
#define DEBUG_ASSERT(f) assert(f)
#else
#define DEBUG_ASSERT(f) ((void)0)
#endif

const int kLocalCodebufSize = 1024;
const int kMaxCodecbufSize = (1 << 16);  // Java spec support a max name length of 64K.

#define GETHEXCHAR(n) static_cast<char>((n) < 10 ? (n) + '0' : (n)-10 + 'a')
#define GETHEXCHARU(n) static_cast<char>((n) < 10 ? (n) + '0' : (n)-10 + 'A')

bool doCompression = false;

// Store a mapping between full string and its compressed version
// More frequent and specific strings go before  general ones,
// e.g. Ljava_2Flang_2FObject_3B goes before Ljava_2Flang_2F
//
using StringMap = std::map<const std::string, const std::string>;

const StringMap kInternalMangleTable = {
    {"Ljava_2Flang_2FObject_3B", "L0_3B"}, {"Ljava_2Flang_2FClass_3B", "L1_3B"}, {"Ljava_2Flang_2FString_3B", "L2_3B"}};

// This mapping is mainly used for compressing annotation strings
const StringMap kOriginalMangleTable = {
    {"Ljava/lang/Object", "L0"}, {"Ljava/lang/Class", "L1"}, {"Ljava/lang/String", "L2"}};

// The returned buffer needs to be explicitly freed
static inline char *AllocCodecBuf(size_t maxLen)
{
    if (maxLen == 0) {
        return nullptr;
    }
    // each char may have 2 more char, so give out the max space buffer
    constexpr int multi = 3;
    return reinterpret_cast<char *>(malloc((maxLen <= kLocalCodebufSize) ? multi * maxLen : multi * kMaxCodecbufSize));
}

static inline void FreeCodecBuf(char *buf)
{
    free(buf);
}

static std::string CompressName(std::string &name, const StringMap &mapping = kInternalMangleTable)
{
    for (auto &entry : mapping) {
        if (name.find(entry.first) != std::string::npos) {
            name = std::regex_replace(name, std::regex(entry.first), entry.second);
        }
    }
    return name;
}

static std::string DecompressName(std::string &name, const StringMap &mapping = kInternalMangleTable)
{
    for (auto &entry : mapping) {
        if (name.find(entry.second) != std::string::npos) {
            name = std::regex_replace(name, std::regex(entry.second), entry.first);
        }
    }
    return name;
}

std::string GetInternalNameLiteral(std::string name)
{
    return (doCompression ? CompressName(name) : name);
}

std::string GetOriginalNameLiteral(std::string name)
{
    return (doCompression ? CompressName(name, kOriginalMangleTable) : name);
}

std::string EncodeName(const std::string &name)
{
    // name is guaranteed to be null-terminated
    size_t nameLen = name.length();
    nameLen = nameLen > kMaxCodecbufSize ? kMaxCodecbufSize : nameLen;
    char *buf = AllocCodecBuf(nameLen);
    if (buf == nullptr) {
        return std::string(name);
    }

    size_t pos = 0;
    size_t i = 0;
    std::string str(name);
    std::u16string str16;
    while (i < nameLen) {
        unsigned char c = static_cast<unsigned char>(name[i]);
        if (c == '_') {
            buf[pos++] = '_';
            buf[pos++] = '_';
        } else if (c == '[') {
            buf[pos++] = 'A';
        } else if (isalnum(c)) {
            buf[pos++] = static_cast<char>(c);
        } else if (c <= 0x7F) {
            // _XX: '_' followed by ascii code in hex
            if (c == '.') {
                c = '/';  // use / in package name
            }
            buf[pos++] = '_';
            unsigned char n = c >> 4;  // get the high 4 bit and calculate
            buf[pos++] = GETHEXCHARU(n);
            n = static_cast<unsigned char>(c - static_cast<unsigned char>(n << 4));  // revert the high 4 bit
            buf[pos++] = GETHEXCHARU(n);
        } else {
            str16.clear();
            // process one 16-bit char at a time
            unsigned int n = UTF8ToUTF16(str16, str.substr(i), 1, false);
            buf[pos++] = '_';
            if ((n >> 16) == 1) {  // if n is 16-bit
                unsigned short m = str16[0];
                buf[pos++] = 'u';
                buf[pos++] = GETHEXCHAR((m & 0xF000) >> 12);
                buf[pos++] = GETHEXCHAR((m & 0x0F00) >> 8);
                buf[pos++] = GETHEXCHAR((m & 0x00F0) >> 4);
                buf[pos++] = GETHEXCHAR(m & 0x000F);
            } else {
                unsigned short m = str16[0];
                buf[pos++] = 'U';
                buf[pos++] = GETHEXCHAR((m & 0xF000) >> 12);
                buf[pos++] = GETHEXCHAR((m & 0x0F00) >> 8);
                buf[pos++] = GETHEXCHAR((m & 0x00F0) >> 4);
                buf[pos++] = GETHEXCHAR(m & 0x000F);
                m = str16[1];
                buf[pos++] = GETHEXCHAR((m & 0xF000) >> 12);
                buf[pos++] = GETHEXCHAR((m & 0x0F00) >> 8);
                buf[pos++] = GETHEXCHAR((m & 0x00F0) >> 4);
                buf[pos++] = GETHEXCHAR(m & 0x000F);
            }
            i += static_cast<size_t>(int32_t(n & 0xFFFF) - 1);
        }
        i++;
    }

    buf[pos] = '\0';
    std::string newName = std::string(buf, pos);
    FreeCodecBuf(buf);
    if (doCompression) {
        newName = CompressName(newName);
    }
    return newName;
}

static inline bool UpdatePrimType(bool primType, int splitNo, uint32_t ch)
{
    if (ch == 'L') {
        return false;
    }

    if (((ch == ';') || (ch == '(') || (ch == ')')) && (splitNo > 1)) {
        return true;
    }

    return primType;
}

namespace {
constexpr int kNumLimit = 10;
constexpr int kCodeOffset3 = 12;
constexpr int kCodeOffset2 = 8;
constexpr int kCodeOffset = 4;
constexpr int kJavaStrLength = 5;
constexpr size_t k64BitShift = 6; // 64 is 1 << 6
}

std::string DecodeName(const std::string &name)
{
    if (name.find(';') != std::string::npos) {  // no need Decoding a non-encoded string
        return name;
    }
    std::string decompressedName;
    const char *namePtr = nullptr;
    size_t nameLen;

    if (doCompression) {
        decompressedName = name;
        decompressedName = DecompressName(decompressedName);
        namePtr = decompressedName.c_str();
        nameLen = decompressedName.length();
    } else {
        namePtr = name.c_str();
        nameLen = name.length();
    }

    // Demangled name is supposed to be shorter. No buffer overflow issue here.
    std::string newName(nameLen, '\0');

    bool primType = true;
    int splitNo = 0;  // split: class 0 | method 1 | signature 2
    size_t pos = 0;
    std::string str;
    std::u16string str16;
    for (size_t i = 0; i < nameLen;) {
        unsigned char c = static_cast<unsigned char>(namePtr[i]);
        ++i;
        if (c == '_') {  // _XX: '_' followed by ascii code in hex
            if (i >= nameLen) {
                break;
            }
            if (namePtr[i] == '_') {
                newName[pos++] = namePtr[i++];
            } else if (namePtr[i] == 'u') {
                str.clear();
                str16.clear();
                i++;
                c = static_cast<unsigned char>(namePtr[i++]);
                uint8_t b1 = (c <= '9') ? c - '0' : c - 'a' + kNumLimit;
                c = static_cast<unsigned char>(namePtr[i++]);
                uint8_t b2 = (c <= '9') ? c - '0' : c - 'a' + kNumLimit;
                c = static_cast<unsigned char>(namePtr[i++]);
                uint8_t b3 = (c <= '9') ? c - '0' : c - 'a' + kNumLimit;
                c = static_cast<unsigned char>(namePtr[i++]);
                uint8_t b4 = (c <= '9') ? c - '0' : c - 'a' + kNumLimit;
                uint32_t codepoint = (b1 << kCodeOffset3) | (b2 << kCodeOffset2) | (b3 << kCodeOffset) | b4;
                str16 += (char16_t)codepoint;
                unsigned int count = UTF16ToUTF8(str, str16, 1, false) >> 16; // shift 16 to get count
                if (count == 2) {  // the count of str equal 2 to 4, use array to save the utf8
                    newName[pos++] = str[0];
                    newName[pos++] = str[1];
                } else if (count == 3) {
                    newName[pos++] = str[0];
                    newName[pos++] = str[1];
                    newName[pos++] = str[2]; // 2 is index of third char
                } else if (count == 4) {
                    newName[pos++] = str[0];
                    newName[pos++] = str[1];
                    newName[pos++] = str[2]; // 2 is index of third char
                    newName[pos++] = str[3]; // 3 is index of fourth char
                }
            } else {
                c = static_cast<unsigned char>(namePtr[i++]);
                unsigned int v = (c <= '9') ? c - '0' : c - 'A' + kNumLimit;
                unsigned int asc = v << kCodeOffset;
                if (i >= nameLen) {
                    break;
                }
                c = static_cast<unsigned char>(namePtr[i++]);
                v = (c <= '9') ? c - '0' : c - 'A' + kNumLimit;
                asc += v;

                newName[pos++] = static_cast<char>(asc);

                if (asc == '|') {
                    splitNo++;
                }

                primType = UpdatePrimType(primType, splitNo, asc);
            }
        } else {
            if (splitNo < 2) {  // split: class 0 | method 1 | signature 2
                newName[pos++] = static_cast<char>(c);
                continue;
            }

            primType = UpdatePrimType(primType, splitNo, c);
            if (primType) {
                newName[pos++] = (c == 'A') ? '[' : c;
            } else {
                newName[pos++] = static_cast<char>(c);
            }
        }
    }

    newName.resize(pos);
    return newName;
}

// input: maple name
// output: Ljava/lang/Object;  [Ljava/lang/Object;
void DecodeMapleNameToJavaDescriptor(const std::string &nameIn, std::string &nameOut)
{
    nameOut = DecodeName(nameIn);
    if (nameOut[0] == 'A') {
        size_t i = 0;
        while (nameOut[i] == 'A') {
            nameOut[i++] = '[';
        }
    }
}

// convert maple name to java name
// http://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/design.html#resolving_native_method_names
std::string NativeJavaName(const std::string &name, bool overLoaded)
{
    // Decompress name first because the generated native function name needs
    // to follow certain spec, not something maple can control.
    std::string decompressedName(name);
    if (doCompression) {
        decompressedName = DecompressName(decompressedName);
    }

    unsigned int nameLen = static_cast<unsigned int>(decompressedName.length()) + kJavaStrLength;
    std::string newName = "Java_";
    unsigned int i = 0;

    // leading A's are array
    while (i < nameLen && name[i] == 'A') {
        newName += "_3";
        i++;
    }

    bool isProto = false;  // class names in prototype have 'L' and ';'
    bool isFuncname = false;
    bool isTypename = false;
    while (i < nameLen) {
        char c = decompressedName[i];
        if (c == '_') {
            i++;
            // UTF16 unicode
            if (decompressedName[i] == 'u') {
                newName += "_0";
                i++;
            } else if (decompressedName[i] == '_') {
                newName += "_1";
                i++;
            } else {
                // _XX: '_' followed by ascii code in hex
                c = decompressedName[i++];
                unsigned char v = (c <= '9') ? c - '0' : c - 'A' + kNumLimit;
                unsigned char asc = v << kCodeOffset;
                c = decompressedName[i++];
                v = (c <= '9') ? c - '0' : c - 'A' + kNumLimit;
                asc += v;
                if (asc == '/') {
                    newName += "_";
                } else if (asc == '|' && !isFuncname) {
                    newName += "_";
                    isFuncname = true;
                } else if (asc == '|' && isFuncname) {
                    if (!overLoaded) {
                        break;
                    }
                    newName += "_";
                    isFuncname = false;
                } else if (asc == '(') {
                    newName += "_";
                    isProto = true;
                } else if (asc == ')') {
                    break;
                } else if (asc == ';' && !isFuncname) {
                    if (isProto) {
                        newName += "_2";
                    }
                    isTypename = false;
                } else if (asc == '$') {
                    newName += "_00024";
                } else if (asc == '-') {
                    newName += "_0002d";
                } else {
                    printf("name = %s\n", decompressedName.c_str());
                    printf("c = %c\n", asc);
                    DEBUG_ASSERT(false && "more cases in NativeJavaName");
                }
            }
        } else {
            if (c == 'L' && !isFuncname && !isTypename) {
                if (isProto) {
                    newName += c;
                }
                isTypename = true;
                i++;
            } else if (c == 'A' && !isTypename && !isFuncname) {
                while (name[i] == 'A') {
                    newName += "_3";
                    i++;
                }
            } else {
                newName += c;
                i++;
            }
        }
    }
    return newName;
}

static uint16_t ChangeEndian16(uint16_t u16)
{
    return ((u16 & 0xFF00) >> kCodeOffset2) | ((u16 & 0xFF) << kCodeOffset2);
}

/* UTF8
 * U+0000 - U+007F   0xxxxxxx
 * U+0080 - U+07FF   110xxxxx 10xxxxxx
 * U+0800 - U+FFFF   1110xxxx 10xxxxxx 10xxxxxx
 * U+10000- U+10FFFF 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 *
 * UTF16
 * U+0000 - U+D7FF   codePoint
 * U+E000 - U+FFFF   codePoint
 * U+10000- U+10FFFF XXXX YYYY
 *   code = codePoint - 0x010000, ie, 20-bit number in the range 0x000000..0x0FFFFF
 *   XXXX: top 10 bits of code + 0xD800: 0xD800..0xDBFF
 *   YYYY: low 10 bits of code + 0xDC00: 0xDC00..0xDFFF
 *
 * convert upto num UTF8 elements
 * return two 16-bit values: return_number_of_elements | consumed_input_number_of_elements
 */
const int kCodepointOffset1 = 6;   // U+0080 - U+07FF   110xxxxx 10xxxxxx
const int kCodepointOffset2 = 12;  // U+0800 - U+FFFF   1110xxxx 10xxxxxx 10xxxxxx
const int kCodepointOffset3 = 18;  // U+10000- U+10FFFF 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
const int kCountOffset = 16;
const int kCodeAfterMinusOffset = 10;  // codePoint equals itself minus 0x10000

unsigned UTF16ToUTF8(std::string &str, const std::u16string &str16, unsigned short num, bool isBigEndian)
{
    uint32_t codePoint = 0;
    uint32_t i = 0;
    unsigned short count = 0;
    unsigned short retNum = 0;
    while (i < str16.length()) {
        if (isBigEndian || num == 1) {
            codePoint = str16[i++];
        } else {
            codePoint = ChangeEndian16(str16[i++]);
        }
        if (codePoint > 0xFFFF) {
            codePoint &= 0x3FF;
            codePoint <<= kNumLimit;
            if (isBigEndian) {
                codePoint += str16[i++] & 0x3FF;
            } else {
                codePoint += ChangeEndian16(str16[i++]) & 0x3FF;
            }
        }
        if (codePoint <= 0x7F) {
            str += static_cast<char>(codePoint);
            retNum += 1;  // 1 UTF8 char
        } else if (codePoint <= 0x7FF) {
            str += static_cast<char>(0xC0 + (codePoint >> kCodepointOffset1));
            str += static_cast<char>(0x80 + (codePoint & 0x3F));
            retNum += 2;  // 2 UTF8 chars
        } else if (codePoint <= 0xFFFF) {
            str += static_cast<char>(0xE0 + ((codePoint >> kCodepointOffset2) & 0xF));
            str += static_cast<char>(0x80 + ((codePoint >> kCodepointOffset1) & 0x3F));
            str += static_cast<char>(0x80 + (codePoint & 0x3F));
            retNum += 3;  // 3 UTF8 chars
        } else {
            str += static_cast<char>(0xF0 + ((codePoint >> kCodepointOffset3) & 0x7));
            str += static_cast<char>(0x80 + ((codePoint >> kCodepointOffset2) & 0x3F));
            str += static_cast<char>(0x80 + ((codePoint >> kCodepointOffset1) & 0x3F));
            str += static_cast<char>(0x80 + (codePoint & 0x3F));
            retNum += 4;  // 4 UTF8 chars
        }
        count++;
        if (num == count) {
            return ((static_cast<unsigned>(retNum)) << kCountOffset) | static_cast<unsigned>(i);
        }
    }
    return i;
}

bool NeedConvertUTF16(const std::string &str8)
{
    uint32_t a = 0;
    size_t i = 0;
    size_t size = str8.length();
    while (i < size) {
        a = static_cast<uint8_t>(str8[i++]);
        constexpr uint8_t maxValidAscii = 0x7F;
        if (a > maxValidAscii) {
            return true;
        }
    }
    return false;
}

uint32_t GetCodePoint(const std::string &str8, uint32_t &i)
{
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t codePoint = 0;
    uint32_t a = static_cast<uint8_t>(str8[i++]);
    if (a <= 0x7F) {  // 0...
        codePoint = a;
    } else if (a >= 0xF0) {  // 11110...
        b = static_cast<uint32_t>(str8[i++]);
        c = static_cast<uint32_t>(str8[i++]);
        d = static_cast<uint32_t>(str8[i++]);
        codePoint = ((a & 0x7) << kCodepointOffset3) | ((b & 0x3F) << kCodepointOffset2) |
                    ((c & 0x3F) << kCodepointOffset1) | (d & 0x3F);
    } else if (a >= 0xE0) {  // 1110...
        b = static_cast<uint32_t>(str8[i++]);
        c = static_cast<uint32_t>(str8[i++]);
        codePoint = ((a & 0xF) << kCodepointOffset2) | ((b & 0x3F) << kCodepointOffset1) | (c & 0x3F);
    } else if (a >= 0xC0) {  // 110...
        b = static_cast<uint32_t>(str8[i++]);
        codePoint = ((a & 0x1F) << kCodepointOffset1) | (b & 0x3F);
    } else {
        DEBUG_ASSERT(false && "invalid UTF-8");
    }
    return codePoint;
}

// convert upto num UTF16 elements
// two 16-bit values: return_number_of_elements | consumed_input_number_of_elements
unsigned UTF8ToUTF16(std::u16string &str16, const std::string &str8, unsigned short num, bool isBigEndian)
{
    uint32_t i = 0;
    unsigned short count = 0;
    unsigned short retNum = 0;
    while (i < str8.length()) {
        uint32_t codePoint = GetCodePoint(str8, i);
        if (codePoint <= 0xFFFF) {
            if (isBigEndian || num == 1) {
                str16 += static_cast<char16_t>(codePoint);
            } else {
                str16 += static_cast<char16_t>(ChangeEndian16(static_cast<uint16_t>(codePoint)));
            }
            retNum += 1;  // 1 utf16
        } else {
            codePoint -= 0x10000;
            if (isBigEndian || num == 1) {
                str16 += static_cast<char16_t>((codePoint >> kCodeAfterMinusOffset) | 0xD800);
                str16 += static_cast<char16_t>((codePoint & 0x3FF) | 0xDC00);
            } else {
                str16 += static_cast<char16_t>(
                    ChangeEndian16(static_cast<uint16_t>((codePoint >> kCodeAfterMinusOffset) | 0xD800)));
                str16 += static_cast<char16_t>(ChangeEndian16((codePoint & 0x3FF) | 0xDC00));
            }
            retNum += 2;  // 2 utf16
        }
        count++;
        // only convert num elmements
        if (num == count) {
            return (static_cast<char16_t>(retNum) << kCountOffset) | static_cast<char16_t>(i);
        }
    }
    return i;
}

const uint32_t kGreybackOffset = 7;
void GetUnsignedLeb128Encode(std::vector<uint8_t> &dest, uint32_t value)
{
    bool done = false;
    do {
        uint8_t byte = value & 0x7f;
        value >>= kGreybackOffset;
        done = (value == 0);
        if (!done) {
            byte |= 0x80;
        }
        dest.push_back(byte);
    } while (!done);
}

uint32_t GetUnsignedLeb128Decode(const uint8_t **data)
{
    DEBUG_ASSERT(data != nullptr && "data in GetUnsignedLeb128Decode() is nullptr");
    const uint8_t *ptr = *data;
    uint32_t result = 0;
    uint32_t shift = 0;
    uint8_t byte = 0;
    while (true) {
        byte = *(ptr++);
        result |= (byte & 0x7f) << shift;
        if ((byte & 0x80) == 0) {
            break;
        }
        shift += kGreybackOffset;
    }
    *data = ptr;
    return result;
}

uint64_t GetLEB128Encode(int64_t val, bool isUnsigned)
{
    uint64_t res = 0;
    uint8_t byte = 0;
    uint8_t count = 0;
    bool done = false;
    do {
        byte = static_cast<uint64_t>(val) & 0x7f;
        val >>= kGreybackOffset;  // intended signed shift: block codedex here
        done = (isUnsigned ? val == 0 : (val == 0 || val == -1));
        if (!done) {
            byte |= 0x80;
        }
        res |= (static_cast<uint64_t>(byte) << (count++ << 3));  // each byte need 8 bit (left shift 3)
    } while (!done);
    return res;
}

uint64_t GetUleb128Encode(uint64_t val)
{
    return GetLEB128Encode(int64_t(val), true);
}

uint64_t GetSleb128Encode(int64_t val)
{
    return GetLEB128Encode(val, false);
}

uint64_t GetUleb128Decode(uint64_t val)
{
    return val;
}

int64_t GetSleb128Decode(uint64_t val)
{
    return static_cast<int64_t>(val);
}

size_t GetUleb128Size(uint64_t v)
{
    DEBUG_ASSERT(v && "if v == 0, __builtin_clzll(v) is not defined");
    size_t clz = static_cast<size_t>(__builtin_clzll(v));
    // num of 7-bit groups, (64 - clz + 6) / 7
    return size_t((64 - clz + 6) / 7);
}

size_t GetSleb128Size(int32_t v)
{
    size_t size = 0;
    int rem = v >> kGreybackOffset;
    bool hasMore = true;
    int end = ((v >= 0) ? 0 : -1);

    while (hasMore) {
        hasMore = (rem != end) || ((rem & 1) != ((v >> k64BitShift) & 1));  // judege whether has More valid rem
        size++;
        v = rem;
        rem >>= static_cast<int>(kGreybackOffset);  // intended signed shift: block codedex here
    }
    return size;
}
}  // namespace namemangler
