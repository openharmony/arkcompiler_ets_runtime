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

#include "triple.h"
#include "driver_options.h"

namespace maple {

Triple::ArchType Triple::ParseArch(std::string_view archStr)
{
    if (maple::utils::Contains({"aarch64", "aarch64_le"}, archStr)) {
        return Triple::ArchType::aarch64;
    } else if (maple::utils::Contains({"aarch64_be"}, archStr)) {
        return Triple::ArchType::aarch64_be;
    }

    // Currently Triple support only aarch64
    return Triple::UnknownArch;
}

Triple::EnvironmentType Triple::ParseEnvironment(std::string_view archStr)
{
    if (maple::utils::Contains({"ilp32", "gnu_ilp32", "gnuilp32"}, archStr)) {
        return Triple::EnvironmentType::GNUILP32;
    } else if (maple::utils::Contains({"gnu"}, archStr)) {
        return Triple::EnvironmentType::GNU;
    }

    // Currently Triple support only ilp32 and default gnu/LP64 ABI
    return Triple::UnknownEnvironment;
}

void Triple::Init(bool isAArch64)
{
    /* Currently Triple is used only to configure aarch64: be/le, ILP32/LP64
     * Other architectures (TARGX86_64, TARGX86, TARGARM32, TARGVM) are configured with compiler build config */
    if (isAArch64) {
        arch = Triple::ArchType::aarch64;
        environment = Triple::EnvironmentType::GNU;
    } else {
        arch = Triple::ArchType::x64;
        environment = Triple::EnvironmentType::GNU;
    }
}

void Triple::Init(const std::string &target)
{
    data = target;

    /* Currently Triple is used only to configure aarch64: be/le, ILP32/LP64.
     * Other architectures (TARGX86_64, TARGX86, TARGARM32, TARGVM) are configured with compiler build config */
#if TARGAARCH64
    Init(true);

    std::vector<std::string_view> components;
    maple::StringUtils::SplitSV(data, components, '-');
    if (components.size() == 0) {  // as minimum 1 component must be
        return;
    }

    auto tmpArch = ParseArch(components[0]);  // to not overwrite arch seting by opts::bigendian
    if (tmpArch == Triple::UnknownArch) {
        return;
    }
    arch = tmpArch;

    /* Try to check environment in option.
     * As example, it can be: aarch64-none-linux-gnu or aarch64-linux-gnu or aarch64-gnu, where gnu is environment */
    for (uint32_t i = 1; i < components.size(); ++i) {
        auto tmpEnvironment = ParseEnvironment(components[i]);
        if (tmpEnvironment != Triple::UnknownEnvironment) {
            environment = tmpEnvironment;
            break;
        }
    }
#endif
}

std::string Triple::GetArchName() const
{
    switch (arch) {
        case ArchType::aarch64_be:
            return "aarch64_be";
        case ArchType::aarch64:
            return "aarch64";
        default:
            DEBUG_ASSERT(false, "Unknown Architecture Type\n");
    }
    return "";
}

std::string Triple::GetEnvironmentName() const
{
    switch (environment) {
        case EnvironmentType::GNUILP32:
            return "gnu_ilp32";
        case EnvironmentType::GNU:
            return "gnu";
        default:
            DEBUG_ASSERT(false, "Unknown Environment Type\n");
    }
    return "";
}

std::string Triple::Str() const
{
    if (!data.empty()) {
        return data;
    }

    if (GetArch() != ArchType::UnknownArch && GetEnvironment() != Triple::EnvironmentType::UnknownEnvironment) {
        /* only linux platform is supported, so "-linux-" is hardcoded */
        return GetArchName() + "-linux-" + GetEnvironmentName();
    }

    CHECK_FATAL(false, "Only aarch64/aarch64_be GNU/GNUILP32 targets are supported\n");
    return data;
}

}  // namespace maple
