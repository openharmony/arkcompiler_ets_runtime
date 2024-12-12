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

#include "ecmascript/regexp/regexp_parser.h"

#include "ecmascript/base/string_helper.h"
#include "libpandabase/utils/utils.h"
#define _NO_DEBUG_

namespace panda::ecmascript {
static constexpr uint32_t CACHE_SIZE = 128;
static constexpr uint32_t CHAR_MAXS = 128;
static constexpr uint32_t ID_START_TABLE_ASCII[4] = {
    /* $ A-Z _ a-z */
    0x00000000, 0x00000010, 0x87FFFFFE, 0x07FFFFFE
};
static RangeSet g_rangeD(0x30, 0x39);  // NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static RangeSet g_rangeS({
    std::pair<uint32_t, uint32_t>(0x0009, 0x000D),  // NOLINTNEXTLINE(readability-magic-numbers)
    std::pair<uint32_t, uint32_t>(0x0020, 0x0020),  // NOLINTNEXTLINE(readability-magic-numbers)
    std::pair<uint32_t, uint32_t>(0x00A0, 0x00A0),  // NOLINTNEXTLINE(readability-magic-numbers)
    std::pair<uint32_t, uint32_t>(0x1680, 0x1680),  // NOLINTNEXTLINE(readability-magic-numbers)
    std::pair<uint32_t, uint32_t>(0x2000, 0x200A),  // NOLINTNEXTLINE(readability-magic-numbers)
    /* 2028;LINE SEPARATOR;Zl;0;WS;;;;;N;;;;; */
    /* 2029;PARAGRAPH SEPARATOR;Zp;0;B;;;;;N;;;;; */
    std::pair<uint32_t, uint32_t>(0x2028, 0x2029),  // NOLINTNEXTLINE(readability-magic-numbers)
    std::pair<uint32_t, uint32_t>(0x202F, 0x202F),  // NOLINTNEXTLINE(readability-magic-numbers)
    std::pair<uint32_t, uint32_t>(0x205F, 0x205F),  // NOLINTNEXTLINE(readability-magic-numbers)
    std::pair<uint32_t, uint32_t>(0x3000, 0x3000),  // NOLINTNEXTLINE(readability-magic-numbers)
    /* FEFF;ZERO WIDTH NO-BREAK SPACE;Cf;0;BN;;;;;N;BYTE ORDER MARK;;;; */
    std::pair<uint32_t, uint32_t>(0xFEFF, 0xFEFF),  // NOLINTNEXTLINE(readability-magic-numbers)
});

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static RangeSet g_rangeW({
    std::pair<uint32_t, uint32_t>(0x0030, 0x0039),  // NOLINTNEXTLINE(readability-magic-numbers)
    std::pair<uint32_t, uint32_t>(0x0041, 0x005A),  // NOLINTNEXTLINE(readability-magic-numbers)
    std::pair<uint32_t, uint32_t>(0x005F, 0x005F),  // NOLINTNEXTLINE(readability-magic-numbers)
    std::pair<uint32_t, uint32_t>(0x0061, 0x007A),  // NOLINTNEXTLINE(readability-magic-numbers)
});

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static RangeSet g_regexpIdentifyStart({
    std::pair<uint32_t, uint32_t>(0x0024, 0x0024),  // NOLINTNEXTLINE(readability-magic-numbers)
    std::pair<uint32_t, uint32_t>(0x0041, 0x005A),  // NOLINTNEXTLINE(readability-magic-numbers)
    std::pair<uint32_t, uint32_t>(0x0061, 0x007A),  // NOLINTNEXTLINE(readability-magic-numbers)
});

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
static RangeSet g_regexpIdentifyContinue({
    std::pair<uint32_t, uint32_t>(0x0024, 0x0024),  // NOLINTNEXTLINE(readability-magic-numbers)
    std::pair<uint32_t, uint32_t>(0x0030, 0x0039),  // NOLINTNEXTLINE(readability-magic-numbers)
    std::pair<uint32_t, uint32_t>(0x0041, 0x005A),  // NOLINTNEXTLINE(readability-magic-numbers)
    std::pair<uint32_t, uint32_t>(0x0061, 0x007A),  // NOLINTNEXTLINE(readability-magic-numbers)
});

