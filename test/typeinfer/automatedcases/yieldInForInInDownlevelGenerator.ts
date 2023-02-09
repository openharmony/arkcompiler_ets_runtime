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

// === tests/cases/compiler/yieldInForInInDownlevelGenerator.ts ===
declare function AssertType(value:any, type:string):void;
// https://github.com/microsoft/TypeScript/issues/49808
function* gen() {
  let obj: any = { foo: 1, bar: 2 };
AssertType(obj, "any");
AssertType({ foo: 1, bar: 2 }, "{ foo: number; bar: number; }");
AssertType(foo, "number");
AssertType(1, "int");
AssertType(bar, "number");
AssertType(2, "int");

  for (let key in obj) {
AssertType(key, "string");
AssertType(obj, "any");

      yield key;
AssertType(yield key, "any");
AssertType(key, "string");

      delete obj.bar;
AssertType(delete obj.bar, "boolean");
AssertType(obj.bar, "any");
  }
}

