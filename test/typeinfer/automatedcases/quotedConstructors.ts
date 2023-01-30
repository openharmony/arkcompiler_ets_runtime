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

// === tests/cases/conformance/classes/constructorDeclarations/quotedConstructors.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    "constructor"() {
        console.log(this);
AssertType(console.log(this), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(this, "this");
    }
}

class D {
    'constructor'() {
        console.log(this);
AssertType(console.log(this), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(this, "this");
    }
}

class E {
    ['constructor']() {
        console.log(this);
AssertType(console.log(this), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(this, "this");
    }
}

new class {
AssertType(new class {    "constructor"() {        console.log(this);    }}, "(Anonymous class)");
AssertType(class {    "constructor"() {        console.log(this);    }}, "typeof (Anonymous class)");

    "constructor"() {
        console.log(this);
AssertType(console.log(this), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(this, "this");
    }
};

let o = { "constructor"() {} };
AssertType(o, "{ constructor(): void; }");
AssertType({ "constructor"() {} }, "{ constructor(): void; }");
AssertType("constructor", "() => void");

class F {
    "\x63onstructor"() {
        console.log(this);
AssertType(console.log(this), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(this, "this");
    }
}


