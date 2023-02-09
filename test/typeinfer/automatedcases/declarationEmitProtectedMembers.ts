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

// === tests/cases/compiler/declarationEmitProtectedMembers.ts ===
declare function AssertType(value:any, type:string):void;
// Class with protected members
class C1 {
    protected x: number;

    protected f() {
AssertType(this.x, "number");
AssertType(this, "this");
        return this.x;
    }

    protected set accessor(a: number) { }
    protected get accessor() { 
AssertType(0, "int");
return 0; 
}

    protected static sx: number;

    protected static sf() {
AssertType(this.sx, "number");
AssertType(this, "typeof C1");
        return this.sx;
    }

    protected static set staticSetter(a: number) { }
    protected static get staticGetter() { 
AssertType(0, "int");
return 0; 
}
}

// Derived class overriding protected members
class C2 extends C1 {
    protected f() {
AssertType(super.f() + this.x, "number");
AssertType(super.f(), "number");
AssertType(super.f, "() => number");
AssertType(super, "C1");
AssertType(this.x, "number");
AssertType(this, "this");
        return super.f() + this.x;
    }
    protected static sf() {
AssertType(super.sf() + this.sx, "number");
AssertType(super.sf(), "number");
AssertType(super.sf, "() => number");
AssertType(super, "typeof C1");
AssertType(this.sx, "number");
AssertType(this, "typeof C2");
        return super.sf() + this.sx;
    }
}

// Derived class making protected members public
class C3 extends C2 {
    x: number;
    static sx: number;
    f() {
AssertType(super.f(), "number");
AssertType(super.f, "() => number");
AssertType(super, "C2");
        return super.f();
    }
    static sf() {
AssertType(super.sf(), "number");
AssertType(super.sf, "() => number");
AssertType(super, "typeof C2");
        return super.sf();
    }

    static get staticGetter() { 
AssertType(1, "int");
return 1; 
}
}

// Protected properties in constructors
class C4 {
    constructor(protected a: number, protected b) { }
}

