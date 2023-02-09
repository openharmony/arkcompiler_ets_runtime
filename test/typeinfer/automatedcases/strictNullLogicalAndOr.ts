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

// === tests/cases/compiler/strictNullLogicalAndOr.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #9113

let sinOrCos = Math.random() < .5;
AssertType(sinOrCos, "boolean");
AssertType(Math.random() < .5, "boolean");
AssertType(Math.random(), "number");
AssertType(Math.random, "() => number");
AssertType(.5, "double");

let choice = sinOrCos && Math.sin || Math.cos;
AssertType(choice, "(number) => number");
AssertType(sinOrCos && Math.sin || Math.cos, "(number) => number");
AssertType(sinOrCos && Math.sin, "union");
AssertType(sinOrCos, "boolean");
AssertType(Math.sin, "(number) => number");
AssertType(Math.cos, "(number) => number");

choice(Math.PI);
AssertType(choice(Math.PI), "number");
AssertType(choice, "(number) => number");
AssertType(Math.PI, "number");

function sq(n?: number): number {
  const r = n !== undefined && n*n || 0;
AssertType(r, "number");
AssertType(n !== undefined && n*n || 0, "number");
AssertType(n !== undefined && n*n, "union");
AssertType(n !== undefined, "boolean");
AssertType(n, "union");
AssertType(undefined, "undefined");
AssertType(n*n, "number");
AssertType(n, "number");
AssertType(n, "number");
AssertType(0, "int");

AssertType(r, "number");
  return r;
}

sq(3);
AssertType(sq(3), "number");
AssertType(sq, "(?union) => number");
AssertType(3, "int");


