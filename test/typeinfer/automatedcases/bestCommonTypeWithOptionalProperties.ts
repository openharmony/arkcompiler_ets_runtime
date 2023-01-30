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

// === tests/cases/compiler/bestCommonTypeWithOptionalProperties.ts ===
declare function AssertType(value:any, type:string):void;
interface X { foo: string }
interface Y extends X { bar?: number }
interface Z extends X { bar: string }

let x: X;
AssertType(x, "X");

let y: Y;
AssertType(y, "Y");

let z: Z;
AssertType(z, "Z");

// All these arrays should be X[]
let b1 = [x, y, z];
AssertType(b1, "X[]");
AssertType([x, y, z], "X[]");
AssertType(x, "X");
AssertType(y, "Y");
AssertType(z, "Z");

let b2 = [x, z, y];
AssertType(b2, "X[]");
AssertType([x, z, y], "X[]");
AssertType(x, "X");
AssertType(z, "Z");
AssertType(y, "Y");

let b3 = [y, x, z];
AssertType(b3, "X[]");
AssertType([y, x, z], "X[]");
AssertType(y, "Y");
AssertType(x, "X");
AssertType(z, "Z");

let b4 = [y, z, x];
AssertType(b4, "X[]");
AssertType([y, z, x], "X[]");
AssertType(y, "Y");
AssertType(z, "Z");
AssertType(x, "X");

let b5 = [z, x, y];
AssertType(b5, "X[]");
AssertType([z, x, y], "X[]");
AssertType(z, "Z");
AssertType(x, "X");
AssertType(y, "Y");

let b6 = [z, y, x];
AssertType(b6, "X[]");
AssertType([z, y, x], "X[]");
AssertType(z, "Z");
AssertType(y, "Y");
AssertType(x, "X");