void RegExpParser::Parse()
{
    // dynbuffer head init [size,capture_count,statck_count,flags,prefilter]
    buffer_.EmitU32(0);
    buffer_.EmitU32(0);
    buffer_.EmitU32(0);
    buffer_.EmitU32(0);
    buffer_.EmitU32(0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    PrintF("Parse Pattern------\n");
    // Pattern[U, N]::
    //      Disjunction[?U, ?N]
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    Advance();
    SaveStartOpCode saveStartOp;
    int captureIndex = captureCount_++;
    saveStartOp.EmitOpCode(&buffer_, captureIndex);
    ParseDisjunction(false);
    if (isError_) {
        return;
    }
    if (c0_ != KEY_EOF) {
        ParseError("extraneous characters at the end");
        return;
    }
    SaveEndOpCode saveEndOp;
    saveEndOp.EmitOpCode(&buffer_, captureIndex);
    MatchEndOpCode matchEndOp;
    matchEndOp.EmitOpCode(&buffer_, 0);

    uint32_t ptr = RegExpParser::OP_START_OFFSET;
    ptr += static_cast<uint32_t>(RegExpOpCode::GetRegExpOpCode(RegExpOpCode::OP_SAVE_START)->GetSize());
    uint8_t opCode = buffer_.GetU8(ptr);
    uint16_t expectedChar = 0;
    if (opCode == RegExpOpCode::OP_CHAR && !IsIgnoreCase()) {
        expectedChar = buffer_.GetU16(ptr + 1);
        if (expectedChar > UINT8_MAX) {
            expectedChar = 0;
        }
    }
    
    // dynbuffer head assignments
    buffer_.PutU32(0, buffer_.size_);
    buffer_.PutU32(NUM_CAPTURE__OFFSET, captureCount_);
    buffer_.PutU32(NUM_STACK_OFFSET, stackCount_);
    buffer_.PutU32(FLAGS_OFFSET, flags_);
    buffer_.PutU32(PREFILTER_OFFSET, expectedChar);
#ifndef _NO_DEBUG_
    RegExpOpCode::DumpRegExpOpCode(std::cout, buffer_, buffer_.GetSize());
#endif
}

void RegExpParser::ParseDisjunction(bool isBackward)
{
    // check stack overflow because infinite recursion may occur
    DoParserStackOverflowCheck("invalid regular expression.");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    PrintF("Parse Disjunction------\n");
    if (c0_ == ')') {
        isEmpty_ = true;
        return;
    }
    size_t start = buffer_.size_;
    ParseAlternative(isBackward);
    if (isError_) {
        return;
    }
    uint32_t para = RegExpOpCode::INVALID_PARA;
    do {
        if (c0_ == '|') {
            SplitNextOpCode splitOp;
            uint32_t len = buffer_.size_ - start;
            GotoOpCode gotoOp;
            splitOp.InsertOpCode(&buffer_, start, len + gotoOp.GetSize());
            uint32_t pos = gotoOp.EmitOpCode(&buffer_, 0) - gotoOp.GetSize();
            gotoOp.UpdateOpPara(&buffer_, pos, para);
            Advance();
            ParseAlternative(isBackward);
            para = buffer_.size_ - pos - gotoOp.GetSize();
            if (c0_ != '|') {
                uint16_t cnt = 0;
                uint32_t opCharSize =
                    static_cast<uint32_t>(RegExpOpCode::GetRegExpOpCode(RegExpOpCode::OP_CHAR)->GetSize());
                uint32_t opSplitSize =
                    static_cast<uint32_t>(RegExpOpCode::GetRegExpOpCode(RegExpOpCode::OP_SPLIT_NEXT)->GetSize());
                std::vector<uint16_t> chars;
                std::vector<uint32_t> offsets;
                std::set<uint16_t> checkSet;
                uint32_t ptr = start;
                bool isSparseable = true;
                do {
                    uint8_t opCode = buffer_.GetU8(ptr);
                    uint32_t offset = 0;
                    uint32_t branch = ptr;
                    bool isLastBranch = false;
                    if (opCode == RegExpOpCode::OP_SPLIT_NEXT) {
                        offset = buffer_.GetU32(ptr + 1);
                        branch = ptr + offset + opSplitSize;
                    } else {
                        isLastBranch = true;
                    }
                    uint8_t opCodeChar = buffer_.GetU8(branch);
                    if (opCodeChar == RegExpOpCode::OP_CHAR) {
                        chars.push_back(buffer_.GetU16(branch + 1));
                        offsets.push_back(offset);
                        if (checkSet.find(chars[cnt]) != checkSet.end()) {
                            isSparseable = false;
                            break;
                        }
                        checkSet.insert(chars[cnt]);
                    } else {
                        isSparseable = false;
                        break;
                    }
                    cnt++;
                    if (isLastBranch) {
                        break;
                    }
                    ptr += opSplitSize;
                } while (true);

                if (isSparseable) {
                    uint32_t sparseLen = SPARSE_HEAD_OFFSET + static_cast<uint32_t>(cnt) * SPARSE_MAX_OFFSET;
                    uint32_t splitsLen = static_cast<uint32_t>(cnt - 1) * opSplitSize;
                    ptr = start;
                    buffer_.Insert(start, sparseLen - splitsLen);
                    pos += sparseLen - splitsLen;
                    buffer_.PutU8(ptr, RegExpOpCode::OP_SPARSE);
                    buffer_.PutU16(ptr + 1, cnt);
                    ptr += SPARSE_HEAD_OFFSET;
                    ASSERT(chars.size() > 0);
                    for (int32_t i = static_cast<int32_t>(chars.size() - 1); i >= 0; i--) {
                        buffer_.PutU16(ptr, chars[i]);
                        // 2: cnt = count of splits + 1, for invert index should be extra - 1, so -1-1=-2
                        offsets[i] += opCharSize - opSplitSize * std::max(0, cnt - i -2);
                        buffer_.PutU32(ptr + SPARSE_OFF_OFFSET, offsets[i]);
                        ptr += SPARSE_MAX_OFFSET;
                    }
                }
                bool isEnd = false;
                do {
                    uint32_t paraTmp = buffer_.GetU32(pos + 1);
                    if (paraTmp == RegExpOpCode::INVALID_PARA) {
                        isEnd = true;
                    }
                    buffer_.PutU32(pos + 1, para);
                    para += paraTmp + gotoOp.GetSize();
                    pos -= paraTmp + gotoOp.GetSize();
                } while (!isEnd);
            }
            if (isError_) {
                return;
            }
        }
    } while (c0_ != KEY_EOF && c0_ != ')');
}

uint32_t RegExpParser::ParseOctalLiteral()
{
    // For compatibility with some other browsers (not all), we parse
    // up to three octal digits with a value below 256.
    // ES#prod-annexB-LegacyOctalEscapeSequence
    uint32_t value = c0_ - '0';
    Advance();
    if (c0_ >= '0' && c0_ <= '7') {
        value = value * OCTAL_VALUE + c0_ - '0';
        Advance();
        if (value < OCTAL_VALUE_RANGE && c0_ >= '0' && c0_ <= '7') {
            value = value * OCTAL_VALUE + c0_ - '0';
            Advance();
        }
    }
    return value;
}

bool RegExpParser::ParseUnlimitedLengthHexNumber(uint32_t maxValue, uint32_t *value)
{
    uint32_t x = 0;
    int d = static_cast<int>(HexValue(c0_));
    if (d < 0) {
        return false;
    }
    while (d >= 0) {
        if (UNLIKELY(x > (std::numeric_limits<uint32_t>::max() - static_cast<uint32_t>(d)) / HEX_VALUE)) {
            LOG_FULL(FATAL) << "value overflow";
            return false;
        }
        x = x * HEX_VALUE + static_cast<uint32_t>(d);
        if (x > maxValue) {
            return false;
        }
        Advance();
        d = static_cast<int>(HexValue(c0_));
    }
    *value = x;
    return true;
}

// This parses RegExpUnicodeEscapeSequence as described in ECMA262.
bool RegExpParser::ParseUnicodeEscape(uint32_t *value)
{
    // Accept both \uxxxx and \u{xxxxxx} (if allowed).
    // In the latter case, the number of hex digits between { } is arbitrary.
    // \ and u have already been read.
    if (c0_ == '{' && IsUtf16()) {
        uint8_t *start = pc_ - 1;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        Advance();
        if (ParseUnlimitedLengthHexNumber(0x10FFFF, value)) {  // NOLINTNEXTLINE(readability-magic-numbers)
            if (c0_ == '}') {
                Advance();
                return true;
            }
        }
        pc_ = start;
        Advance();
        return false;
    }
    // \u but no {, or \u{...} escapes not allowed.
    bool result = ParseHexEscape(UNICODE_HEX_VALUE, value);
    if (result && IsUtf16() && U16_IS_LEAD(*value) && c0_ == '\\') {
        // Attempt to read trail surrogate.
        uint8_t *start = pc_ - 1;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (*pc_ == 'u') {
            Advance(UNICODE_HEX_ADVANCE);
            uint32_t trail = 0;
            if (ParseHexEscape(UNICODE_HEX_VALUE, &trail) && U16_IS_TRAIL(trail)) {
                *value = U16_GET_SUPPLEMENTARY((*value), (trail));  // NOLINTNEXTLINE(hicpp-signed-bitwise)
                return true;
            }
        }
        pc_ = start;
        Advance();
    }
    return result;
}

bool RegExpParser::ParseHexEscape(int length, uint32_t *value)
{
    uint8_t *start = pc_ - 1;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    uint32_t val = 0;
    for (int i = 0; i < length; ++i) {
        uint32_t c = c0_;
        int d = static_cast<int>(HexValue(c));
        if (d < 0) {
            pc_ = start;
            Advance();
            return false;
        }
        val = val * HEX_VALUE + static_cast<uint32_t>(d);
        Advance();
    }
    *value = val;
    return true;
}

// NOLINTNEXTLINE(readability-function-size)
void RegExpParser::ParseAlternative(bool isBackward)
{
    size_t start = buffer_.size_;
    while (c0_ != '|' && c0_ != KEY_EOF && c0_ != ')') {
        if (isError_) {
            return;
        }
        size_t atomBcStart = buffer_.GetSize();
        int captureIndex = 0;
        bool isAtom = false;
        switch (c0_) {
            case '^': {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                PrintF("Assertion %c line start \n", c0_);
                LineStartOpCode lineStartOp;
                lineStartOp.EmitOpCode(&buffer_, 0);
                Advance();
                break;
            }
            case '$': {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                PrintF("Assertion %c line end \n", c0_);
                LineEndOpCode lineEndOp;
                lineEndOp.EmitOpCode(&buffer_, 0);
                Advance();
                break;
            }
            case '\\': {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                PrintF("Escape %c \n", c0_);
                Advance();
                switch (c0_) {
                    case 'b': {
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                        PrintF("Assertion %c \n", c0_);
                        WordBoundaryOpCode wordBoundaryOp;
                        wordBoundaryOp.EmitOpCode(&buffer_, 0);
                        Advance();
                        break;
                    }
                    case 'B': {
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                        PrintF("Assertion %c \n", c0_);
                        NotWordBoundaryOpCode notWordBoundaryOp;
                        notWordBoundaryOp.EmitOpCode(&buffer_, 0);
                        Advance();
                        break;
                    }
                    default: {
                        isAtom = true;
                        int atomValue = ParseAtomEscape(isBackward);
                        if (atomValue != -1) {
                            PrevOpCode prevOp;
                            if (isBackward) {
                                prevOp.EmitOpCode(&buffer_, 0);
                            }
                            if (IsIgnoreCase()) {
                                if (!IsUtf16()) {
                                    atomValue = Canonicalize(atomValue, false);
                                } else {
                                    icu::UnicodeSet set(atomValue, atomValue);
                                    set.closeOver(USET_CASE_INSENSITIVE);
                                    set.removeAllStrings();
                                    uint32_t size = static_cast<uint32_t>(set.size());
                                    RangeOpCode rangeOp;
                                    RangeSet rangeResult;
                                    for (uint32_t idx = 0; idx < size; idx++) {
                                        int32_t uc = set.charAt(idx);
                                        RangeSet curRange(uc);
                                        rangeResult.Insert(curRange);
                                    }
                                    rangeOp.InsertOpCode(&buffer_, rangeResult);
                                    break;
                                }
                            }
                            if (atomValue <= UINT16_MAX) {
                                CharOpCode charOp;
                                charOp.EmitOpCode(&buffer_, atomValue);
                            } else {
                                Char32OpCode charOp;
                                charOp.EmitOpCode(&buffer_, atomValue);
                            }
                            if (isBackward) {
                                prevOp.EmitOpCode(&buffer_, 0);
                            }
                        }
                        break;
                    }
                }
                break;
            }
            case '(': {
                Advance();
                isAtom = ParseAssertionCapture(&captureIndex, isBackward);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                Advance();
                break;
            }
            case '.': {
                PrevOpCode prevOp;
                if (isBackward) {
                    prevOp.EmitOpCode(&buffer_, 0);
                }
                if (IsDotAll()) {
                    AllOpCode allOp;
                    allOp.EmitOpCode(&buffer_, 0);
                } else {
                    DotsOpCode dotsOp;
                    dotsOp.EmitOpCode(&buffer_, 0);
                }
                if (isBackward) {
                    prevOp.EmitOpCode(&buffer_, 0);
                }
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                PrintF("Atom %c match any \n", c0_);
                isAtom = true;
                Advance();
                break;
            }
            case '[': {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                PrintF("Atom %c match range \n", c0_);
                isAtom = true;
                PrevOpCode prevOp;
                Advance();
                if (isBackward) {
                    prevOp.EmitOpCode(&buffer_, 0);
                }
                bool isInvert = false;
                if (c0_ == '^') {
                    isInvert = true;
                    Advance();
                }
                RangeSet rangeResult;
                if (!ParseClassRanges(&rangeResult)) {
                    break;
                }
                if (isInvert) {
                    rangeResult.Invert(IsUtf16());
                }
                uint32_t highValue = rangeResult.HighestValue();
                if (highValue <= UINT16_MAX) {
                    RangeOpCode rangeOp;
                    rangeOp.InsertOpCode(&buffer_, rangeResult);
                } else {
                    Range32OpCode rangeOp;
                    rangeOp.InsertOpCode(&buffer_, rangeResult);
                }

                if (isBackward) {
                    prevOp.EmitOpCode(&buffer_, 0);
                }
                break;
            }
            case '*':
            case '+':
            case '?':
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                ParseError("nothing to repeat");
                return;
            case '{': {
                uint8_t *begin = pc_ - 1;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                int dummy;
                if (ParserIntervalQuantifier(&dummy, &dummy)) {
                    ParseError("nothing to repeat");
                    return;
                }
                pc_ = begin;
                Advance();
            }
                [[fallthrough]];
            case '}':
            case ']':
                if (IsUtf16()) {
                    ParseError("syntax error");
                    return;
                }
                [[fallthrough]];
            default: {
                // PatternCharacter
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                PrintF("PatternCharacter %c\n", c0_);
                isAtom = true;
                {
                    PrevOpCode prevOp;
                    if (isBackward) {
                        prevOp.EmitOpCode(&buffer_, 0);
                    }
                    uint32_t matchedChar = c0_;
                    if (c0_ > (INT8_MAX + 1)) {
                        Prev();
                        UChar32 c;
                        int32_t length = end_ - pc_ + 1;
                        // NOLINTNEXTLINE(hicpp-signed-bitwise)
                        auto unicodeChar = base::utf_helper::ConvertUtf8ToUnicodeChar(pc_, length);
                        c = unicodeChar.first;
                        matchedChar = static_cast<uint32_t>(c);
                        pc_ += unicodeChar.second;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    }
                    if (IsIgnoreCase()) {
                        matchedChar = static_cast<uint32_t>(Canonicalize(static_cast<int>(matchedChar), IsUtf16()));
                    }
                    if (matchedChar > UINT16_MAX) {
                        Char32OpCode charOp;
                        charOp.EmitOpCode(&buffer_, matchedChar);
                    } else {
                        CharOpCode charOp;
                        charOp.EmitOpCode(&buffer_, matchedChar);
                    }
                    if (isBackward) {
                        prevOp.EmitOpCode(&buffer_, 0);
                    }
                }
                Advance();
                break;
            }
        }
        if (isAtom && !isError_) {
            ParseQuantifier(atomBcStart, captureIndex, captureCount_ - 1);
        }
        if (isBackward) {
            size_t end = buffer_.GetSize();
            size_t termSize = end - atomBcStart;
            size_t moveSize = end - start;
            buffer_.Expand(end + termSize);
            if (memmove_s(buffer_.buf_ + start +  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                              termSize,           // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                          moveSize,
                          buffer_.buf_ + start,  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                          moveSize) != EOK) {
                LOG_FULL(FATAL) << "memmove_s failed";
                UNREACHABLE();
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(buffer_.buf_ + start, termSize, buffer_.buf_ + end, termSize) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
        }
    }
}

int RegExpParser::FindGroupName(const CString &name)
{
    size_t len = 0;
    size_t nameLen = name.size();
    const char *p = reinterpret_cast<char *>(groupNames_.buf_);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char *bufEnd = reinterpret_cast<char *>(groupNames_.buf_) + groupNames_.size_;
    int captureIndex = 1;
    while (p < bufEnd) {
        len = strlen(p);
        if (len == nameLen && memcmp(name.c_str(), p, nameLen) == 0) {
            return captureIndex;
        }
        p += len + 1;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        captureIndex++;
    }
    return -1;
}

bool RegExpParser::ParseAssertionCapture(int *captureIndex, bool isBackward)
{
    bool isAtom = false;
    do {
        if (c0_ == '?') {
            Advance();
            switch (c0_) {
                // (?=Disjunction[?U, ?N])
                case '=': {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                    PrintF("Assertion(?= Disjunction)\n");
                    Advance();
                    uint32_t start = buffer_.size_;
                    ParseDisjunction(isBackward);
                    MatchOpCode matchOp;
                    matchOp.EmitOpCode(&buffer_, 0);
                    MatchAheadOpCode matchAheadOp;
                    uint32_t len = buffer_.size_ - start;
                    matchAheadOp.InsertOpCode(&buffer_, start, len);
                    break;
                }
                // (?!Disjunction[?U, ?N])
                case '!': {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                    PrintF("Assertion(?! Disjunction)\n");
                    uint32_t start = buffer_.size_;
                    Advance();
                    ParseDisjunction(isBackward);
                    MatchOpCode matchOp;
                    matchOp.EmitOpCode(&buffer_, 0);
                    NegativeMatchAheadOpCode matchAheadOp;
                    uint32_t len = buffer_.size_ - start;
                    matchAheadOp.InsertOpCode(&buffer_, start, len);
                    break;
                }
                case '<': {
                    Advance();
                    // (?<=Disjunction[?U, ?N])
                    if (c0_ == '=') {
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                        PrintF("Assertion(?<= Disjunction)\n");
                        Advance();
                        uint32_t start = buffer_.size_;
                        ParseDisjunction(true);
                        MatchOpCode matchOp;
                        matchOp.EmitOpCode(&buffer_, 0);
                        MatchAheadOpCode matchAheadOp;
                        uint32_t len = buffer_.size_ - start;
                        matchAheadOp.InsertOpCode(&buffer_, start, len);
                        // (?<!Disjunction[?U, ?N])
                    } else if (c0_ == '!') {
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                        PrintF("Assertion(?<! Disjunction)\n");
                        Advance();
                        uint32_t start = buffer_.size_;
                        ParseDisjunction(true);
                        MatchOpCode matchOp;
                        matchOp.EmitOpCode(&buffer_, 0);
                        NegativeMatchAheadOpCode matchAheadOp;
                        uint32_t len = buffer_.size_ - start;
                        matchAheadOp.InsertOpCode(&buffer_, start, len);
                    } else {
                        Prev();
                        CString name;
                        auto **pp = const_cast<const uint8_t **>(&pc_);
                        if (!ParseGroupSpecifier(pp, name)) {
                            ParseError("GroupName Syntax error.");
                            return false;
                        }
                        if (FindGroupName(name) > 0) {
                            ParseError("Duplicate GroupName error.");
                            return false;
                        }
                        groupNames_.EmitStr(name.c_str());
                        newGroupNames_.push_back(name);
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                        PrintF("group name %s", name.c_str());
                        Advance();
                        goto parseCapture;  // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
                    }
                    break;
                }
                // (?:Disjunction[?U, ?N])
                case ':':
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                    PrintF("Atom(?<: Disjunction)\n");
                    isAtom = true;
                    Advance();
                    ParseDisjunction(isBackward);
                    break;
                default:
                    Advance();
                    ParseError("? Syntax error.");
                    return false;
            }
            if (isError_) {
                return false;
            }
        } else {
            groupNames_.EmitChar(0);
        parseCapture:
            isAtom = true;
            *captureIndex = captureCount_++;
            SaveEndOpCode saveEndOp;
            SaveStartOpCode saveStartOp;
            if (isBackward) {
                saveEndOp.EmitOpCode(&buffer_, *captureIndex);
            } else {
                saveStartOp.EmitOpCode(&buffer_, *captureIndex);
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("capture start %d \n", *captureIndex);
            ParseDisjunction(isBackward);
            if (isError_) {
                return false;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("capture end %d \n", *captureIndex);
            if (isBackward) {
                saveStartOp.EmitOpCode(&buffer_, *captureIndex);
            } else {
                saveEndOp.EmitOpCode(&buffer_, *captureIndex);
            }
        }
    } while (c0_ != ')' && c0_ != KEY_EOF);
    if (c0_ != ')') {
        ParseError("capture syntax error");
        return false;
    }
    return isAtom;
}

int RegExpParser::ParseDecimalDigits()
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    PrintF("Parse DecimalDigits------\n");
    uint32_t result = 0;
    bool overflow = false;
    while (true) {
        if (c0_ < '0' || c0_ > '9') {
            break;
        }
        if (!overflow) {
            if (UNLIKELY(result > (INT32_MAX - c0_ + '0') / DECIMAL_DIGITS_ADVANCE)) {
                overflow = true;
            } else {
                result = result * DECIMAL_DIGITS_ADVANCE + c0_ - '0';
            }
        }
        Advance();
    }
    if (overflow) {
        return INT32_MAX;
    }
    return result;
}

bool RegExpParser::ParserIntervalQuantifier(int *pmin, int *pmax)
{
    // Quantifier::
    //     QuantifierPrefix
    //     QuantifierPrefix?
    // QuantifierPrefix::
    // *
    // +
    // ?
    // {DecimalDigits}
    // {DecimalDigits,}
    // {DecimalDigits,DecimalDigits}
    Advance();
    *pmin = ParseDecimalDigits();
    *pmax = *pmin;
    switch (c0_) {
        case ',': {
            Advance();
            if (c0_ == '}') {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                PrintF("QuantifierPrefix{DecimalDigits,}\n");
                *pmax = INT32_MAX;
                Advance();
            } else {
                *pmax = ParseDecimalDigits();
                if (c0_ == '}') {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                    PrintF("QuantifierPrefix{DecimalDigits,DecimalDigits}\n");
                    Advance();
                } else {
                    return false;
                }
            }
            break;
        }
        case '}':
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("QuantifierPrefix{DecimalDigits}\n");
            Advance();
            break;
        default:
            Advance();
            return false;
    }
    return true;
}

void RegExpParser::ParseQuantifier(size_t atomBcStart, int captureStart, int captureEnd)
{
    int min = -1;
    int max = -1;
    bool isGreedy = true;
    switch (c0_) {
        case '*':
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("QuantifierPrefix %c\n", c0_);
            min = 0;
            max = INT32_MAX;
            Advance();
            break;
        case '+':
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("QuantifierPrefix %c\n", c0_);
            min = 1;
            max = INT32_MAX;
            Advance();
            break;
        case '?':
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("QuantifierPrefix %c\n", c0_);
            Advance();
            min = 0;
            max = 1;
            break;
        case '{': {
            uint8_t *start = pc_ - 1;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (!ParserIntervalQuantifier(&min, &max)) {
                pc_ = start;
                Advance();  // back to '{'
                return;
            }
            if (min > max) {
                ParseError("Invalid repetition count");
                return;
            }
            break;
        }
        default:
            break;
    }
    if (c0_ == '?') {
        isGreedy = false;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        PrintF("Quantifier::QuantifierPrefix?\n");
        Advance();
    } else if (c0_ == '?' || c0_ == '+' || c0_ == '*' || c0_ == '{') {
        ParseError("nothing to repeat");
        return;
    }

    if (max == 0) {
        buffer_.size_ = atomBcStart; // Drop all unnecessary bytecode
    } else if (min != -1 && max != -1 && !isEmpty_) {
        bool isLoopOp = false;
        size_t checkCharPara = SIZE_MAX;

        if (captureStart != 0) {
            SaveResetOpCode saveResetOp;
            saveResetOp.InsertOpCode(&buffer_, atomBcStart, captureStart, captureEnd);
        }

        // zero advance check
        uint8_t firstOp = buffer_.GetU8(atomBcStart);
        if (max == INT32_MAX && firstOp != RegExpOpCode::OP_CHAR && firstOp != RegExpOpCode::OP_CHAR32 &&
                                firstOp != RegExpOpCode::OP_RANGE && firstOp != RegExpOpCode::OP_RANGE32 &&
                                firstOp != RegExpOpCode::OP_ALL && firstOp != RegExpOpCode::OP_DOTS &&
                                firstOp != RegExpOpCode::OP_SPARSE) {
            stackCount_++;
            PushCharOpCode pushCharOp;
            pushCharOp.InsertOpCode(&buffer_, atomBcStart);
            CheckCharOpCode checkCharOp;
            checkCharPara = buffer_.GetSize() + 1;
            // NOLINTNEXTLINE(readability-magic-numbers)
            checkCharOp.EmitOpCode(&buffer_, 0);
        }

        if (min <= 1 && max == INT32_MAX) {
            if (checkCharPara != SIZE_MAX) {
                buffer_.PutU32(checkCharPara, RegExpOpCode::GetRegExpOpCode(RegExpOpCode::OP_SPLIT_NEXT)->GetSize());
            }
            if (isGreedy) {
                SplitFirstOpCode splitOp;
                splitOp.EmitOpCode(&buffer_, atomBcStart - buffer_.GetSize() - splitOp.GetSize());
            } else {
                SplitNextOpCode splitOp;
                splitOp.EmitOpCode(&buffer_, atomBcStart - buffer_.GetSize() - splitOp.GetSize());
            }
        } else if (max > 1) {
            if (checkCharPara != SIZE_MAX) {
                buffer_.PutU32(checkCharPara, RegExpOpCode::GetRegExpOpCode(RegExpOpCode::OP_LOOP)->GetSize());
            }
            if (isGreedy) {
                LoopGreedyOpCode loopOp;
                loopOp.EmitOpCode(&buffer_, atomBcStart - buffer_.GetSize() - loopOp.GetSize(), min, max);
                isLoopOp = true;
            } else {
                LoopOpCode loopOp;
                loopOp.EmitOpCode(&buffer_, atomBcStart - buffer_.GetSize() - loopOp.GetSize(), min, max);
                isLoopOp = true;
            }
        }

        if (min == 0) {
            if (isGreedy) {
                SplitNextOpCode splitNextOp;
                splitNextOp.InsertOpCode(&buffer_, atomBcStart, buffer_.GetSize() - atomBcStart);
            } else {
                SplitFirstOpCode splitFirstOp;
                splitFirstOp.InsertOpCode(&buffer_, atomBcStart, buffer_.GetSize() - atomBcStart);
            }
        }
        if (isLoopOp) {
            stackCount_++;
            PushOpCode pushOp;
            pushOp.InsertOpCode(&buffer_, atomBcStart);
            PopOpCode popOp;
            popOp.EmitOpCode(&buffer_);
        }
    }
    isEmpty_ = false;
}

bool RegExpParser::ParseGroupSpecifier(const uint8_t **pp, CString &name)
{
    const uint8_t *p = *pp;
    uint32_t c = 0;
    char buffer[CACHE_SIZE] = {0};
    char *q = buffer;
    while (true) {
        if (p <= end_) {
            c = *p;
        } else {
            c = KEY_EOF;
        }
        if (c == '\\') {
            p++;
            if (*p != 'u') {
                return false;
            }
            if (!ParseUnicodeEscape(&c)) {
                return false;
            }
        } else if (c == '>') {
            break;
        } else if (c > CACHE_SIZE && c != KEY_EOF) {
            c = static_cast<uint32_t>(base::StringHelper::UnicodeFromUtf8(p, UTF8_CHAR_LEN_MAX, &p));
        } else if (c != KEY_EOF) {
            p++;
        } else {
            return false;
        }
        if (q == buffer) {
            if (!IsIdentFirst(c)) {
                return false;
            }
        } else {
            if (!u_isIDPart(c)) {
                return false;
            }
        }
        if (q != nullptr) {
            *q++ = c;
        }
    } // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    p++;
    *pp = p;
    name = buffer;
    return true;
}

int RegExpParser::ParseCaptureCount(const char *groupName)
{
    const uint8_t *p = nullptr;
    int captureIndex = 1;
    CString name;
    hasNamedCaptures_ = 0;
    for (p = base_; p < end_; p++) {  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        switch (*p) {
            case '(': {
                if (p[1] == '?') {  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    if (p[CAPTURE_CONUT_ADVANCE - 1] == '<' && p[CAPTURE_CONUT_ADVANCE] != '!' &&
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        p[CAPTURE_CONUT_ADVANCE] != '=') {
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        hasNamedCaptures_ = 1;
                        p += CAPTURE_CONUT_ADVANCE;
                        if (groupName != nullptr) {
                            if (ParseGroupSpecifier(&p, name)) {
                                if (strcmp(name.c_str(), groupName) == 0) {
                                    return captureIndex;
                                }
                            }
                        }
                        captureIndex++;
                    }
                } else {
                    captureIndex++;
                }
                break;
            }
            case '\\':
                p++;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                break;
            case '[': {
                while (p < end_ && *p != ']') {
                    if (*p == '\\') {
                        p++;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    }
                    p++;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                }
                break;
            }
            default:
                break;
        }
    }
    return captureIndex;
}

// NOLINTNEXTLINE(readability-function-size)
int RegExpParser::ParseAtomEscape(bool isBackward)
{
    // AtomEscape[U, N]::
    //     DecimalEscape
    //     CharacterClassEscape[?U]
    //     CharacterEscape[?U]
    //     [+N]kGroupName[?U]
    int result = -1;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    PrintF("Parse AtomEscape------\n");
    PrevOpCode prevOp;
    switch (c0_) {
        case KEY_EOF:
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            ParseError("unexpected end");
            break;
        // DecimalEscape
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("NonZeroDigit %c\n", c0_);
            int capture = ParseDecimalDigits();
            if (capture > captureCount_ - 1 && capture > ParseCaptureCount(nullptr) - 1) {
                ParseError("invalid backreference count");
                break;
            }
            if (isBackward) {
                BackwardBackReferenceOpCode backReferenceOp;
                backReferenceOp.EmitOpCode(&buffer_, capture);
            } else {
                BackReferenceOpCode backReferenceOp;
                backReferenceOp.EmitOpCode(&buffer_, capture);
            }
            break;
        }
        // CharacterClassEscape
        case 'd': {
            // [0-9]
            RangeOpCode rangeOp;
            if (isBackward) {
                prevOp.EmitOpCode(&buffer_, 0);
            }
            rangeOp.InsertOpCode(&buffer_, g_rangeD);
            goto parseLookBehind;
        }
        case 'D': {
            // [^0-9]
            RangeSet atomRange(g_rangeD);
            atomRange.Invert(IsUtf16());
            Range32OpCode rangeOp;
            if (isBackward) {
                prevOp.EmitOpCode(&buffer_, 0);
            }
            rangeOp.InsertOpCode(&buffer_, atomRange);
            goto parseLookBehind;
        }
        case 's': {
            // [\f\n\r\t\v]
            RangeOpCode rangeOp;
            if (isBackward) {
                prevOp.EmitOpCode(&buffer_, 0);
            }
            rangeOp.InsertOpCode(&buffer_, g_rangeS);
            goto parseLookBehind;
        }
        case 'S': {
            RangeSet atomRange(g_rangeS);
            Range32OpCode rangeOp;
            atomRange.Invert(IsUtf16());
            if (isBackward) {
                prevOp.EmitOpCode(&buffer_, 0);
            }
            rangeOp.InsertOpCode(&buffer_, atomRange);
            goto parseLookBehind;
        }
        case 'w': {
            // [A-Za-z0-9]
            RangeOpCode rangeOp;
            if (isBackward) {
                prevOp.EmitOpCode(&buffer_, 0);
            }
            rangeOp.InsertOpCode(&buffer_, g_rangeW);
            goto parseLookBehind;
        }
        case 'W': {
            // [^A-Za-z0-9]
            RangeSet atomRange(g_rangeW);
            atomRange.Invert(IsUtf16());
            Range32OpCode rangeOp;
            if (isBackward) {
                prevOp.EmitOpCode(&buffer_, 0);
            }
            rangeOp.InsertOpCode(&buffer_, atomRange);
            goto parseLookBehind;
        }
        case 'P':
        case 'p': {
            //CharacterClassStrings
            RangeSet atomRange;
            Range32OpCode rangeOp;
            ParseClassEscape(&atomRange);
            if (isBackward) {
                prevOp.EmitOpCode(&buffer_, 0);
            }
            rangeOp.InsertOpCode(&buffer_, atomRange);
            break;
        }
        // [+N]kGroupName[?U]
        case 'k': {
            Advance();
            if (c0_ != '<') {
                if (!IsUtf16() || HasNamedCaptures()) {
                    ParseError("expecting group name.");
                    break;
                }
            }
            Advance();
            Prev();
            CString name;
            auto **pp = const_cast<const uint8_t **>(&pc_);
            if (!ParseGroupSpecifier(pp, name)) {
                ParseError("GroupName Syntax error.");
                break;
            }
            int postion = FindGroupName(name);
            if (postion < 0) {
                postion = ParseCaptureCount(name.c_str());
                if (postion < 0 && (!IsUtf16() || HasNamedCaptures())) {
                    ParseError("group name not defined");
                    break;
                }
            }
            if (isBackward) {
                BackwardBackReferenceOpCode backReferenceOp;
                backReferenceOp.EmitOpCode(&buffer_, postion);
            } else {
                BackReferenceOpCode backReferenceOp;
                backReferenceOp.EmitOpCode(&buffer_, postion);
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            Advance();
            break;
        }
        parseLookBehind: {
            if (isBackward) {
                prevOp.EmitOpCode(&buffer_, 0);
            }
            Advance();
            break;
        }
        default:
            result = ParseCharacterEscape();
            break;
    }
    return result;
}

int RegExpParser::RecountCaptures()
{
    if (totalCaptureCount_ < 0) {
        const char *name = reinterpret_cast<const char*>(groupNames_.buf_);
        totalCaptureCount_ = ParseCaptureCount(name);
    }
    return totalCaptureCount_;
}
bool RegExpParser::HasNamedCaptures()
{
    if (hasNamedCaptures_ < 0) {
        RecountCaptures();
    }
    return false;
}

int RegExpParser::ParseCharacterEscape()
{
    // CharacterEscape[U]::
    //     ControlEscape
    //     c ControlLetter
    //     0 [lookahead ? DecimalDigit]
    //     HexEscapeSequence
    //     RegExpUnicodeEscapeSequence[?U]
    //     IdentityEscape[?U]
    uint32_t result = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    switch (c0_) {
        // ControlEscape
        case 'f':
            result = '\f';
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("ControlEscape %c\n", c0_);
            Advance();
            break;
        case 'n':
            result = '\n';
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("ControlEscape %c\n", c0_);
            Advance();
            break;
        case 'r':
            result = '\r';
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("ControlEscape %c\n", c0_);
            Advance();
            break;
        case 't':
            result = '\t';
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("ControlEscape %c\n", c0_);
            Advance();
            break;
        case 'v':
            result = '\v';
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("ControlEscape %c\n", c0_);
            Advance();
            break;
        // c ControlLetter
        case 'c': {
            Advance();
            if ((c0_ >= 'A' && c0_ <= 'Z') || (c0_ >= 'a' && c0_ <= 'z')) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                PrintF("ControlLetter %c\n", c0_);
                result = static_cast<uint32_t>(c0_) & 0x1f;  // NOLINTNEXTLINE(readability-magic-numbers)
                Advance();
            } else {
                if (!IsUtf16()) {
                    pc_--;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    result = '\\';
                } else {
                    ParseError("Invalid control letter");
                    return -1;
                }
            }
            break;
        }
        case '0': {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("CharacterEscape 0 [lookahead ? DecimalDigit]\n");
            if (IsUtf16() && !(*pc_ >= '0' && *pc_ <= '9')) {  // NOLINTNEXTLINE(readability-magic-numbers)
                Advance();
                result = 0;
                break;
            }
            [[fallthrough]];
        }
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7': {
            if (IsUtf16()) {
                // With /u, decimal escape is not interpreted as octal character code.
                ParseError("Invalid class escape");
                return 0;
            }
            result = ParseOctalLiteral();
            break;
        }
        // ParseHexEscapeSequence
        // ParseRegExpUnicodeEscapeSequence
        case 'x': {
            Advance();
            if (ParseHexEscape(UNICODE_HEX_ADVANCE, &result)) {
                return result;
            }
            if (IsUtf16()) {
                ParseError("Invalid class escape");
                return -1;
            }
            result = 'x';
            break;
        }
        case 'u': {
            Advance();
            if (ParseUnicodeEscape(&result)) {
                return result;
            }
            if (IsUtf16()) {
                // With /u, invalid escapes are not treated as identity escapes.
                ParseError("Invalid unicode escape");
                return 0;
            }
            // If \u is not followed by a two-digit hexadecimal, treat it
            // as an identity escape.
            result = 'u';
            break;
        }
        // IdentityEscape[?U]
        case '$':
        case '(':
        case ')':
        case '*':
        case '+':
        case '.':
        case '/':
        case '?':
        case '[':
        case '\\':
        case ']':
        case '^':
        case '{':
        case '|':
        case '}':
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("IdentityEscape %c\n", c0_);
            result = c0_;
            Advance();
            break;
        default: {
            if (IsUtf16()) {
                ParseError("Invalid unicode escape");
                return 0;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("SourceCharacter %c\n", c0_);
            result = c0_;
            if (result < CHAR_MAXS) {
                Advance();
            } else {
                Prev();
                const uint8_t *p = pc_;
                result = static_cast<uint32_t>(base::StringHelper::UnicodeFromUtf8(p, UTF8_CHAR_LEN_MAX, &p));
                int offset = static_cast<int>(p - pc_);
                Advance(offset + 1);
            }
            break;
        }
    }
    return static_cast<int>(result);
}

bool RegExpParser::ParseClassRanges(RangeSet *result)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    PrintF("Parse ClassRanges------\n");
    while (c0_ != ']') {
        RangeSet s1;
        bool needInter = false;
        uint32_t c1 = ParseClassAtom(&s1);
        if (c1 == UINT32_MAX) {
            ParseError("invalid class range");
            return false;
        }
        needInter = NeedIntersection(c1);
        int next_c0 = *pc_;
        if (c0_ == '-' && next_c0 != ']') {
            if (c1 == CLASS_RANGE_BASE) {
                if (IsUtf16()) {
                    ParseError("invalid class range");
                    return false;
                }
                result->Insert(s1);
                continue;
            }
            Advance();
            RangeSet s2;
            uint32_t c2 = ParseClassAtom(&s2);
            if (c2 == UINT32_MAX) {
                ParseError("invalid class range");
                return false;
            }
            if (c2 == CLASS_RANGE_BASE) {
                if (IsUtf16()) {
                    ParseError("invalid class range");
                    return false;
                }
                result->Insert(s2);
                continue;
            }
            if (c1 < INT8_MAX) {
                if (c1 > c2) {
                    ParseError("invalid class range");
                    return false;
                }
            }
            needInter = NeedIntersection(c2);
            result->Insert(c1, c2);
            if (IsIgnoreCase() && needInter) {
                ProcessIntersection(result);
            }
        } else {
            result->Insert(s1);
            if (!(IsIgnoreCase() && needInter)) {
                continue;
            }
            if (c1 <= 'z' && c1 >= 'a') {
                result->Insert(RangeSet(c1 - 'a' + 'A'));
            } else {
                result->Insert(RangeSet(c1 - 'A' + 'a'));
            }
        }
    }
    Advance();
    return true;
}

uint32_t RegExpParser::ParseClassAtom(RangeSet *atom)
{
    uint32_t ret = UINT32_MAX;
    switch (c0_) {
        case '\\': {
            Advance();
            ret = static_cast<uint32_t>(ParseClassEscape(atom));
            break;
        }
        case KEY_EOF:
            break;
        case 0: {
            if (pc_ >= end_) {
                return UINT32_MAX;
            }
            [[fallthrough]];
        }
        default: {
            uint32_t value = c0_;
            size_t u16_size = 0;
            if (c0_ > INT8_MAX) {  // NOLINTNEXTLINE(readability-magic-numbers)
                pc_ -= 1;          // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                auto u16_result = base::utf_helper::ConvertUtf8ToUtf16Pair(pc_, true);
                value = u16_result.first;
                u16_size = u16_result.second;
                Advance(u16_size + 1);
            } else {
                Advance();
            }
            atom->Insert(RangeSet(value));
            ret = value;
            break;
        }
    }
    return ret;
}

int RegExpParser::ParseClassEscape(RangeSet *atom)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    PrintF("Parse ClassEscape------\n");
    int result = -1;
    switch (c0_) {
        case 'b':
            Advance();
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("ClassEscape %c", 'b');
            result = '\b';
            atom->Insert(RangeSet(static_cast<uint32_t>('\b')));
            break;
        case '-':
            Advance();
            result = '-';
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("ClassEscape %c", '-');
            atom->Insert(RangeSet(static_cast<uint32_t>('-')));
            break;
        // CharacterClassEscape
        case 'd':
        case 'D':
            result = CLASS_RANGE_BASE;
            atom->Insert(g_rangeD);
            if (c0_ == 'D') {
                atom->Invert(IsUtf16());
            }
            Advance();
            break;
        case 's':
        case 'S':
            result = CLASS_RANGE_BASE;
            atom->Insert(g_rangeS);
            if (c0_ == 'S') {
                atom->Invert(IsUtf16());
            }
            Advance();
            break;
        case 'w':
        case 'W':
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            PrintF("ClassEscape::CharacterClassEscape %c\n", c0_);
            result = CLASS_RANGE_BASE;
            atom->Insert(g_rangeW);
            if (c0_ == 'W') {
                atom->Invert(IsUtf16());
            }
            Advance();
            break;
        case 'P':
        case 'p': {
            bool negate = (c0_ == 'P');
            CString propertyName;
            CString valueName;
            if (!ParseUnicodePropertyValueCharacters(propertyName, valueName) ||
                !ParseUnicodePropertyClassRange(propertyName, valueName, atom, negate)) {
                char *originExpression = (char *)base_;
                CString expression(originExpression);
                CString msg = "Invalid regular expression :" + expression;
                ParseError(msg.c_str());
            }
            result = CLASS_RANGE_BASE;
            break;
        }
        default:
            result = ParseCharacterEscape();
            int value = result;
            if (IsIgnoreCase()) {
                value = Canonicalize(value, IsUtf16());
            }
            atom->Insert(RangeSet(static_cast<uint32_t>(value)));
            break;
    }
    return result;
}

