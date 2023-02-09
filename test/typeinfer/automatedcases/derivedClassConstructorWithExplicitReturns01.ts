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

// === tests/cases/compiler/derivedClassConstructorWithExplicitReturns01.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    cProp = 10;

    foo() { 
AssertType("this never gets used.", "string");
return "this never gets used."; 
}

    constructor(value: number) {
AssertType({            cProp: value,            foo() {                return "well this looks kinda C-ish.";            }        }, "{ cProp: number; foo(): string; }");
        return {

            cProp: value,
AssertType(cProp, "number");
AssertType(value, "number");

            foo() {
AssertType(foo, "() => string");

AssertType("well this looks kinda C-ish.", "string");
                return "well this looks kinda C-ish.";
            }
        }
    }
}

class D extends C {
    dProp = () => this;

    constructor(a = 100) {
        super(a);
AssertType(super(a), "void");
AssertType(super, "typeof C");
AssertType(a, "number");

        if (Math.random() < 0.5) {
AssertType(Math.random() < 0.5, "boolean");
AssertType(Math.random(), "number");
AssertType(Math.random, "() => number");
AssertType(0.5, "double");

            "You win!"
AssertType("You win!", "string");

AssertType({                cProp: 1,                dProp: () => this,                foo() { return "You win!!!!!" }            }, "{ cProp: number; dProp: () => this; foo(): string; }");
            return {

                cProp: 1,
AssertType(cProp, "number");
AssertType(1, "int");

                dProp: () => this,
AssertType(dProp, "() => this");
AssertType(() => this, "() => this");
AssertType(this, "this");

                foo() { 
AssertType(foo, "() => string");
return "You win!!!!!" 

AssertType("You win!!!!!", "string");
}

            };
        }
        else
AssertType(null, "null");
            return null;
    }
}

