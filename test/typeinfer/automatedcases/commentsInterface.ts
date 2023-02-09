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

// === tests/cases/compiler/commentsInterface.ts ===
declare function AssertType(value:any, type:string):void;
/** this is interface 1*/
interface i1 {
}
let i1_i: i1;
AssertType(i1_i, "i1");

interface nc_i1 {
}
let nc_i1_i: nc_i1;
AssertType(nc_i1_i, "nc_i1");

/** this is interface 2 with memebers*/
interface i2 {
    /** this is x*/
    x: number;
    /** this is foo*/
    foo: (/**param help*/b: number) => string;
    /** this is indexer*/
    [/**string param*/i: string]: any;
    /**new method*/
    new (/** param*/i: i1);
    nc_x: number;
    nc_foo: (b: number) => string;
    [i: number]: number;
    /** this is call signature*/
    (/**paramhelp a*/a: number,/**paramhelp b*/ b: number) : number;
    /** this is fnfoo*/
    fnfoo(/**param help*/b: number): string;
    nc_fnfoo(b: number): string;
    // nc_y
    nc_y: number;
}
let i2_i: i2;
AssertType(i2_i, "i2");

let i2_i_x = i2_i.x;
AssertType(i2_i_x, "number");
AssertType(i2_i.x, "number");

let i2_i_foo = i2_i.foo;
AssertType(i2_i_foo, "(number) => string");
AssertType(i2_i.foo, "(number) => string");

let i2_i_foo_r = i2_i.foo(30);
AssertType(i2_i_foo_r, "string");
AssertType(i2_i.foo(30), "string");
AssertType(i2_i.foo, "(number) => string");
AssertType(30, "int");

let i2_i_i2_si = i2_i["hello"];
AssertType(i2_i_i2_si, "any");
AssertType(i2_i["hello"], "any");
AssertType(i2_i, "i2");
AssertType("hello", "string");

let i2_i_i2_ii = i2_i[30];
AssertType(i2_i_i2_ii, "number");
AssertType(i2_i[30], "number");
AssertType(i2_i, "i2");
AssertType(30, "int");

let i2_i_n = new i2_i(i1_i);
AssertType(i2_i_n, "any");
AssertType(new i2_i(i1_i), "any");
AssertType(i2_i, "i2");
AssertType(i1_i, "i1");

let i2_i_nc_x = i2_i.nc_x;
AssertType(i2_i_nc_x, "number");
AssertType(i2_i.nc_x, "number");

let i2_i_nc_foo = i2_i.nc_foo;
AssertType(i2_i_nc_foo, "(number) => string");
AssertType(i2_i.nc_foo, "(number) => string");

let i2_i_nc_foo_r = i2_i.nc_foo(30);
AssertType(i2_i_nc_foo_r, "string");
AssertType(i2_i.nc_foo(30), "string");
AssertType(i2_i.nc_foo, "(number) => string");
AssertType(30, "int");

let i2_i_r = i2_i(10, 20);
AssertType(i2_i_r, "number");
AssertType(i2_i(10, 20), "number");
AssertType(i2_i, "i2");
AssertType(10, "int");
AssertType(20, "int");

let i2_i_fnfoo = i2_i.fnfoo;
AssertType(i2_i_fnfoo, "(number) => string");
AssertType(i2_i.fnfoo, "(number) => string");

let i2_i_fnfoo_r = i2_i.fnfoo(10);
AssertType(i2_i_fnfoo_r, "string");
AssertType(i2_i.fnfoo(10), "string");
AssertType(i2_i.fnfoo, "(number) => string");
AssertType(10, "int");

let i2_i_nc_fnfoo = i2_i.nc_fnfoo;
AssertType(i2_i_nc_fnfoo, "(number) => string");
AssertType(i2_i.nc_fnfoo, "(number) => string");

let i2_i_nc_fnfoo_r = i2_i.nc_fnfoo(10);
AssertType(i2_i_nc_fnfoo_r, "string");
AssertType(i2_i.nc_fnfoo(10), "string");
AssertType(i2_i.nc_fnfoo, "(number) => string");
AssertType(10, "int");

interface i3 {
    /** Comment i3 x*/
    x: number;
    /** Function i3 f*/
    f(/**number parameter*/a: number): string;
    /** i3 l*/
    l: (/**comment i3 l b*/b: number) => string;
    nc_x: number;
    nc_f(a: number): string;
    nc_l: (b: number) => string;
}
let i3_i: i3;
AssertType(i3_i, "i3");

i3_i = {
AssertType(i3_i = {    f: /**own f*/ (/**i3_i a*/a: number) => "Hello" + a,    l: this.f,    /** own x*/    x: this.f(10),    nc_x: this.l(this.x),    nc_f: this.f,    nc_l: this.l}, "{ f: (number) => string; l: any; x: any; nc_x: any; nc_f: any; nc_l: any; }");
AssertType(i3_i, "i3");
AssertType({    f: /**own f*/ (/**i3_i a*/a: number) => "Hello" + a,    l: this.f,    /** own x*/    x: this.f(10),    nc_x: this.l(this.x),    nc_f: this.f,    nc_l: this.l}, "{ f: (number) => string; l: any; x: any; nc_x: any; nc_f: any; nc_l: any; }");

    f: /**own f*/ (/**i3_i a*/a: number) => "Hello" + a,
AssertType(f, "(number) => string");
AssertType((/**i3_i a*/a: number) => "Hello" + a, "(number) => string");
AssertType(a, "number");
AssertType("Hello" + a, "string");
AssertType("Hello", "string");
AssertType(a, "number");

    l: this.f,
AssertType(l, "any");
AssertType(this.f, "any");

    /** own x*/
    x: this.f(10),
AssertType(x, "any");
AssertType(this.f(10), "any");
AssertType(this.f, "any");
AssertType(10, "int");

    nc_x: this.l(this.x),
AssertType(nc_x, "any");
AssertType(this.l(this.x), "any");
AssertType(this.l, "any");
AssertType(this.x, "any");

    nc_f: this.f,
AssertType(nc_f, "any");
AssertType(this.f, "any");

    nc_l: this.l
AssertType(nc_l, "any");
AssertType(this.l, "any");

};
i3_i.f(10);
AssertType(i3_i.f(10), "string");
AssertType(i3_i.f, "(number) => string");
AssertType(10, "int");

i3_i.l(10);
AssertType(i3_i.l(10), "string");
AssertType(i3_i.l, "(number) => string");
AssertType(10, "int");

i3_i.nc_f(10);
AssertType(i3_i.nc_f(10), "string");
AssertType(i3_i.nc_f, "(number) => string");
AssertType(10, "int");

i3_i.nc_l(10);
AssertType(i3_i.nc_l(10), "string");
AssertType(i3_i.nc_l, "(number) => string");
AssertType(10, "int");


