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

// === tests/cases/conformance/types/typeRelationships/assignmentCompatibility/anyAssignableToEveryType2.ts ===
declare function AssertType(value:any, type:string):void;

// any is not a subtype of any other types, but is assignable, all the below should work

interface I {
    [x: string]: any;
    foo: any; // ok, any identical to itself
}


interface I2 {
    [x: string]: number;
    foo: any;
}


interface I3 {
    [x: string]: string;
    foo: any;
}


interface I4 {
    [x: string]: boolean;
    foo: any;
}


interface I5 {
    [x: string]: Date;
    foo: any;
}


interface I6 {
    [x: string]: RegExp;
    foo: any;
}


interface I7 {
    [x: string]: { bar: number };
    foo: any;
}


interface I8 {
    [x: string]: number[];
    foo: any;
}


interface I9 {
    [x: string]: I8;
    foo: any;
}

class A { foo: number; }
interface I10 {
    [x: string]: A;
    foo: any;
}

class A2<T> { foo: T; }
interface I11 {
    [x: string]: A2<number>;
    foo: any;
}


interface I12 {
    [x: string]: (x) => number;
    foo: any;
}


interface I13 {
    [x: string]: <T>(x: T) => T;
    foo: any;
}


enum E { A }
interface I14 {
    [x: string]: E;
    foo: any;
}


function f() { }
module f {
    export let bar = 1;
}
interface I15 {
    [x: string]: typeof f;
    foo: any;
}


class c { baz: string }
module c {
    export let bar = 1;
}
interface I16 {
    [x: string]: typeof c;
    foo: any;
}


interface I17<T> {
    [x: string]: T;
    foo: any;
}


interface I18<T, U extends T> {
    [x: string]: U;
    foo: any;
}


interface I19 {
    [x: string]: Object;
    foo: any;
}


interface I20 {
    [x: string]: {};
    foo: any;
}


