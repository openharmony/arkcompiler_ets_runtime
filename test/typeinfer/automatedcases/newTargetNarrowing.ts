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

// === tests/cases/conformance/es6/newTarget/newTargetNarrowing.ts ===
declare function AssertType(value:any, type:string):void;
function foo(x: true) { }

function f() {
  if (new.target.marked === true) {
AssertType(new.target.marked === true, "boolean");
AssertType(new.target.marked, "boolean");
AssertType(new.target, "typeof f");
AssertType(target, "any");
AssertType(true, "boolean");

    foo(new.target.marked);
AssertType(foo(new.target.marked), "void");
AssertType(foo, "(true) => void");
AssertType(new.target.marked, "boolean");
AssertType(new.target, "typeof f");
AssertType(target, "any");
  }
}

f.marked = true;
AssertType(f.marked = true, "boolean");
AssertType(f.marked, "boolean");
AssertType(true, "boolean");