bool RegExpParser::ParseUnicodePropertyValueCharacters(CString &propertyName, CString &valueName)
{
    Advance();
    if (c0_ == '{') {
        if (!GetUnicodePropertyName(propertyName)) {
            return false;
        }

        if (!GetUnicodePropertyValueName(valueName)) {
            return false;
        }
    } else {
        return false;
    }
    Advance();
    return true;
}

bool RegExpParser::GetUnicodePropertyName(CString &propertyName)
{
    Advance();
    while (c0_ != '}' && c0_ != '=') {
        if (IsUnicodePropertyValueCharacter(c0_)) {
            propertyName += c0_;
        } else {
            return false;
        }
        Advance();
    }
    return true;
}

bool RegExpParser::GetUnicodePropertyValueName(CString &valueName)
{
    if (c0_ == '=') {
        Advance();
        while (c0_ != '}') {
            if (IsUnicodePropertyValueCharacter(c0_)) {
                valueName += c0_;
            } else {
                return false;
            }
            Advance();
        }
    }
    return true;
}

// NOLINTNEXTLINE(cert-dcl50-cpp)
void RegExpParser::PrintF(const char *fmt, ...)
{
#ifndef _NO_DEBUG_
    va_list args;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,)
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#else
    (void)fmt;
#endif
}

