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

// === tests/cases/conformance/async/es2017/asyncMethodWithSuperConflict_es6.ts ===
declare function AssertType(value:any, type:string):void;
class A {
    x() {
    }
    y() {
    }
}

class B extends A {
    // async method with only call/get on 'super' does not require a binding
    async simple() {
        const _super = null;
AssertType(_super, "any");
AssertType(null, "null");

        const _superIndex = null;
AssertType(_superIndex, "any");
AssertType(null, "null");

        // call with property access
        super.x();
AssertType(super.x(), "void");
AssertType(super.x, "() => void");
AssertType(super, "A");

        // call additional property.
        super.y();
AssertType(super.y(), "void");
AssertType(super.y, "() => void");
AssertType(super, "A");

        // call with element access
        super["x"]();
AssertType(super["x"](), "void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");

        // property access (read)
        const a = super.x;
AssertType(a, "() => void");
AssertType(super.x, "() => void");
AssertType(super, "A");

        // element access (read)
        const b = super["x"];
AssertType(b, "() => void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");
    }

    // async method with assignment/destructuring on 'super' requires a binding
    async advanced() {
        const _super = null;
AssertType(_super, "any");
AssertType(null, "null");

        const _superIndex = null;
AssertType(_superIndex, "any");
AssertType(null, "null");

        const f = () => {};
AssertType(f, "() => void");
AssertType(() => {}, "() => void");

        // call with property access
        super.x();
AssertType(super.x(), "void");
AssertType(super.x, "() => void");
AssertType(super, "A");

        // call with element access
        super["x"]();
AssertType(super["x"](), "void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");

        // property access (read)
        const a = super.x;
AssertType(a, "() => void");
AssertType(super.x, "() => void");
AssertType(super, "A");

        // element access (read)
        const b = super["x"];
AssertType(b, "() => void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");

        // property access (assign)
        super.x = f;
AssertType(super.x = f, "() => void");
AssertType(super.x, "() => void");
AssertType(super, "A");
AssertType(f, "() => void");

        // element access (assign)
        super["x"] = f;
AssertType(super["x"] = f, "() => void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");
AssertType(f, "() => void");

        // destructuring assign with property access
        ({ f: super.x } = { f });
AssertType(({ f: super.x } = { f }), "{ f: () => void; }");
AssertType({ f: super.x } = { f }, "{ f: () => void; }");
AssertType({ f: super.x }, "{ f: () => void; }");
AssertType(f, "() => void");
AssertType(super.x, "() => void");
AssertType(super, "A");
AssertType({ f }, "{ f: () => void; }");
AssertType(f, "() => void");

        // destructuring assign with element access
        ({ f: super["x"] } = { f });
AssertType(({ f: super["x"] } = { f }), "{ f: () => void; }");
AssertType({ f: super["x"] } = { f }, "{ f: () => void; }");
AssertType({ f: super["x"] }, "{ f: () => void; }");
AssertType(f, "() => void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");
AssertType({ f }, "{ f: () => void; }");
AssertType(f, "() => void");
    }
}


