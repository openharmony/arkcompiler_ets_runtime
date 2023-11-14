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

#include "driver_options.h"
#include "triple.h"
#include "gtest/gtest.h"

using namespace maple;

/* Check Triple initialization with target string */
TEST(utTriple, triple1)
{
    Triple::GetTriple().Init("aarch64");
    ASSERT_EQ(Triple::GetTriple().GetEnvironment(), Triple::EnvironmentType::GNU);
    ASSERT_EQ(Triple::GetTriple().GetArch(), Triple::ArchType::aarch64);
    ASSERT_EQ(Triple::GetTriple().IsBigEndian(), false);

    Triple::GetTriple().Init("aarch64_le");
    ASSERT_EQ(Triple::GetTriple().GetEnvironment(), Triple::EnvironmentType::GNU);
    ASSERT_EQ(Triple::GetTriple().GetArch(), Triple::ArchType::aarch64);
    ASSERT_EQ(Triple::GetTriple().IsBigEndian(), false);

    Triple::GetTriple().Init("aarch64_be");
    ASSERT_EQ(Triple::GetTriple().GetEnvironment(), Triple::EnvironmentType::GNU);
    ASSERT_EQ(Triple::GetTriple().GetArch(), Triple::ArchType::aarch64_be);
    ASSERT_EQ(Triple::GetTriple().IsBigEndian(), true);

    Triple::GetTriple().Init("aarch64_be-ilp32");
    ASSERT_EQ(Triple::GetTriple().GetEnvironment(), Triple::EnvironmentType::GNUILP32);
    ASSERT_EQ(Triple::GetTriple().GetArch(), Triple::ArchType::aarch64_be);
    ASSERT_EQ(Triple::GetTriple().IsBigEndian(), true);

    Triple::GetTriple().Init("aarch64_be-gnuilp32");
    ASSERT_EQ(Triple::GetTriple().GetEnvironment(), Triple::EnvironmentType::GNUILP32);
    ASSERT_EQ(Triple::GetTriple().GetArch(), Triple::ArchType::aarch64_be);
    ASSERT_EQ(Triple::GetTriple().IsBigEndian(), true);

    Triple::GetTriple().Init("aarch64_be-linux-gnuilp32");
    ASSERT_EQ(Triple::GetTriple().GetEnvironment(), Triple::EnvironmentType::GNUILP32);
    ASSERT_EQ(Triple::GetTriple().GetArch(), Triple::ArchType::aarch64_be);
    ASSERT_EQ(Triple::GetTriple().IsBigEndian(), true);

    Triple::GetTriple().Init();
    ASSERT_EQ(Triple::GetTriple().GetEnvironment(), Triple::EnvironmentType::GNU);
    ASSERT_EQ(Triple::GetTriple().GetArch(), Triple::ArchType::aarch64);
    ASSERT_EQ(Triple::GetTriple().IsBigEndian(), false);

    /* Incorrect value leads to default initialization */
    Triple::GetTriple().Init("incorect-triple");
    ASSERT_EQ(Triple::GetTriple().GetEnvironment(), Triple::EnvironmentType::GNU);
    ASSERT_EQ(Triple::GetTriple().GetArch(), Triple::ArchType::aarch64);
    ASSERT_EQ(Triple::GetTriple().IsBigEndian(), false);
}

/* Check Triple initialization with options */
TEST(utTriple, triple2)
{
    const char *argv[] = {"maple",  // program name
                          "-be", "--ilp32", nullptr};
    int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

    cl::CommandLine::GetCommandLine().Clear();
    auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
    ASSERT_EQ(err, cl::RetCode::noError);

    /* Triple checks -be and --ilp32 options inside to set the target */
    Triple::GetTriple().Init();
    ASSERT_EQ(Triple::GetTriple().GetEnvironment(), Triple::EnvironmentType::GNUILP32);
    ASSERT_EQ(Triple::GetTriple().GetArch(), Triple::ArchType::aarch64_be);
    ASSERT_EQ(Triple::GetTriple().IsBigEndian(), true);

    const char *argv2[] = {"maple",  // program name
                           nullptr};
    argc = (sizeof(argv) / sizeof(argv[0])) - 1;

    cl::CommandLine::GetCommandLine().Clear();
    err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv2);
    ASSERT_EQ(err, cl::RetCode::noError);

    Triple::GetTriple().Init();
    ASSERT_EQ(Triple::GetTriple().GetEnvironment(), Triple::EnvironmentType::GNU);
    ASSERT_EQ(Triple::GetTriple().GetArch(), Triple::ArchType::aarch64);
    ASSERT_EQ(Triple::GetTriple().IsBigEndian(), false);

    // create command line
    const char *argv3[] = {"maple",  // program name
                           "--target=aarch64_be-gnuilp32", nullptr};
    argc = (sizeof(argv) / sizeof(argv[0])) - 1;

    cl::CommandLine::GetCommandLine().Clear();
    err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv3);
    ASSERT_EQ(err, cl::RetCode::noError);

    Triple::GetTriple().Init(opts::target);
    ASSERT_EQ(Triple::GetTriple().GetEnvironment(), Triple::EnvironmentType::GNUILP32);
    ASSERT_EQ(Triple::GetTriple().GetArch(), Triple::ArchType::aarch64_be);
    ASSERT_EQ(Triple::GetTriple().IsBigEndian(), true);
}

/* Check the priority of Triple initialization: -be/--ilp32 vs --target */
TEST(utTriple, triple3)
{
    // create command line
    const char *argv[] = {"maple",  // program name
                          "-be", "--ilp32", "--target=aarch64-gnu", nullptr};
    int argc = (sizeof(argv) / sizeof(argv[0])) - 1;

    cl::CommandLine::GetCommandLine().Clear();
    auto err = cl::CommandLine::GetCommandLine().Parse(argc, (char **)argv);
    ASSERT_EQ(err, cl::RetCode::noError);

    /* --target option has hicher priority than --ilp32/-be options */
    Triple::GetTriple().Init(opts::target);
    ASSERT_EQ(Triple::GetTriple().GetEnvironment(), Triple::EnvironmentType::GNU);
    ASSERT_EQ(Triple::GetTriple().GetArch(), Triple::ArchType::aarch64);
    ASSERT_EQ(Triple::GetTriple().IsBigEndian(), false);
}
