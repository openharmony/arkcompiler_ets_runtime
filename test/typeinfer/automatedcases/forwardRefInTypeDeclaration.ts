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

// === tests/cases/compiler/forwardRefInTypeDeclaration.ts ===
declare function AssertType(value:any, type:string):void;
// forward ref ignored in a typeof
declare let s: typeof s1;
AssertType(s, "string");
AssertType(s1, "string");

const s1 = "x";
AssertType(s1, "string");
AssertType("x", "string");

// ignored anywhere in an interface (#35947)
interface Foo2 { [s2]: number; }
const s2 = "x";
AssertType(s2, "string");
AssertType("x", "string");

// or in a type definition
type Foo3 = { [s3]: number; }
const s3 = "x";
AssertType(s3, "string");
AssertType("x", "string");


