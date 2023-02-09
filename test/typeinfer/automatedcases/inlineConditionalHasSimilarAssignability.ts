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

// === tests/cases/compiler/inlineConditionalHasSimilarAssignability.ts ===
declare function AssertType(value:any, type:string):void;
type MyExtract<T, U> = T extends U ? T : never

function foo<T>(a: T) {
  const b: Extract<any[], T> = 0 as any;
AssertType(b, "Extract<any[], T>");
AssertType(0 as any, "any");
AssertType(0, "int");

  a = b; // ok
AssertType(a = b, "Extract<any[], T>");
AssertType(a, "T");
AssertType(b, "Extract<any[], T>");

  const c: (any[] extends T ? any[] : never) = 0 as any;
AssertType(c, "any[] extends T ? any[] : never");
AssertType(0 as any, "any");
AssertType(0, "int");

  a = c;
AssertType(a = c, "any[] extends T ? any[] : never");
AssertType(a, "T");
AssertType(c, "any[] extends T ? any[] : never");

  const d: MyExtract<any[], T> = 0 as any;
AssertType(d, "MyExtract<any[], T>");
AssertType(0 as any, "any");
AssertType(0, "int");

  a = d; // ok
AssertType(a = d, "MyExtract<any[], T>");
AssertType(a, "T");
AssertType(d, "MyExtract<any[], T>");

  type CustomType = any[] extends T ? any[] : never;
AssertType(CustomType, "any[] extends T ? any[] : never");

  const e: CustomType = 0 as any;
AssertType(e, "any[] extends T ? any[] : never");
AssertType(0 as any, "any");
AssertType(0, "int");

  a = e;
AssertType(a = e, "any[] extends T ? any[] : never");
AssertType(a, "T");
AssertType(e, "any[] extends T ? any[] : never");
}

