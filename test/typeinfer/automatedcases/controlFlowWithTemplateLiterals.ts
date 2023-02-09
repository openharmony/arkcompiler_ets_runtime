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

// === tests/cases/conformance/controlFlow/controlFlowWithTemplateLiterals.ts ===
declare function AssertType(value:any, type:string):void;
declare const envVar: string | undefined;
AssertType(envVar, "union");

if (typeof envVar === `string`) {
  envVar.slice(0)
AssertType(envVar.slice(0), "string");
AssertType(envVar.slice, "(?union, ?union) => string");
AssertType(0, "int");
}

declare const obj: {test: string} | {
AssertType(obj, "union");

AssertType(test, "string");
}

if (`test` in obj) {
  obj.test.slice(0)
AssertType(obj.test.slice(0), "string");
AssertType(obj.test.slice, "(?union, ?union) => string");
AssertType(obj.test, "string");
AssertType(0, "int");
}


