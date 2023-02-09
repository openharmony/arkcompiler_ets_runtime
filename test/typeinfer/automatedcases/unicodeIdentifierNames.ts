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

// === tests/cases/compiler/unicodeIdentifierNames.ts ===
declare function AssertType(value:any, type:string):void;
// This test is passing, except that on node the resulting file will not have a BOM and on CScript it will which makes 
// impossible to use a single baseline file to match the result of both. this test should be enabled once this issue is resolved.
let 才能ソЫⅨ蒤郳र्क्ड्राüışğİliيونيكودöÄüß才能ソЫⅨ蒤郳र्क्ड्राüışğİliيونيكودöÄüßAbcd123 = 1;
AssertType(才能ソЫⅨ蒤郳र्क्ड्राüışğİliيونيكودöÄüß才能ソЫⅨ蒤郳र्क्ड्राüışğİliيونيكودöÄüßAbcd123, "number");
AssertType(1, "int");


