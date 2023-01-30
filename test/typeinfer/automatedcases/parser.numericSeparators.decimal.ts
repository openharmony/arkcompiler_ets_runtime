/*
* Copyright (c) Microsoft Corporation. All rights reserved.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/conformance/parser/ecmascript2021/numericSeparators/parser.numericSeparators.decimal.ts ===
declare function AssertType(value:any, type:string):void;
1_000_000_000
AssertType(1_000_000_000, "int");

1.1_00_01
AssertType(1.1_00_01, "double");

1e1_0
AssertType(1e1_0, "int");

1e+1_0
AssertType(1e+1_0, "int");

1e-1_0
AssertType(1e-1_0, "int");

1.1e10_0
AssertType(1.1e10_0, "int");

1.1e+10_0
AssertType(1.1e+10_0, "int");

1.1e-10_0
AssertType(1.1e-10_0, "int");

12_34_56
AssertType(12_34_56, "int");

1_22_333
AssertType(1_22_333, "int");

1_2.3_4
AssertType(1_2.3_4, "double");

1_2.3_4e5_6
AssertType(1_2.3_4e5_6, "int");

1_2.3_4e+5_6
AssertType(1_2.3_4e+5_6, "int");

1_2.3_4e-5_6
AssertType(1_2.3_4e-5_6, "int");


