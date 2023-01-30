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

// === tests/cases/conformance/async/es6/asyncMethodWithSuper_es6.ts ===
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

        // property access in arrow
        (() => super.x());
AssertType((() => super.x()), "() => void");
AssertType(() => super.x(), "() => void");
AssertType(super.x(), "void");
AssertType(super.x, "() => void");
AssertType(super, "A");

        // element access in arrow
        (() => super["x"]());
AssertType((() => super["x"]()), "() => void");
AssertType(() => super["x"](), "() => void");
AssertType(super["x"](), "void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");

        // property access in async arrow
        (async () => super.x());
AssertType((async () => super.x()), "() => Promise<void>");
AssertType(async () => super.x(), "() => Promise<void>");
AssertType(super.x(), "void");
AssertType(super.x, "() => void");
AssertType(super, "A");

        // element access in async arrow
        (async () => super["x"]());
AssertType((async () => super["x"]()), "() => Promise<void>");
AssertType(async () => super["x"](), "() => Promise<void>");
AssertType(super["x"](), "void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");
    }

    async property_access_only_read_only() {
        // call with property access
        super.x();
AssertType(super.x(), "void");
AssertType(super.x, "() => void");
AssertType(super, "A");

        // property access (read)
        const a = super.x;
AssertType(a, "() => void");
AssertType(super.x, "() => void");
AssertType(super, "A");

        // property access in arrow
        (() => super.x());
AssertType((() => super.x()), "() => void");
AssertType(() => super.x(), "() => void");
AssertType(super.x(), "void");
AssertType(super.x, "() => void");
AssertType(super, "A");

        // property access in async arrow
        (async () => super.x());
AssertType((async () => super.x()), "() => Promise<void>");
AssertType(async () => super.x(), "() => Promise<void>");
AssertType(super.x(), "void");
AssertType(super.x, "() => void");
AssertType(super, "A");
    }

    async property_access_only_write_only() {
        const f = () => {};
AssertType(f, "() => void");
AssertType(() => {}, "() => void");

        // property access (assign)
        super.x = f;
AssertType(super.x = f, "() => void");
AssertType(super.x, "() => void");
AssertType(super, "A");
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

        // property access (assign) in arrow
        (() => super.x = f);
AssertType((() => super.x = f), "() => () => void");
AssertType(() => super.x = f, "() => () => void");
AssertType(super.x = f, "() => void");
AssertType(super.x, "() => void");
AssertType(super, "A");
AssertType(f, "() => void");

        // property access (assign) in async arrow
        (async () => super.x = f);
AssertType((async () => super.x = f), "() => Promise<() => void>");
AssertType(async () => super.x = f, "() => Promise<() => void>");
AssertType(super.x = f, "() => void");
AssertType(super.x, "() => void");
AssertType(super, "A");
AssertType(f, "() => void");
    }

    async element_access_only_read_only() {
        // call with element access
        super["x"]();
AssertType(super["x"](), "void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");

        // element access (read)
        const a = super["x"];
AssertType(a, "() => void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");

        // element access in arrow
        (() => super["x"]());
AssertType((() => super["x"]()), "() => void");
AssertType(() => super["x"](), "() => void");
AssertType(super["x"](), "void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");

        // element access in async arrow
        (async () => super["x"]());
AssertType((async () => super["x"]()), "() => Promise<void>");
AssertType(async () => super["x"](), "() => Promise<void>");
AssertType(super["x"](), "void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");
    }

    async element_access_only_write_only() {
        const f = () => {};
AssertType(f, "() => void");
AssertType(() => {}, "() => void");

        // element access (assign)
        super["x"] = f;
AssertType(super["x"] = f, "() => void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");
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

        // element access (assign) in arrow
        (() => super["x"] = f);
AssertType((() => super["x"] = f), "() => () => void");
AssertType(() => super["x"] = f, "() => () => void");
AssertType(super["x"] = f, "() => void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");
AssertType(f, "() => void");

        // element access (assign) in async arrow
        (async () => super["x"] = f);
AssertType((async () => super["x"] = f), "() => Promise<() => void>");
AssertType(async () => super["x"] = f, "() => Promise<() => void>");
AssertType(super["x"] = f, "() => void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");
AssertType(f, "() => void");
    }

    async * property_access_only_read_only_in_generator() {
        // call with property access
        super.x();
AssertType(super.x(), "void");
AssertType(super.x, "() => void");
AssertType(super, "A");

        // property access (read)
        const a = super.x;
AssertType(a, "() => void");
AssertType(super.x, "() => void");
AssertType(super, "A");

        // property access in arrow
        (() => super.x());
AssertType((() => super.x()), "() => void");
AssertType(() => super.x(), "() => void");
AssertType(super.x(), "void");
AssertType(super.x, "() => void");
AssertType(super, "A");

        // property access in async arrow
        (async () => super.x());
AssertType((async () => super.x()), "() => Promise<void>");
AssertType(async () => super.x(), "() => Promise<void>");
AssertType(super.x(), "void");
AssertType(super.x, "() => void");
AssertType(super, "A");
    }

    async * property_access_only_write_only_in_generator() {
        const f = () => {};
AssertType(f, "() => void");
AssertType(() => {}, "() => void");

        // property access (assign)
        super.x = f;
AssertType(super.x = f, "() => void");
AssertType(super.x, "() => void");
AssertType(super, "A");
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

        // property access (assign) in arrow
        (() => super.x = f);
AssertType((() => super.x = f), "() => () => void");
AssertType(() => super.x = f, "() => () => void");
AssertType(super.x = f, "() => void");
AssertType(super.x, "() => void");
AssertType(super, "A");
AssertType(f, "() => void");

        // property access (assign) in async arrow
        (async () => super.x = f);
AssertType((async () => super.x = f), "() => Promise<() => void>");
AssertType(async () => super.x = f, "() => Promise<() => void>");
AssertType(super.x = f, "() => void");
AssertType(super.x, "() => void");
AssertType(super, "A");
AssertType(f, "() => void");
    }

    async * element_access_only_read_only_in_generator() {
        // call with element access
        super["x"]();
AssertType(super["x"](), "void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");

        // element access (read)
        const a = super["x"];
AssertType(a, "() => void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");

        // element access in arrow
        (() => super["x"]());
AssertType((() => super["x"]()), "() => void");
AssertType(() => super["x"](), "() => void");
AssertType(super["x"](), "void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");

        // element access in async arrow
        (async () => super["x"]());
AssertType((async () => super["x"]()), "() => Promise<void>");
AssertType(async () => super["x"](), "() => Promise<void>");
AssertType(super["x"](), "void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");
    }

    async * element_access_only_write_only_in_generator() {
        const f = () => {};
AssertType(f, "() => void");
AssertType(() => {}, "() => void");

        // element access (assign)
        super["x"] = f;
AssertType(super["x"] = f, "() => void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");
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

        // element access (assign) in arrow
        (() => super["x"] = f);
AssertType((() => super["x"] = f), "() => () => void");
AssertType(() => super["x"] = f, "() => () => void");
AssertType(super["x"] = f, "() => void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");
AssertType(f, "() => void");

        // element access (assign) in async arrow
        (async () => super["x"] = f);
AssertType((async () => super["x"] = f), "() => Promise<() => void>");
AssertType(async () => super["x"] = f, "() => Promise<() => void>");
AssertType(super["x"] = f, "() => void");
AssertType(super["x"], "() => void");
AssertType(super, "A");
AssertType("x", "string");
AssertType(f, "() => void");
    }
}

