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

// === tests/cases/compiler/voidIsInitialized.ts ===
declare function AssertType(value:any, type:string):void;
const x: void = undefined;
AssertType(x, "void");
AssertType(undefined, "undefined");

const y: void = undefined;
AssertType(y, "void");
AssertType(undefined, "undefined");

if(typeof x === "undefined") {
    x // no error: assume x2 is initialised
AssertType(x, "undefined");
}

if(typeof y !== "undefined") {
    y // no error: do not narrow void
AssertType(y, "never");
}


