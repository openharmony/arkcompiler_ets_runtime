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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_ARKSTEED_ARKSTEED_DUMP_HELPER_H
#define ECMASCRIPT_ARKSTEED_ARKSTEED_DUMP_HELPER_H

#include <iostream>
#include "libpandabase/macros.h"

#define ANSI_COLORS(V)        \
    V(Red,     RED,     31)   \
    V(Green,   GREEN,   32)   \
    V(Yellow,  YELLOW,  33)   \
    V(Blue,    BLUE,    34)   \
    V(Magenta, MAGENTA, 35)   \
    V(Cyan,    CYAN,    36)

namespace panda::ecmascript::arksteed {
class AnsiColorScope {
public:
#define DEFINE_ANSI_COLOR_CONSTANT(Color, COLOR, code)                  \
    static constexpr const char *BRIGHT_##COLOR = "\033[" #code "m";    \
    static constexpr const char *FAINT_##COLOR = "\033[0;" #code "m";

    ANSI_COLORS(DEFINE_ANSI_COLOR_CONSTANT)
#undef DEFINE_ANSI_COLOR_CONSTANT

#define LIST_ANSI_COLOR_CONSTANT(Color, COLOR, code) BRIGHT_##COLOR,
    static constexpr const char *BRIGHT_COLORS[] = {ANSI_COLORS(LIST_ANSI_COLOR_CONSTANT)};
#undef LIST_ANSI_COLOR_CONSTANT

#define LIST_ANSI_COLOR_CONSTANT(Color, COLOR, code) FAINT_##COLOR,
    static constexpr const char *FAINT_COLORS[] = {ANSI_COLORS(LIST_ANSI_COLOR_CONSTANT)};
#undef LIST_ANSI_COLOR_CONSTANT

    static constexpr const char *RESET = "\033[0m";
    static constexpr size_t NUM_BRIGHT_COLORS = std::size(BRIGHT_COLORS);
    static constexpr size_t NUM_FAINT_COLORS = std::size(FAINT_COLORS);

    explicit AnsiColorScope(std::ostream &out, const char *header, bool enabled = true)
        : out_(out), enabled_(enabled)
    {
        ASSERT(header != nullptr);
        if (enabled) {
            out_ << header;
        }
    }
    ~AnsiColorScope()
    {
        if (enabled_) {
            out_ << RESET;
        }
    }

#define DEFINE_ANSI_COLOR_SCOPE_MAKER(Color, COLOR, code)                       \
    static AnsiColorScope Bright##Color(std::ostream &out, bool enabled = true) \
    {                                                                           \
        return AnsiColorScope(out, BRIGHT_##COLOR, enabled);                    \
    }                                                                           \
    static AnsiColorScope Faint##Color(std::ostream &out, bool enabled = true)  \
    {                                                                           \
        return AnsiColorScope(out, FAINT_##COLOR, enabled);                     \
    }
    ANSI_COLORS(DEFINE_ANSI_COLOR_SCOPE_MAKER)
#undef DEFINE_ANSI_COLOR_SCOPE_MAKER

    static AnsiColorScope NthBrightColor(std::ostream &out, size_t index, bool enabled = true)
    {
        return AnsiColorScope(out, BRIGHT_COLORS[index % NUM_BRIGHT_COLORS], enabled);
    }
    static AnsiColorScope NthFaintColor(std::ostream &out, size_t index, bool enabled = true)
    {
        return AnsiColorScope(out, FAINT_COLORS[index % NUM_FAINT_COLORS], enabled);
    }

    operator bool() const
    {
        return true;  // Used in WITH_ANSI_COLOR_SCOPE macro
    }

private:
    std::ostream &out_;
    bool enabled_;
};

#define CONCAT_ANSI_COLOR_SCOPE_IMPL(x, y) x ## y
#define CONCAT_ANSI_COLOR_SCOPE(x, y) CONCAT_ANSI_COLOR_SCOPE_IMPL(x, y)

#define WITH_ANSI_COLOR_SCOPE(scopeExpr) \
    if (AnsiColorScope CONCAT_ANSI_COLOR_SCOPE(ansiColorScope_, __LINE__) = AnsiColorScope::scopeExpr)
}

#undef ANSI_COLORS
#endif  // ECMASCRIPT_ARKSTEED_ARKSTEED_DUMP_HELPER_H
