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

// === tests/cases/conformance/controlFlow/controlFlowParameter.ts ===
declare function AssertType(value:any, type:string):void;
function f1(
  required: unknown = (() => {
    throw new Error("bad");
AssertType(new Error("bad"), "Error");
AssertType(Error, "ErrorConstructor");
AssertType("bad", "string");

  })()
) {
  console.log("ok"); // should not trigger 'Unreachable code detected.'
AssertType(console.log("ok"), "void");
AssertType(console.log, "(...any[]) => void");
AssertType("ok", "string");
}

function f2(
  a: number | string | undefined,
  required: unknown = (() => {
    a = 1;
AssertType(a = 1, "int");
AssertType(a, "union");
AssertType(1, "int");

  })()
) {
  a; // should be number | string | undefined
AssertType(a, "union");
}

function f3(
  a: number | string | undefined = 1,
  required: unknown = (() => {
    a = "";
AssertType(a = "", "string");
AssertType(a, "union");
AssertType("", "string");

  })()
) {
  a; // should be number | string
AssertType(a, "union");
}

function f4(
  a: number | string | undefined = 1,
  { [(a = "")]: b } = {} as any
) {
  a; // should be string
AssertType(a, "string");
}


