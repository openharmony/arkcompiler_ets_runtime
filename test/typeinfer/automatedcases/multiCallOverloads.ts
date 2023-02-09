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

// === tests/cases/compiler/multiCallOverloads.ts ===
declare function AssertType(value:any, type:string):void;
interface ICallback {
    (x?: string):void;
}

function load(f: ICallback) {}

let f1: ICallback = function(z?) {
AssertType(f1, "ICallback");

AssertType(function(z?) {}, "(?string) => void");

AssertType(z, "string");
}

let f2: ICallback = function(z?) {
AssertType(f2, "ICallback");

AssertType(function(z?) {}, "(?string) => void");

AssertType(z, "string");
}

load(f1) // ok
AssertType(load(f1), "void");
AssertType(load, "(ICallback) => void");
AssertType(f1, "ICallback");

load(f2) // ok
AssertType(load(f2), "void");
AssertType(load, "(ICallback) => void");
AssertType(f2, "ICallback");

load(function() {}) // this shouldn’t be an error
AssertType(load(function() {}), "void");
AssertType(load, "(ICallback) => void");
AssertType(function() {}, "() => void");

load(function(z?) {}) // this shouldn't be an error
AssertType(load(function(z?) {}), "void");
AssertType(load, "(ICallback) => void");
AssertType(function(z?) {}, "(?string) => void");
AssertType(z, "string");


