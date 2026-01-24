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

#ifndef ECMASCRIPT_PGO_PROFILER_PGO_UTILS_H
#define ECMASCRIPT_PGO_PROFILER_PGO_UTILS_H

#include <algorithm>
#include <iomanip>
#include <list>
#include <mutex>
#include <sstream>
#include <string>

#include "file.h"

#include "ecmascript/common.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript::pgo {
static constexpr Alignment ALIGN_SIZE = Alignment::LOG_ALIGN_4;
using PGOMethodId = panda_file::File::EntityId;
using ApEntityId = uint32_t;

// Text formatter with pure chain API for aligned output
class TextFormatter {
public:
    // Static constants
    static constexpr const char* NEW_LINE = "\n";
    static constexpr const char* PIPE = " | ";
    static constexpr const char* INDENT = "  ";

    // Label widths
    static constexpr size_t LABEL_WIDTH_LARGE = 20;
    static constexpr size_t LABEL_WIDTH_MEDIUM = 16;
    static constexpr size_t LABEL_WIDTH_SMALL = 8;

    // Table column widths
    static constexpr size_t COL_WIDTH_ABC_ID = 8;
    static constexpr size_t COL_WIDTH_CHECKSUM = 12;

    // Basic chain methods
    TextFormatter& Indent(int level = 1)
    {
        for (int i = 0; i < level; i++) {
            oss_ << INDENT;
        }
        return *this;
    }

    template<typename T>
    TextFormatter& Text(const T& text)
    {
        oss_ << text;
        return *this;
    }

    TextFormatter& NewLine()
    {
        oss_ << NEW_LINE;
        return *this;
    }

    TextFormatter& Pipe()
    {
        oss_ << PIPE;
        return *this;
    }

    // Alignment methods
    template<typename T>
    TextFormatter& Left(const T& value, size_t width)
    {
        oss_ << std::left << std::setw(static_cast<int>(width)) << value;
        return *this;
    }

    template<typename T>
    TextFormatter& Right(const T& value, size_t width)
    {
        oss_ << std::right << std::setw(static_cast<int>(width)) << value;
        return *this;
    }

    // Set label width for alignment (call before using Label)
    TextFormatter& SetLabelWidth(size_t width)
    {
        labelWidth_ = width;
        return *this;
    }

    // Label-Value pair with automatic alignment
    TextFormatter& Label(const std::string& label, bool align = false)
    {
        if (align_ || align) {
            oss_ << std::left << std::setw(static_cast<int>(labelWidth_)) << label;
        } else {
            oss_ << label;
        }
        oss_ << ":";
        return *this;
    }

    TextFormatter& LabelAlign()
    {
        align_ = true;
        return *this;
    }

    TextFormatter& LabelReset()
    {
        align_ = false;
        labelWidth_ = LABEL_WIDTH_MEDIUM;
        return *this;
    }

    template<typename T>
    TextFormatter& Value(const T& value, bool alignWithLabel = false)
    {
        oss_ << " ";
        if (alignWithLabel) {
            oss_ << std::left << std::setw(static_cast<int>(labelWidth_)) << value;
        } else {
            oss_ << value;
        }
        return *this;
    }

    TextFormatter& Hex(uint32_t value)
    {
        oss_ << " " << HexStr(value);
        return *this;
    }

    TextFormatter& Fixed(double value, int precision = 1)
    {
        oss_ << " " << std::fixed << std::setprecision(precision) << value;
        return *this;
    }

    static std::string HexStr(uint32_t value)
    {
        std::ostringstream oss;
        oss << "0x" << std::hex << value;
        return oss.str();
    }

    // Table formatting
    static constexpr size_t SECTION_LINE_WIDTH = 80;

    TextFormatter& SectionLine()
    {
        oss_ << std::string(SECTION_LINE_WIDTH, '=');
        return *this;
    }

    TextFormatter& CenteredTitle(const std::string& title)
    {
        size_t padding = (SECTION_LINE_WIDTH > title.length()) ? (SECTION_LINE_WIDTH - title.length()) / 2 : 0;
        oss_ << std::string(padding, ' ') << title;
        return *this;
    }

    // Output
    std::string Str() const
    {
        return oss_.str();
    }

    void Clear()
    {
        oss_.str("");
        oss_.clear();
    }

    std::ostringstream& GetStream()
    {
        return oss_;
    }

    TextFormatter& PushIndent(int levels = 1)
    {
        indentLevel_ += levels;
        return *this;
    }

    TextFormatter& PopIndent(int levels = 1)
    {
        indentLevel_ = std::max(0, indentLevel_ - levels);
        return *this;
    }

    TextFormatter& AutoIndent()
    {
        return Indent(indentLevel_);
    }

    TextFormatter& ResetIndent()
    {
        indentLevel_ = 0;
        return *this;
    }

