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

// === tests/cases/conformance/types/members/objectTypeHidingMembersOfObject.ts ===
declare function AssertType(value:any, type:string):void;
// all of these valueOf calls should return the type shown in the overriding signatures here

class C {
    valueOf() { }
}

let c: C;
AssertType(c, "C");

let r1: void = c.valueOf();
AssertType(r1, "void");
AssertType(c.valueOf(), "void");
AssertType(c.valueOf, "() => void");

interface I {
    valueOf(): void;
}

let i: I;
AssertType(i, "I");

let r2: void = i.valueOf();
AssertType(r2, "void");
AssertType(i.valueOf(), "void");
AssertType(i.valueOf, "() => void");

let a = {
AssertType(a, "{ valueOf: () => void; }");
AssertType({    valueOf: () => { }}, "{ valueOf: () => void; }");

    valueOf: () => { 
AssertType(valueOf, "() => void");

AssertType(() => { }, "() => void");
}
}

let r3: void = a.valueOf();
AssertType(r3, "void");
AssertType(a.valueOf(), "void");
AssertType(a.valueOf, "() => void");

let b: {
AssertType(b, "{ valueOf(): void; }");

    valueOf(): void;
AssertType(valueOf, "() => void");
}

let r4: void = b.valueOf();
AssertType(r4, "void");
AssertType(b.valueOf(), "void");
AssertType(b.valueOf, "() => void");