// https://github.com/microsoft/TypeScript/issues/46828
class Base {
    set setter(x: any) {}
    get getter(): any { 
return; }
    method(x: string): any {}

    static set setter(x: any) {}
    static get getter(): any { 
return; }
    static method(x: string): any {}
}

class Derived extends Base {
    a() { 
AssertType(async () => super.method(''), "() => Promise<any>");
return async () => super.method('') 

AssertType(super.method(''), "any");

AssertType(super.method, "(string) => any");

AssertType(super, "Base");

AssertType('', "string");
}

    b() { 
AssertType(async () => super.getter, "() => Promise<any>");
return async () => super.getter 

AssertType(super.getter, "any");

AssertType(super, "Base");
}

    c() { 
AssertType(async () => super.setter = '', "() => Promise<string>");
return async () => super.setter = '' 

AssertType(super.setter = '', "string");

AssertType(super.setter, "any");

AssertType(super, "Base");

AssertType('', "string");
}

    d() { 
AssertType(async () => super["method"](''), "() => Promise<any>");
return async () => super["method"]('') 

AssertType(super["method"](''), "any");

AssertType(super["method"], "(string) => any");

AssertType(super, "Base");

AssertType("method", "string");

AssertType('', "string");
}

    e() { 
AssertType(async () => super["getter"], "() => Promise<any>");
return async () => super["getter"] 

AssertType(super["getter"], "any");

AssertType(super, "Base");

AssertType("getter", "string");
}

    f() { 
AssertType(async () => super["setter"] = '', "() => Promise<string>");
return async () => super["setter"] = '' 

AssertType(super["setter"] = '', "string");

AssertType(super["setter"], "any");

AssertType(super, "Base");

AssertType("setter", "string");

AssertType('', "string");
}

    static a() { 
AssertType(async () => super.method(''), "() => Promise<any>");
return async () => super.method('') 

AssertType(super.method(''), "any");

AssertType(super.method, "(string) => any");

AssertType(super, "typeof Base");

AssertType('', "string");
}

    static b() { 
AssertType(async () => super.getter, "() => Promise<any>");
return async () => super.getter 

AssertType(super.getter, "any");

AssertType(super, "typeof Base");
}

    static c() { 
AssertType(async () => super.setter = '', "() => Promise<string>");
return async () => super.setter = '' 

AssertType(super.setter = '', "string");

AssertType(super.setter, "any");

AssertType(super, "typeof Base");

AssertType('', "string");
}

    static d() { 
AssertType(async () => super["method"](''), "() => Promise<any>");
return async () => super["method"]('') 

AssertType(super["method"](''), "any");

AssertType(super["method"], "(string) => any");

AssertType(super, "typeof Base");

AssertType("method", "string");

AssertType('', "string");
}

    static e() { 
AssertType(async () => super["getter"], "() => Promise<any>");
return async () => super["getter"] 

AssertType(super["getter"], "any");

AssertType(super, "typeof Base");

AssertType("getter", "string");
}

    static f() { 
AssertType(async () => super["setter"] = '', "() => Promise<string>");
return async () => super["setter"] = '' 

AssertType(super["setter"] = '', "string");

AssertType(super["setter"], "any");

AssertType(super, "typeof Base");

AssertType("setter", "string");

AssertType('', "string");
}
}