void RegExpParser::ParseError(const char *errorMessage)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    PrintF("error: ");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    PrintF(errorMessage);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    PrintF("\n");
    SetIsError();
    size_t length = strlen(errorMessage) + 1;
    if (memcpy_s(errorMsg_, length, errorMessage, length) != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
}

int RegExpParser::IsIdentFirst(uint32_t c)
{
    if (c < CACHE_SIZE) {
        return (ID_START_TABLE_ASCII[c >> 5] >> (c & 31)) & 1; // 5: Shift five bits 31: and operation binary of 31
    } else {
        auto uchar = static_cast<UChar32>(c);
        return static_cast<int>(u_isIDStart(uchar));
    }
}

int RegExpParser::Canonicalize(int c, bool isUnicode)
{
    if (c < TMP_BUF_SIZE) {  // NOLINTNEXTLINE(readability-magic-numbers)
        if (c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        }
    } else {
        int cur = c;
        if (isUnicode) {
            c = u_tolower(static_cast<UChar32>(c));
            if (c >= 'a' && c <= 'z') {
                c = cur;
            }
        } else {
            c = u_toupper(static_cast<UChar32>(c));
            if (c >= 'A' && c <= 'Z') {
                c = cur;
            }
        }
    }
    return c;
}

bool RegExpParser::NeedIntersection(uint32_t c)
{
    return (c <= 'z' && c >= 'a') || (c <= 'Z' && c >= 'A');
}

