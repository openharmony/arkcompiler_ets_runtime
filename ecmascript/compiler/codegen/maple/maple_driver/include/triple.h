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

#ifndef MAPLE_TRIPLE_H
#define MAPLE_TRIPLE_H

#include <string_utils.h>
#include <utils.h>

#include <string>
#include <string_view>
#include <vector>

namespace maple {

class Triple {
public:
    /* Currently, only aarch64 is supported */
    enum ArchType { UnknownArch, aarch64, aarch64_be, x64, LastArchType };

    /* Currently, only ILP32 and LP64 are supported */
    enum EnvironmentType { UnknownEnvironment, GNU, GNUILP32, LastEnvironmentType };

    ArchType GetArch() const
    {
        return arch;
    }
    EnvironmentType GetEnvironment() const
    {
        return environment;
    }

    bool IsBigEndian() const
    {
        return (GetArch() == ArchType::aarch64_be);
    }

    bool IsAarch64BeOrLe() const
    {
        return (GetArch() == ArchType::aarch64_be) || (GetArch() == ArchType::aarch64);
    }

    static Triple &GetTriple()
    {
        static Triple triple;
        return triple;
    }
    Triple(const Triple &) = delete;
    Triple &operator=(const Triple &) = delete;

    void Init(bool isAArch64);

private:
    std::string data;
    ArchType arch;
    EnvironmentType environment;

    Triple() : arch(UnknownArch), environment(UnknownEnvironment) {}
};
}  // namespace maple

#endif /* MAPLE_TRIPLE_H */
