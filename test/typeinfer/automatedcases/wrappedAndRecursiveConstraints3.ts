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

// === tests/cases/conformance/types/typeParameters/typeArgumentLists/wrappedAndRecursiveConstraints3.ts ===
declare function AssertType(value:any, type:string):void;
// no errors expected

class C<T extends { length: number }> {
    constructor(x: T) { }
    foo<U extends T>(x: U) {
        function bar<V extends U>(x: V) {
AssertType(bar, "<V extends U>(V) => V");
AssertType(x, "V");

AssertType(x, "V");
            return x;
        }
AssertType(bar, "<V extends U>(V) => V");
        return bar;
    }
}

let c = new C({ length: 2 });
AssertType(c, "C<{ length: number; }>");
AssertType(new C({ length: 2 }), "C<{ length: number; }>");
AssertType(C, "typeof C");
AssertType({ length: 2 }, "{ length: number; }");
AssertType(length, "number");
AssertType(2, "int");

let r = c.foo({ length: 3, charAt: (x: number) => { '' } });
AssertType(r, "<V extends { length: number; charAt: (number) => void; }>(V) => V");
AssertType(c.foo({ length: 3, charAt: (x: number) => { '' } }), "<V extends { length: number; charAt: (number) => void; }>(V) => V");
AssertType(c.foo, "<U extends { length: number; }>(U) => <V extends U>(V) => V");
AssertType({ length: 3, charAt: (x: number) => { '' } }, "{ length: number; charAt: (number) => void; }");
AssertType(length, "number");
AssertType(3, "int");
AssertType(charAt, "(number) => void");
AssertType((x: number) => { '' }, "(number) => void");
AssertType(x, "number");
AssertType('', "string");

let r2 = r('');
AssertType(r2, "string");
AssertType(r(''), "string");
AssertType(r, "<V extends { length: number; charAt: (number) => void; }>(V) => V");
AssertType('', "string");