void RegExpParser::DoParserStackOverflowCheck(const char *errorMessage)
{
    if (UNLIKELY(thread_->GetCurrentStackPosition() < thread_->GetStackLimit())) {
        LOG_ECMA(ERROR) << "Stack overflow! current:" << thread_->GetCurrentStackPosition() <<
            " limit:" << thread_->GetStackLimit();
        ParseError(errorMessage);
        return;
    }
}

bool RegExpParser::ParseUnicodePropertyClassRange(CString &propertyName, CString &valueName,
                                                  RangeSet *atom, bool negate)
{
    const char *name = propertyName.c_str();
    if (valueName.size() == 0) {
        if (MatchUnicodeProperty(UCHAR_GENERAL_CATEGORY_MASK, name, atom, negate)) {
            return true;
        }
        if (MatchSepcialUnicodeProperty(propertyName, negate, atom)) {
            return true;
        }
        UProperty property = u_getPropertyEnum(name);
        if (!IsSupportedBinaryProperty(property)) {
            return false;
        }
        if (!IsExactPropertyAlias(name, property)) {
            return false;
        }
        if (negate && IsBinaryPropertyOfStrings(property)) {
            return false;
        }
        return MatchUnicodeProperty(property, negate ? "N" : "Y", atom, false);
    } else {
        UProperty property = u_getPropertyEnum(propertyName.c_str());
        if (property == UCHAR_GENERAL_CATEGORY) {
            property = UCHAR_GENERAL_CATEGORY_MASK;
        } else if (property != UCHAR_SCRIPT && property != UCHAR_SCRIPT_EXTENSIONS) {
            return false;
        }
        return MatchUnicodeProperty(property, valueName.c_str(), atom, negate);
    }
}

