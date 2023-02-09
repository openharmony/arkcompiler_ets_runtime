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

// === tests/cases/compiler/objectLiteralArraySpecialization.ts ===
declare function AssertType(value:any, type:string):void;
declare function create<T>(initialValues?: T[]): MyArrayWrapper<T>;
interface MyArrayWrapper<T> {
	constructor(initialItems?: T[]);
	doSomething(predicate: (x: T, y: T) => boolean): void;
}
let thing = create([ { name: "bob", id: 24 }, { name: "doug", id: 32 } ]); // should not error
AssertType(thing, "MyArrayWrapper<{ name: string; id: number; }>");
AssertType(create([ { name: "bob", id: 24 }, { name: "doug", id: 32 } ]), "MyArrayWrapper<{ name: string; id: number; }>");
AssertType(create, "<T>(?T[]) => MyArrayWrapper<T>");
AssertType([ { name: "bob", id: 24 }, { name: "doug", id: 32 } ], "{ name: string; id: number; }[]");
AssertType({ name: "bob", id: 24 }, "{ name: string; id: number; }");
AssertType(name, "string");
AssertType("bob", "string");
AssertType(id, "number");
AssertType(24, "int");
AssertType({ name: "doug", id: 32 }, "{ name: string; id: number; }");
AssertType(name, "string");
AssertType("doug", "string");
AssertType(id, "number");
AssertType(32, "int");

thing.doSomething((x, y) => x.name === "bob"); // should not error
AssertType(thing.doSomething((x, y) => x.name === "bob"), "void");
AssertType(thing.doSomething, "(({ name: string; id: number; }, { name: string; id: number; }) => boolean) => void");
AssertType((x, y) => x.name === "bob", "({ name: string; id: number; }, { name: string; id: number; }) => boolean");
AssertType(x, "{ name: string; id: number; }");
AssertType(y, "{ name: string; id: number; }");
AssertType(x.name === "bob", "boolean");
AssertType(x.name, "string");
AssertType("bob", "string");


