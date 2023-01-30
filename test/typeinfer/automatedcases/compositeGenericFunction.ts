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

// === tests/cases/compiler/compositeGenericFunction.ts ===
declare function AssertType(value:any, type:string):void;
function f<T>(value: T) { 
AssertType(value, "T");
return value; };

function h<R>(func: (x: number) => R): R { 
AssertType(null, "null");
return null; 
}

let z: number = h<number>(f);
AssertType(z, "number");
AssertType(h<number>(f), "number");
AssertType(h, "<R>((number) => R) => R");
AssertType(f, "<T>(T) => T");

let z: number = h(f);
AssertType(z, "number");
AssertType(h(f), "number");
AssertType(h, "<R>((number) => R) => R");
AssertType(f, "<T>(T) => T");