bool RegExpParser::MatchUnicodeProperty(UProperty property, const char* propertyName, RangeSet *atom, bool negate)
{
    UProperty propertyForMatch = property;
    if (propertyForMatch == UCHAR_SCRIPT_EXTENSIONS) {
        propertyForMatch = UCHAR_SCRIPT;
    }
    int32_t propertyValue = u_getPropertyValueEnum(propertyForMatch, propertyName);
    if (propertyValue == UCHAR_INVALID_CODE) {
        return false;
    }
    if (!IsExactPropertyValueAlis(propertyName, propertyForMatch, propertyValue)) {
        return false;
    }
    UErrorCode ec = U_ZERO_ERROR;
    icu::UnicodeSet set;
    set.applyIntPropertyValue(property, propertyValue, ec);
    bool success = ec == U_ZERO_ERROR && !set.isEmpty();
    if (success) {
        const bool caseFolding = IsIgnoreCase();
        if (negate) {
            set.complement();
        }
        if (caseFolding) {
            set.closeOver(USET_CASE_INSENSITIVE);
        }
        set.removeAllStrings();
        for (int i = 0; i < set.getRangeCount(); i++) {
            atom->Insert(set.getRangeStart(i),  set.getRangeEnd(i));
        }
    }
    return success;
}

bool RegExpParser::IsExactPropertyValueAlis(const char *valueName, UProperty property, int32_t propertyValue)
{
    const char *shortName = u_getPropertyValueName(property, propertyValue, U_SHORT_PROPERTY_NAME);
    if (shortName != nullptr && strcmp(valueName, shortName) == 0) {
        return true;
    }
    int i = 0;
    bool flag = true;
    while (flag) {
        const char *longName = u_getPropertyValueName(property, propertyValue,
            static_cast<UPropertyNameChoice>(U_LONG_PROPERTY_NAME + i));
        if (longName == nullptr) {
            flag = false;
            break;
        }
        if (strcmp(valueName, longName) == 0) {
            return true;
        }
        i++;
    }
    return false;
}

