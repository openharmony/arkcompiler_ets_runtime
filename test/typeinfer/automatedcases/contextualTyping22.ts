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

// === tests/cases/compiler/contextualTyping22.ts ===
declare function AssertType(value:any, type:string):void;
let foo:(a:number)=>number = function(a){
AssertType(foo, "(number) => number");
AssertType(a, "number");
AssertType(function(a){return a}, "(number) => number");
AssertType(a, "number");
AssertType(a, "number");
AssertType(foo = function(b){return b}, "(number) => number");
AssertType(foo, "(number) => number");
AssertType(function(b){return b}, "(number) => number");
AssertType(b, "number");
AssertType(b, "number");
return a}; foo = function(b){return b};


