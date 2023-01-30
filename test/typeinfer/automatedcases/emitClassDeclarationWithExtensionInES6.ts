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

// === tests/cases/conformance/es6/classDeclaration/emitClassDeclarationWithExtensionInES6.ts ===
declare function AssertType(value:any, type:string):void;
class B {
    baz(a: string, y = 10) { }
}
class C extends B {
    foo() { }
    baz(a: string, y:number) {
        super.baz(a, y);
AssertType(super.baz(a, y), "void");
AssertType(super.baz, "(string, ?number) => void");
AssertType(super, "B");
AssertType(a, "string");
AssertType(y, "number");
    }
}
class D extends C {
    constructor() {
        super();
AssertType(super(), "void");
AssertType(super, "typeof C");
    }

    foo() {
        super.foo();
AssertType(super.foo(), "void");
AssertType(super.foo, "() => void");
AssertType(super, "C");
    }

    baz() {
        super.baz("hello", 10);
AssertType(super.baz("hello", 10), "void");
AssertType(super.baz, "(string, number) => void");
AssertType(super, "C");
AssertType("hello", "string");
AssertType(10, "int");
    }
}


