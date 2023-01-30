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

// === tests/cases/compiler/m7Bugs.ts ===
declare function AssertType(value:any, type:string):void;
// scenario 1
interface ISomething {
   something: number;
}

let s: ISomething = <ISomething>({ });
AssertType(s, "ISomething");
AssertType(<ISomething>({ }), "ISomething");
AssertType(({ }), "{}");
AssertType({ }, "{}");


// scenario 2
interface A { x: string; }

interface B extends A { }

let x: B = <B>{ };
AssertType(x, "B");
AssertType(<B>{ }, "B");
AssertType({ }, "{}");

class C1 {
	public x: string;
}

class C2 extends C1 {}

let y1: C1 = new C2();
AssertType(y1, "C1");
AssertType(new C2(), "C2");
AssertType(C2, "typeof C2");

let y2: C1 = <C1> new C2();
AssertType(y2, "C1");
AssertType(<C1> new C2(), "C1");
AssertType(new C2(), "C2");
AssertType(C2, "typeof C2");

let y3: C1 = <C1> {};
AssertType(y3, "C1");
AssertType(<C1> {}, "C1");
AssertType({}, "{}");



