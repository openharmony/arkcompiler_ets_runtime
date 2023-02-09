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

// === tests/cases/conformance/classes/members/inheritanceAndOverriding/derivedClassOverridesProtectedMembers.ts ===
declare function AssertType(value:any, type:string):void;
let x: { foo: string; 
AssertType(x, "{ foo: string; }");

AssertType(foo, "string");
}

let y: { foo: string; bar: string; 
AssertType(y, "{ foo: string; bar: string; }");

AssertType(foo, "string");

AssertType(bar, "string");
}

class Base {
    protected a: typeof x;
    protected b(a: typeof x) { }
    protected get c() { 
AssertType(x, "{ foo: string; }");
return x; 
}

    protected set c(v: typeof x) { }
    protected d: (a: typeof x) => void;

    protected static r: typeof x;
    protected static s(a: typeof x) { }
    protected static get t() { 
AssertType(x, "{ foo: string; }");
return x; 
}

    protected static set t(v: typeof x) { }
    protected static u: (a: typeof x) => void;

    constructor(a: typeof x) { }
}

class Derived extends Base {
    protected a: typeof y;
    protected b(a: typeof y) { }
    protected get c() { 
AssertType(y, "{ foo: string; bar: string; }");
return y; 
}

    protected set c(v: typeof y) { }
    protected d: (a: typeof y) => void;

    protected static r: typeof y;
    protected static s(a: typeof y) { }
    protected static get t() { 
AssertType(y, "{ foo: string; bar: string; }");
return y; 
}

    protected static set t(a: typeof y) { }
    protected static u: (a: typeof y) => void;

    constructor(a: typeof y) { super(x) 
AssertType(super(x), "void");

AssertType(super, "typeof Base");

AssertType(x, "{ foo: string; }");
}
}


