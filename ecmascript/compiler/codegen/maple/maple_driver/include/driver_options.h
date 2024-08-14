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

#ifndef MAPLE_DRIVER_INCLUDE_DRIVER_OPTIONS_H
#define MAPLE_DRIVER_INCLUDE_DRIVER_OPTIONS_H

#include "cl_option.h"
#include "cl_parser.h"

#include <string>

static maplecl::OptionCategory &driverCategory = maplecl::CommandLine::GetCommandLine().defaultCategory;

static maplecl::OptionCategory &clangCategory = maplecl::CommandLine::GetCommandLine().clangCategory;
static maplecl::OptionCategory &hir2mplCategory = maplecl::CommandLine::GetCommandLine().hir2mplCategory;
static maplecl::OptionCategory &mpl2mplCategory = maplecl::CommandLine::GetCommandLine().mpl2mplCategory;
static maplecl::OptionCategory &meCategory = maplecl::CommandLine::GetCommandLine().meCategory;
static maplecl::OptionCategory &cgCategory = maplecl::CommandLine::GetCommandLine().cgCategory;
static maplecl::OptionCategory &asCategory = maplecl::CommandLine::GetCommandLine().asCategory;
static maplecl::OptionCategory &ldCategory = maplecl::CommandLine::GetCommandLine().ldCategory;

static maplecl::OptionCategory &jbc2mplCategory = maplecl::CommandLine::GetCommandLine().jbc2mplCategory;
static maplecl::OptionCategory &ipaCategory = maplecl::CommandLine::GetCommandLine().ipaCategory;

#endif /* MAPLE_DRIVER_INCLUDE_DRIVER_OPTIONS_H */
