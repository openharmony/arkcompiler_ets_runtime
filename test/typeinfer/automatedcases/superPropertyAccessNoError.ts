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

// === tests/cases/conformance/expressions/superPropertyAccess/superPropertyAccessNoError.ts ===
declare function AssertType(value:any, type:string):void;
//super.publicInstanceMemberFunction in constructor of derived class
//super.publicInstanceMemberFunction in instance member function of derived class
//super.publicInstanceMemberFunction in instance member accessor(get and set) of derived class
//super.publicInstanceMemberFunction in lambda in member function
//super.publicStaticMemberFunction in static member function of derived class
//super.publicStaticMemberFunction in static member accessor(get and set) of derived class


class SomeBaseClass {
    public func() {
AssertType('', "string");
        return '';
    }

    static func() {
AssertType(3, "int");
        return 3;
    }

    returnThis() {
AssertType(this, "this");
        return this;
    }
}

class SomeDerivedClass extends SomeBaseClass {
    constructor() {
        super();
AssertType(super(), "void");
AssertType(super, "typeof SomeBaseClass");

        let x = super.func();
AssertType(x, "string");
AssertType(super.func(), "string");
AssertType(super.func, "() => string");
AssertType(super, "SomeBaseClass");

        let x: string;
AssertType(x, "string");
    }

    fn() {
        let x = super.func();
AssertType(x, "string");
AssertType(super.func(), "string");
AssertType(super.func, "() => string");
AssertType(super, "SomeBaseClass");

        let x: string;
AssertType(x, "string");

        let y = () => super.func();
AssertType(y, "() => string");
AssertType(() => super.func(), "() => string");
AssertType(super.func(), "string");
AssertType(super.func, "() => string");
AssertType(super, "SomeBaseClass");
    }

    get a() {
        let x = super.func();
AssertType(x, "string");
AssertType(super.func(), "string");
AssertType(super.func, "() => string");
AssertType(super, "SomeBaseClass");

        let x: string;
AssertType(x, "string");

AssertType(null, "null");
        return null;
    }

    set a(n) {
        let x = super.func();
AssertType(x, "string");
AssertType(super.func(), "string");
AssertType(super.func, "() => string");
AssertType(super, "SomeBaseClass");

        let x: string;
AssertType(x, "string");
    }

    static fn() {
        let x = super.func();
AssertType(x, "number");
AssertType(super.func(), "number");
AssertType(super.func, "() => number");
AssertType(super, "typeof SomeBaseClass");

        let x: number;
AssertType(x, "number");
    }

    static get a() {
        let x = super.func();
AssertType(x, "number");
AssertType(super.func(), "number");
AssertType(super.func, "() => number");
AssertType(super, "typeof SomeBaseClass");

        let x: number;
AssertType(x, "number");

AssertType(null, "null");
        return null;
    }

    static set a(n) {
        let x = super.func();
AssertType(x, "number");
AssertType(super.func(), "number");
AssertType(super.func, "() => number");
AssertType(super, "typeof SomeBaseClass");

        let x: number;
AssertType(x, "number");
    }

    returnThis() {
AssertType(super.returnThis(), "this");
AssertType(super.returnThis, "() => this");
AssertType(super, "SomeBaseClass");
        return super.returnThis();
    }
}

let instance = new SomeDerivedClass();
AssertType(instance, "SomeDerivedClass");
AssertType(new SomeDerivedClass(), "SomeDerivedClass");
AssertType(SomeDerivedClass, "typeof SomeDerivedClass");

instance.
AssertType(instance.returnThis().fn(), "void");
AssertType(instance.returnThis().fn, "() => void");
returnThis().fn();


