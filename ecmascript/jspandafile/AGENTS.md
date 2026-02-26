# AGENTS.md - ArkCompiler ETS Runtime

This file contains guidelines for agentic coding assistants working in this repository.

## Build and Test Commands

### 编译参数说明

| **参数名称** | **描述** | **示例** |
| --- | --- | --- |
| `--product-name` | 指定产品名称 | rk3568 |
| `--build-target` | 指定构建编译对象 | ark_js_host_unittest：单元测试
ark_js_host_linux_tools_packages：工具包
ets_frontend_build：前端编译 |
| `--gn-args` | 传递编译参数 | is_debug=true：启用调试模式 |

### 常用编译命令

#### Release 编译

编译发布版本（无调试信息）：

```bash
./build.sh --product-name rk3568 \
           --build-target ark_js_host_unittest \
           --build-target ark_js_unittest \
           --build-target ark_js_host_linux_tools_packages
```

#### Debug 编译

编译调试版本（包含调试信息）：

```bash
./build.sh --product-name rk3568 \
           --build-target ark_js_host_unittest \
           --build-target ark_js_unittest \
           --build-target ark_js_host_linux_tools_packages \
           --gn-args is_debug=true
```

### 运行测试

```bash
# 运行所有 JSPandaFile 测试
./build.sh --product-name rk3568 --build-target JSPandaFileTestAction

# 运行单个测试
./build.sh --product-name rk3568 --build-target JSPandaFileTestAction --gn-args target_test_filters="JSPandaFileTest.TestName"

# 运行测试文件中的所有测试
./build.sh --product-name rk3568 --build-target JSPandaFileTestAction --gn-args target_test_filters="ClassInfoExtractorTest.*"
```

**NOTE:** Run all commands from **OpenHarmony root directory**.

## JSPandaFile Architecture

### Key Classes

| Class | Purpose |
|--------|----------|
| JSPandaFile | Panda file core class for loading and managing ABC bytecode files |
| Program | Program object containing main function references |
| ConstantPool | Constant pool storing strings, method literals, class literals, etc. |
| JSRecordInfo | JS record info containing module metadata |
| TranslateClassesTask | Class translation task supporting multi-threaded class definition translation |
| ClassInfoExtractor | Class information extractor from bytecode |
| LiteralDataExtractor | Literal data extractor from constant pool |
| MethodLiteral | Method literal representation |
| PandaFileTranslator | Panda file translator for code generation |
| JSPandaFileManager | Panda file manager for lifecycle management |
| JSExecutor | Panda file executor for running bytecode |

### Class Relationships

| Relationship | Description |
|-------------|-------------|
| Program → ECMAObject | Program inherits from ECMAObject |
| ConstantPool → TaggedArray | ConstantPool inherits from TaggedArray |
| JSPandaFile ↔ ConstantPool | JSPandaFile contains ConstantPool reference |
| JSPandaFile ↔ MethodLiteral | JSPandaFile manages method literal mappings |
| TranslateClassesTask ↔ JSPandaFile | Translation task combines JSPandaFile with translation work |

### Key Enums

| Enum | Values |
|------|---------|
| CreateMode | RUNTIME, DFX |
| ConstPoolType | Constant pool type definitions |
| FunctionKind | Function type definitions |
| ClassKind | Class type definitions |

### Key Constants

| Constant | Value |
|----------|-------|
| ENTRY_FUNCTION_NAME | Entry function name constant |
| MODULE_CLASS | Module class constant |
| COMMONJS_CLASS | CommonJS class constant |

## Code Style Guidelines

### File Header
/*
 * Copyright (c) 2022-225 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2. (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    ://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions
 * limitations under the License.
 */
### Header Guards
Use `#ifndef` with full path format: `ECMASCRIPT_MODULE_FILENAME_H`.

### Include Order
1. Local project includes with `ecmascript/` prefix
2. Related module includes
3. Standard library includes

### Indentation & Formatting
- 4 spaces for indentation (no tabs)
- Opening brace on same line for classes, functions, control structures
- Closing brace on new line
- Access specifiers indented by 4 spaces (at class level)

### Naming Conventions
- **Classes:** PascalCase (`JSPandaFile`, `DebugInfoExtractor`)
- **Methods/Functions:** camelCase (`GetMethodName`, `ParseEntryPoint`)
- **Variables:** camelCase with trailing underscore for members (`methodIndex_`, `jsPandaFile`)
- **Constants:** UPPER_SNAKE_CASE with `static constexpr` (`ENTRY_FUNCTION_NAME`, `ASYN_TRANSLATE_CLSSS_COUNT`)
- **Enums:** PascalCase for type, UPPER_SNAKE_CASE for values
- **Macros:** UPPER_SNAKE_CASE

### Type Usage
- `uint8_t`, `uint32_t`, `int32_t` for fixed-width integers
- `CString` (project custom) for internal strings
- `std::string_view` for string parameters not needing modification
- `bool` for boolean values
- `std::shared_ptr`/`std::unique_ptr` for smart pointers
- `std::vector`, `std::unordered_map`, `std::set` for standard containers
- `CVector`, `CUnorderedMap`, `CString` from project's custom containers for GC-managed types

### Class Design
Mark classes with `NO_COPY_SEMANTIC` and `NO_MOVE_SEMANTIC` if copying/moving is forbidden. Use `PUBLIC_API` macro for classes/methods that need to be exported.

### Inline Functions
Mark short getter/setter functions as `inline`.

### Error Handling
- `LOG_ECMA(ERROR)` for errors, `LOG_ECMA(FATAL)` for fatal errors
- `LOG_FULL(FATAL)` for fatal errors in some contexts
- `UNREACHABLE()` after fatal logs to indicate code should not be reached
- `ASSERT()` for internal assertions
- `Expected<T, E>` return type for operations that can fail

### Member Initialization
Use in-class member initializers where possible: `const panda_file::File *pf_ {nullptr};`

### Const Correctness
Mark methods `const` when they don't modify object state.

### Test Style
- `TEST_F(TestClassName, TestName)` macro for basic tests
- Test class name ends with `Test`, inherits from `::` or `BaseTestWithScope<true>`
- `TestHelper::CreateEcmaVMWithScope` for VM setup
- Test documentation uses `.name`, `@tc.desc`, `@tc.type`, `@tc.require`

### Static Assertions
Use `STATIC_ASSERT_EQ_ARCH` for architecture-dependent size checks.

### Conditional Compilation
Use `#if ENABLE_LATEST_OPTIMIZATION` and similar feature flags with comments.

### Friend Classes
List at the end of class definition.

### Template Style
Template parameters use PascalCase.
