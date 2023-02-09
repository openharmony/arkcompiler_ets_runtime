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

// === tests/cases/conformance/types/namedTypes/classWithOnlyPublicMembersEquivalentToInterface2.ts ===
declare function AssertType(value:any, type:string):void;
// no errors expected

class C {
    public x: string;
    public y(a: number): number { 
AssertType(null, "null");
return null; 
}

    public get z() { 
AssertType(1, "int");
return 1; 
}

    public set z(v) { }
    [x: string]: Object;
    [x: number]: Object;
    0: number;

    public static foo: string; // doesn't effect equivalence
}

interface I {
    x: string;
    y(b: number): number;
    z: number;
    [x: string]: Object;
    [x: number]: Object;
    0: number;
}

let c: C;
AssertType(c, "C");

let i: I;
AssertType(i, "I");

c = i;
AssertType(c = i, "I");
AssertType(c, "C");
AssertType(i, "I");

i = c;
AssertType(i = c, "C");
AssertType(i, "I");
AssertType(c, "C");