bool RegExpParser::IsExactPropertyAlias(const char* propertyName, UProperty property)
{
    const char* shortName = u_getPropertyName(property, U_SHORT_PROPERTY_NAME);
    if (shortName != nullptr && strcmp(propertyName, shortName) == 0) {
        return true;
    }
    int i = 0;
    bool flag = true;
    while (flag) {
        const char* longName = u_getPropertyName(property,
            static_cast<UPropertyNameChoice>(U_LONG_PROPERTY_NAME + i));
        if (longName == nullptr) {
            flag = false;
            break;
        }
        if (strcmp(propertyName, longName) == 0) {
            return true;
        }
        i++;
    }
    return false;
}

bool RegExpParser::MatchSepcialUnicodeProperty(CString &name, bool negate, RangeSet *atom)
{
    if (name == "Any") {
        if (!negate) {
            atom->Insert(0, 0x10FFFF);
        }
    } else if (name == "ASCII") {
        if (negate) {
            atom->Insert(0x80, 0x10FFFF);
        } else {
            atom->Insert(0x0, 0x7F);
        }
    } else if (name == "Assigned") {
        return MatchUnicodeProperty(UCHAR_GENERAL_CATEGORY, "Unassigned", atom, !negate);
    } else {
        return false;
    }
    return true;
}