    int GetIndentLevel() const
    {
        return indentLevel_;
    }

private:
    std::ostringstream oss_;
    size_t labelWidth_ {LABEL_WIDTH_MEDIUM};
    bool align_ {false};
    int indentLevel_ {0};
};

class IndentScope {
public:
    explicit IndentScope(TextFormatter& fmt): fmt_(fmt)
    {
        fmt_.PushIndent();
    }

    ~IndentScope()
    {
        fmt_.PopIndent();
    }

    NO_COPY_SEMANTIC(IndentScope);
    NO_MOVE_SEMANTIC(IndentScope);

private:
    TextFormatter& fmt_;
};

class DumpJsonUtils {
public:
    static inline const std::string ABC_FILE_POOL = "abcFilePool";
    static inline const std::string ABC_FILE = "abcFile";
    static inline const std::string RECORD_DETAIL = "recordDetail";
    static inline const std::string MODULE_NAME = "moduleName";
    static inline const std::string FUNCTION = "function";
    static inline const std::string FUNCTION_NAME = "functionName";
    static inline const std::string TYPE = "type";
    static inline const std::string TYPE_OFFSET = "typeOffset";
    static inline const std::string TYPE_NAME = "typeName";
    static inline const std::string IS_ROOT = "isRoot";
    static inline const std::string KIND = "kind";
    static inline const std::string ABC_ID = "abcId";
    static inline const std::string ID = "id";
};

class ApNameUtils {
public:
    static const std::string AP_SUFFIX;
    static const std::string RUNTIME_AP_PREFIX;
    static const std::string MERGED_AP_PREFIX;
    static const std::string DEFAULT_AP_NAME;
    static std::string PUBLIC_API GetRuntimeApName(const std::string &ohosModuleName);
    static std::string PUBLIC_API GetMergedApName(const std::string &ohosModuleName);
    static std::string PUBLIC_API GetOhosPkgApName(const std::string &ohosModuleName);

private:
    static std::string GetBriefApName(const std::string &ohosModuleName);
};

class ConcurrentGuardValue {
public:
    mutable std::atomic_int last_tid {0};
    mutable std::atomic_int count {0};

    int Gettid()
    {
        return os::thread::GetCurrentThreadId();
    }
};

class ConcurrentGuard {
private:
    std::string operation_;

public:
    ConcurrentGuard(ConcurrentGuardValue& v, std::string operation = "ConcurrentGuard")
        : operation_(operation), v_(v)
    {
        auto tid = v_.Gettid();
        auto except = 0;
        auto desired = 1;
        if (!v_.count.compare_exchange_strong(except, desired) && v_.last_tid != tid) {
            LOG_PGO(FATAL) << "[" << operation_ << "] total thead count should be 0, but get " << except
                           << ", current tid: " << tid << ", last tid: " << v_.last_tid;
        }
        v_.last_tid = tid;
    }
    ~ConcurrentGuard()
    {
        auto tid = v_.Gettid();
        auto except = 1;
        auto desired = 0;
        if (!v_.count.compare_exchange_strong(except, desired) && v_.last_tid != tid) {
            LOG_PGO(FATAL) << "[" << operation_ << "] total thead count should be 1, but get " << except
                           << ", current tid: " << tid << ", last tid: " << v_.last_tid;
        }
    };

private:
    ConcurrentGuardValue& v_;
};

class PGOProfiler;
class PGOPendingProfilers {
public:
    void PushBack(PGOProfiler* value)
    {
        WriteLockHolder lock(listMutex_);
        if (std::find(list_.begin(), list_.end(), value) == list_.end()) {
            list_.push_back(value);
        }
    }

    PGOProfiler* PopFront()
    {
        WriteLockHolder lock(listMutex_);
        if (list_.empty()) {
            return nullptr;
        }
        PGOProfiler* value = list_.front();
        list_.pop_front();
        return value;
    }

    PGOProfiler* Front() const
    {
        ReadLockHolder lock(listMutex_);
        if (list_.empty()) {
            return nullptr;
        }
        return list_.front();
    }

    void Pop()
    {
        WriteLockHolder lock(listMutex_);
        if (list_.empty()) {
            return;
        }
        list_.pop_front();
    }

    bool Empty() const
    {
        ReadLockHolder lock(listMutex_);
        return list_.empty();
    }

    size_t Size() const
    {
        ReadLockHolder lock(listMutex_);
        return list_.size();
    }

    void Remove(PGOProfiler* profiler)
    {
        WriteLockHolder lock(listMutex_);
        list_.remove(profiler);
    }

private:
    std::list<PGOProfiler*> list_;
    mutable RWLock listMutex_;
};
}  // namespace panda::ecmascript::pgo
#endif // ECMASCRIPT_PGO_PROFILER_PGO_UTILS_H