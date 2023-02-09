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

// === tests/cases/conformance/types/typeRelationships/typeAndMemberIdentity/objectTypesIdentityWithCallSignaturesDifferingParamCounts2.ts ===
declare function AssertType(value:any, type:string):void;
// object types are identical structurally

interface I {
    (x: string): string;
}

interface I2<T> {
    (x: T): T;
}

let a: { (x: string, y: string): string 
AssertType(a, "(string, string) => string");

AssertType(x, "string");

AssertType(y, "string");
}

function foo2(x: I);
function foo2(x: I); // error
function foo2(x: any) { }

function foo3(x: typeof a);
function foo3(x: typeof a); // error
function foo3(x: any) { }

function foo4(x: I2<string>);
function foo4(x: I2<string>); // error
function foo4(x: any) { }

function foo5(x: I2<string>);
function foo5(x: I2<number>); // ok
function foo5(x: any) { }

function foo13(x: I);
function foo13(x: typeof a); // ok
function foo13(x: any) { }

function foo14(x: I);
function foo14(x: I2<string>); // error
function foo14(x: any) { }

function foo14b(x: typeof a);
function foo14b(x: I2<string>); // ok
function foo14b(x: any) { }

function foo15(x: I);
function foo15(x: I2<number>); // ok
function foo15(x: any) { }

