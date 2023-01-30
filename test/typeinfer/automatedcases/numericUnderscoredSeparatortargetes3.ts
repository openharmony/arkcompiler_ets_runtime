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

// === tests/cases/compiler/numericUnderscoredSeparator.ts ===
declare function AssertType(value:any, type:string):void;
1_000_000_000_000
AssertType(1_000_000_000_000, "int");

0b1010_0001_1000_0101
AssertType(0b1010_0001_1000_0101, "int");

0b1010_0001_1000_0101
AssertType(0b1010_0001_1000_0101, "int");

0xA0_B0_C0
AssertType(0xA0_B0_C0, "int");


