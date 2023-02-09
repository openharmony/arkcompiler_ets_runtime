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

// === tests/cases/conformance/types/members/objectTypePropertyAccess.ts ===
declare function AssertType(value:any, type:string):void;
// Index notation should resolve to the type of a declared property with that same name
class C {
    foo: string;
}

let c: C;
AssertType(c, "C");

let r1 = c.toString();
AssertType(r1, "string");
AssertType(c.toString(), "string");
AssertType(c.toString, "() => string");

let r2 = c['toString']();
AssertType(r2, "string");
AssertType(c['toString'](), "string");
AssertType(c['toString'], "() => string");
AssertType(c, "C");
AssertType('toString', "string");

let r3 = c.foo;
AssertType(r3, "string");
AssertType(c.foo, "string");

let r4 = c['foo'];
AssertType(r4, "string");
AssertType(c['foo'], "string");
AssertType(c, "C");
AssertType('foo', "string");

interface I {
    bar: string;
}
let i: I;
AssertType(i, "I");

let r4 = i.toString();
AssertType(r4, "string");
AssertType(i.toString(), "string");
AssertType(i.toString, "() => string");

let r5 = i['toString']();
AssertType(r5, "string");
AssertType(i['toString'](), "string");
AssertType(i['toString'], "() => string");
AssertType(i, "I");
AssertType('toString', "string");

let r6 = i.bar;
AssertType(r6, "string");
AssertType(i.bar, "string");

let r7 = i['bar'];
AssertType(r7, "string");
AssertType(i['bar'], "string");
AssertType(i, "I");
AssertType('bar', "string");

let a = {
AssertType(a, "{ foo: string; }");
AssertType({    foo: ''}, "{ foo: string; }");

    foo: ''
AssertType(foo, "string");
AssertType('', "string");
}

let r8 = a.toString();
AssertType(r8, "string");
AssertType(a.toString(), "string");
AssertType(a.toString, "() => string");

let r9 = a['toString']();
AssertType(r9, "string");
AssertType(a['toString'](), "string");
AssertType(a['toString'], "() => string");
AssertType(a, "{ foo: string; }");
AssertType('toString', "string");

let r10 = a.foo;
AssertType(r10, "string");
AssertType(a.foo, "string");

let r11 = a['foo'];
AssertType(r11, "string");
AssertType(a['foo'], "string");
AssertType(a, "{ foo: string; }");
AssertType('foo', "string");


