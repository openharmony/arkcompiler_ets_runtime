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
#include <map>

namespace namemangler {
#ifdef __MRT_DEBUG
#define DEBUG_ASSERT(f) assert(f)
#else
#define DEBUG_ASSERT(f) ((void)0)
#endif

#define GETHEXCHAR(n) static_cast<char>((n) < 10 ? (n) + '0' : (n)-10 + 'a')
#define GETHEXCHARU(n) static_cast<char>((n) < 10 ? (n) + '0' : (n)-10 + 'A')

using StringMap = std::map<const std::string, const std::string>;

#ifdef ARK_LITECG_DEBUG
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
    namePtr = name.c_str();
    nameLen = name.length();

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
                str16 += static_cast<char16_t>(codepoint);
                unsigned int count = UTF16ToUTF8(str, str16, 1, false) >> 16; // shift 16 to get count
                if (count == 2) {  // the count of str equal 2 to 4, use array to save the utf8
                    newName[pos++] = str[0];
                    newName[pos++] = str[1];
                } else if (count == 3) {  // the count of str equal 2 to 4, deal 3 new
                    newName[pos++] = str[0];
                    newName[pos++] = str[1];
                    newName[pos++] = str[2];  // 2 is index of third char
                } else if (count == 4) {      // the count of str equal 2 to 4
                    newName[pos++] = str[0];
                    newName[pos++] = str[1];
                    newName[pos++] = str[2];  // 2 is index of third char
                    newName[pos++] = str[3];  // 3 is index of fourth char
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
// output: Lj/lang/Object;  [Lj/lang/Object;
void DecodeMapleNameToJDescriptor(const std::string &nameIn, std::string &nameOut)
{
    nameOut = DecodeName(nameIn);
    if (nameOut[0] == 'A') {
        size_t i = 0;
        while (nameOut[i] == 'A') {
            nameOut[i++] = '[';
        }
    }
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

const uint32_t kGreybackOffset = 7;

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
#endif
}  // namespace namemangler
