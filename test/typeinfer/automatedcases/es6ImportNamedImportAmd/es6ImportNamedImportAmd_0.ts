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

// === tests/cases/compiler/es6ImportNamedImportAmd_0.ts ===
declare function AssertType(value:any, type:string):void;
export let a = 10;
AssertType(a, "number");
AssertType(10, "int");

export let x = a;
AssertType(x, "number");
AssertType(a, "number");

export let m = a;
AssertType(m, "number");
AssertType(a, "number");

export let a1 = 10;
AssertType(a1, "number");
AssertType(10, "int");

export let x1 = 10;
AssertType(x1, "number");
AssertType(10, "int");

export let z1 = 10;
AssertType(z1, "number");
AssertType(10, "int");

export let z2 = 10;
AssertType(z2, "number");
AssertType(10, "int");

export let aaaa = 10;
AssertType(aaaa, "number");
AssertType(10, "int");


