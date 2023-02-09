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

// === tests/cases/compiler/constructorWithCapturedSuper.ts ===
declare function AssertType(value:any, type:string):void;
let oneA: A;
AssertType(oneA, "A");

class A {
    constructor() {
AssertType(oneA, "A");
        return oneA;
    }
}

class B extends A {
    constructor(x: number) {
        super();
AssertType(super(), "void");
AssertType(super, "typeof A");

        if (x === 1) {
AssertType(x === 1, "boolean");
AssertType(x, "number");
AssertType(1, "int");

            return;
        }
        while (x < 2) {
AssertType(x < 2, "boolean");
AssertType(x, "number");
AssertType(2, "int");

            return;
        }
        try {
            return
        }
        catch (e) {
AssertType(e, "any");

            return;
        }
        finally {
            return;
        }
    }
}

class C extends A {
    constructor(x: number) {
        super();
AssertType(super(), "void");
AssertType(super, "typeof A");

        for (let i = 0; i < 10; ++i) {
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < 10, "boolean");
AssertType(i, "number");
AssertType(10, "int");
AssertType(++i, "number");
AssertType(i, "number");

            () => i + x;
AssertType(() => i + x, "() => number");
AssertType(i + x, "number");
AssertType(i, "number");
AssertType(x, "number");

            if (x === 1) {
AssertType(x === 1, "boolean");
AssertType(x, "number");
AssertType(1, "int");

                return;
            } 
        }
    }
}

class D extends A {
    constructor(x: number) {
        super();
AssertType(super(), "void");
AssertType(super, "typeof A");

        () => {
AssertType(() => {            return;        }, "() => void");

            return;
        }
        function foo() {
AssertType(foo, "() => void");

            return;
        }
    }
}

