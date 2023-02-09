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

// === tests/cases/compiler/genericTypeAssertions3.ts ===
declare function AssertType(value:any, type:string):void;
let r = < <T>(x: T) => T > ((x) => { 
AssertType(r, "<T>(T) => T");
AssertType(< <T>(x: T) => T > ((x) => { return null; }), "<T>(T) => T");
AssertType(x, "T");
AssertType(((x) => { return null; }), "<T>(T) => any");
AssertType((x) => { return null; }, "<T>(T) => any");
AssertType(x, "T");
AssertType(null, "null");
return null; }); // bug was 'could not find dotted symbol T' on x's annotation in the type assertion instead of no error

let s = < <T>(x: T) => T > ((x: any) => { 
AssertType(s, "<T>(T) => T");
AssertType(< <T>(x: T) => T > ((x: any) => { return null; }), "<T>(T) => T");
AssertType(x, "T");
AssertType(((x: any) => { return null; }), "(any) => any");
AssertType((x: any) => { return null; }, "(any) => any");
AssertType(x, "any");
AssertType(null, "null");
return null; }); // no error


