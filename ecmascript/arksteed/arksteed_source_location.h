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

#ifndef ECMASCRIPT_ARKSTEED_ARKSTEED_SOURCE_LOCATION_H
#define ECMASCRIPT_ARKSTEED_ARKSTEED_SOURCE_LOCATION_H

#if defined(__has_builtin)
#define ARKSTEED_SUPPORTS_SOURCE_LOCATION \
    (__has_builtin(__builtin_FUNCTION) && __has_builtin(__builtin_FILE) && __has_builtin(__builtin_LINE))
#elif defined(__GNUC__) && __GNUC__ >= 7
#define ARKSTEED_SUPPORTS_SOURCE_LOCATION 1
#else
#define ARKSTEED_SUPPORTS_SOURCE_LOCATION 0
#endif

namespace panda::ecmascript::arksteed {

class SrcPosition {
public:
#if ARKSTEED_SUPPORTS_SOURCE_LOCATION
    static constexpr SrcPosition Current(const char *function = __builtin_FUNCTION(),
                                         const char *file = __builtin_FILE(), int line = __builtin_LINE())
    {
        return SrcPosition(function, file, line);
    }
#else
    static constexpr SrcPosition Current()
    {
        return SrcPosition(nullptr, nullptr, 0);
    }
#endif

    constexpr const char *FileName() const
    {
        return file_;
    }

    constexpr int Line() const
    {
        return line_;
    }

    constexpr const char *Function() const
    {
        return function_;
    }

    std::string ToString() const
    {
        if (!file_) {
            return {};
        }
        std::string result;
        if (function_) {
            result = std::string(function_) + "@";
        }
        result += file_;
        result += ":";
        result += std::to_string(line_);
        return result;
    }

    bool IsValid() const
    {
        return file_ != nullptr;
    }

private:
    constexpr SrcPosition(const char *func, const char *file, int line) : function_(func), file_(file), line_(line) {}

    const char *function_;
    const char *file_;
    int line_;

    friend class ArkSteedAssembler;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_ARKSTEED_SOURCE_LOCATION_H