bool RegExpParser::IsSupportedBinaryProperty(UProperty property)
{
    switch (property) {
        case UCHAR_ALPHABETIC:
        case UCHAR_ASCII_HEX_DIGIT:
        case UCHAR_BIDI_CONTROL:
        case UCHAR_BIDI_MIRRORED:
        case UCHAR_DASH:
        case UCHAR_DEFAULT_IGNORABLE_CODE_POINT:
        case UCHAR_DEPRECATED:
        case UCHAR_DIACRITIC:
        case UCHAR_JOIN_CONTROL:
        case UCHAR_IDS_TRINARY_OPERATOR:
        case UCHAR_IDS_BINARY_OPERATOR:
        case UCHAR_IDEOGRAPHIC:
        case UCHAR_S_TERM:
        case UCHAR_ID_START:
        case UCHAR_ID_CONTINUE:
        case UCHAR_HEX_DIGIT:
        case UCHAR_GRAPHEME_EXTEND:
        case UCHAR_GRAPHEME_BASE:
        case UCHAR_EXTENDER:
        case UCHAR_LOGICAL_ORDER_EXCEPTION:
        case UCHAR_LOWERCASE:
        case UCHAR_MATH:
        case UCHAR_NONCHARACTER_CODE_POINT:
        case UCHAR_QUOTATION_MARK:
        case UCHAR_RADICAL:
        case UCHAR_SOFT_DOTTED:
        case UCHAR_TERMINAL_PUNCTUATION:
        case UCHAR_UNIFIED_IDEOGRAPH:
        case UCHAR_UPPERCASE:
        case UCHAR_WHITE_SPACE:
        case UCHAR_XID_CONTINUE:
        case UCHAR_XID_START:
        case UCHAR_VARIATION_SELECTOR:
        case UCHAR_PATTERN_SYNTAX:
        case UCHAR_PATTERN_WHITE_SPACE:
        case UCHAR_CASED:
        case UCHAR_CASE_IGNORABLE:
        case UCHAR_CHANGES_WHEN_LOWERCASED:
        case UCHAR_CHANGES_WHEN_UPPERCASED:
        case UCHAR_CHANGES_WHEN_TITLECASED:
        case UCHAR_CHANGES_WHEN_CASEFOLDED:
        case UCHAR_CHANGES_WHEN_CASEMAPPED:
        case UCHAR_CHANGES_WHEN_NFKC_CASEFOLDED:
        case UCHAR_REGIONAL_INDICATOR:
        case UCHAR_EMOJI:
        case UCHAR_EMOJI_PRESENTATION:
        case UCHAR_EMOJI_MODIFIER:
        case UCHAR_EMOJI_MODIFIER_BASE:
        case UCHAR_EMOJI_COMPONENT:
        case UCHAR_EXTENDED_PICTOGRAPHIC:
            return true;
        case UCHAR_BASIC_EMOJI:
        case UCHAR_EMOJI_KEYCAP_SEQUENCE:
        case UCHAR_RGI_EMOJI_MODIFIER_SEQUENCE:
        case UCHAR_RGI_EMOJI_FLAG_SEQUENCE:
        case UCHAR_RGI_EMOJI_TAG_SEQUENCE:
        case UCHAR_RGI_EMOJI_ZWJ_SEQUENCE:
        case UCHAR_RGI_EMOJI:
            return false;
        default:
            break;
    }
    return false;
}

bool RegExpParser::IsBinaryPropertyOfStrings(UProperty property)
{
    switch (property) {
        case UCHAR_BASIC_EMOJI:
        case UCHAR_EMOJI_KEYCAP_SEQUENCE:
        case UCHAR_RGI_EMOJI_MODIFIER_SEQUENCE:
        case UCHAR_RGI_EMOJI_FLAG_SEQUENCE:
        case UCHAR_RGI_EMOJI_TAG_SEQUENCE:
        case UCHAR_RGI_EMOJI_ZWJ_SEQUENCE:
        case UCHAR_RGI_EMOJI:
            return true;
        default:
            break;
    }
    return false;
}
}  // namespace panda::ecmascript
