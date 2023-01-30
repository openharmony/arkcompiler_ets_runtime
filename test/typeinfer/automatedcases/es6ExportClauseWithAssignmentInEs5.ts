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

// === tests/cases/compiler/server.ts ===
declare function AssertType(value:any, type:string):void;
let foo = 2;
AssertType(foo, "number");
AssertType(2, "int");

foo = 3;
AssertType(foo = 3, "int");
AssertType(foo, "number");
AssertType(3, "int");

let baz = 3;
AssertType(baz, "number");
AssertType(3, "int");

baz = 4;
AssertType(baz = 4, "int");
AssertType(baz, "number");
AssertType(4, "int");

let buzz = 10;
AssertType(buzz, "number");
AssertType(10, "int");

buzz += 3;
AssertType(buzz += 3, "number");
AssertType(buzz, "number");
AssertType(3, "int");

let bizz = 8;
AssertType(bizz, "number");
AssertType(8, "int");

bizz++; // compiles to exports.bizz = (bizz++, bizz)
AssertType(bizz++, "number");
AssertType(bizz, "number");

bizz--; // similarly
AssertType(bizz--, "number");
AssertType(bizz, "number");

++bizz; // compiles to exports.bizz = ++bizz
AssertType(++bizz, "number");
AssertType(bizz, "number");

export { foo, baz, baz as quux, buzz, bizz };


