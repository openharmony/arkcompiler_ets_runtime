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

// === tests/cases/compiler/mutrec.ts ===
declare function AssertType(value:any, type:string):void;
interface A {
    x:B[];
}

interface B {
    x:A[];
}

function f(p: A) { 
AssertType(p, "A");
return p };

let b:B;
AssertType(b, "B");

f(b);
AssertType(f(b), "A");
AssertType(f, "(A) => A");
AssertType(b, "B");

interface I1 {
    y:I2;
}

interface I2 {
    y:I3;
}

interface I3 {
    y:I1;
}

function g(p: I1) { 
AssertType(p, "I1");
return p };

let i2:I2;
AssertType(i2, "I2");

g(i2);
AssertType(g(i2), "I1");
AssertType(g, "(I1) => I1");
AssertType(i2, "I2");

let i3:I3;
AssertType(i3, "I3");

g(i3);
AssertType(g(i3), "I1");
AssertType(g, "(I1) => I1");
AssertType(i3, "I3");

interface I4 {
    y:I5;
}

interface I5 {
    y:I4;
}

let i4:I4;
AssertType(i4, "I4");

g(i4);
AssertType(g(i4), "I1");
AssertType(g, "(I1) => I1");
AssertType(i4, "I4");



