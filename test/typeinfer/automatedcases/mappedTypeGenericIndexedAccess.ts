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

// === tests/cases/compiler/mappedTypeGenericIndexedAccess.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #49242

type Types = {
    first: { a1: true };
    second: { a2: true };
    third: { a3: true };
}

class Test {
    entries: { [T in keyof Types]?: Types[T][] };

    constructor() {
        this.entries = {};
AssertType(this.entries = {}, "{}");
AssertType(this.entries, "{ first?: union; second?: union; third?: union; }");
AssertType(this, "this");
AssertType({}, "{}");
    }

    addEntry<T extends keyof Types>(name: T, entry: Types[T]) {
        if (!this.entries[name]) {
AssertType(!this.entries[name], "boolean");
AssertType(this.entries[name], "{ first?: union; second?: union; third?: union; }[T]");
AssertType(this.entries, "{ first?: union; second?: union; third?: union; }");
AssertType(this, "this");
AssertType(name, "T");

            this.entries[name] = [];
AssertType(this.entries[name] = [], "never[]");
AssertType(this.entries[name], "{ first?: union; second?: union; third?: union; }[T]");
AssertType(this.entries, "{ first?: union; second?: union; third?: union; }");
AssertType(this, "this");
AssertType(name, "T");
AssertType([], "never[]");
        }
        this.entries[name]?.push(entry);
AssertType(this.entries[name]?.push(entry), "union");
AssertType(this.entries[name]?.push, "union");
AssertType(this.entries[name], "union");
AssertType(this.entries, "{ first?: union; second?: union; third?: union; }");
AssertType(this, "this");
AssertType(name, "T");
AssertType(entry, "Types[T]");
    }
}

// Repro from #49338

type TypesMap = {
    [0]: { foo: 'bar'; };
    [1]: { a: 'b'; };
};

type P<T extends keyof TypesMap> = { t: T; } & TypesMap[T];

type TypeHandlers = {
    [T in keyof TypesMap]?: (p: P<T>) => void;
};

const typeHandlers: TypeHandlers = {
AssertType(typeHandlers, "TypeHandlers");
AssertType({    [0]: (p) => console.log(p.foo),    [1]: (p) => console.log(p.a),}, "{ 0: (P<0>) => void; 1: (P<1>) => void; }");

    [0]: (p) => console.log(p.foo),
AssertType([0], "(P<0>) => void");
AssertType(0, "int");
AssertType((p) => console.log(p.foo), "(P<0>) => void");
AssertType(p, "P<0>");
AssertType(console.log(p.foo), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(p.foo, "string");

    [1]: (p) => console.log(p.a),
AssertType([1], "(P<1>) => void");
AssertType(1, "int");
AssertType((p) => console.log(p.a), "(P<1>) => void");
AssertType(p, "P<1>");
AssertType(console.log(p.a), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(p.a, "string");

};

const onSomeEvent = <T extends keyof TypesMap>(p: P<T>) =>
AssertType(onSomeEvent, "<T extends keyof TypesMap>(P<T>) => union");
AssertType(<T extends keyof TypesMap>(p: P<T>) =>    typeHandlers[p.t]?.(p), "<T extends keyof TypesMap>(P<T>) => union");
AssertType(p, "P<T>");

    typeHandlers[p.t]?.(p);
AssertType(typeHandlers[p.t]?.(p), "union");
AssertType(typeHandlers[p.t], "union");
AssertType(typeHandlers, "TypeHandlers");
AssertType(p.t, "T");
AssertType(p, "P<T>");


