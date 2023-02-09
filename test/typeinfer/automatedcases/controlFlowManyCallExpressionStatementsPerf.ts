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

// === tests/cases/compiler/controlFlowManyCallExpressionStatementsPerf.ts ===
declare function AssertType(value:any, type:string):void;
function test(x: boolean): boolean { 
AssertType(x, "boolean");
return x; 
}

let state = true;
AssertType(state, "boolean");
AssertType(true, "boolean");

if (state) {
  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");

  test(state as any && state);
AssertType(test(state as any && state), "boolean");
AssertType(test, "(boolean) => boolean");
AssertType(state as any && state, "any");
AssertType(state as any, "any");
AssertType(state, "boolean");
AssertType(state, "boolean");
}


