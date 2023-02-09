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

// === tests/cases/compiler/indexClassByNumber.ts ===
declare function AssertType(value:any, type:string):void;
// Shouldn't be able to index a class instance by a number (unless it has declared a number index signature)

class foo { }

let f = new foo();
AssertType(f, "foo");
AssertType(new foo(), "foo");
AssertType(foo, "typeof foo");

f[0] = 4; // Shouldn't be allowed
AssertType(f[0] = 4, "int");
AssertType(f[0], "error");
AssertType(f, "foo");
AssertType(0, "int");
AssertType(4, "int");


