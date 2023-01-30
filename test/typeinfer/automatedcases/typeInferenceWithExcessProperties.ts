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

// === tests/cases/compiler/typeInferenceWithExcessProperties.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #17041

interface Named {
    name: string;
}

function parrot<T extends Named>(obj: T): T {
AssertType(obj, "T");
    return obj;
}


parrot({
AssertType(parrot({    name: "TypeScript",}), "{ name: string; }");
AssertType(parrot, "<T extends Named>(T) => T");
AssertType({    name: "TypeScript",}, "{ name: string; }");

    name: "TypeScript",
AssertType(name, "string");
AssertType("TypeScript", "string");

});

parrot({
AssertType(parrot({    name: "TypeScript",    age: 5,}), "{ name: string; age: number; }");
AssertType(parrot, "<T extends Named>(T) => T");
AssertType({    name: "TypeScript",    age: 5,}, "{ name: string; age: number; }");

    name: "TypeScript",
AssertType(name, "string");
AssertType("TypeScript", "string");

    age: 5,
AssertType(age, "number");
AssertType(5, "int");

});

parrot({
AssertType(parrot({    name: "TypeScript",    age: function () { },}), "{ name: string; age: () => void; }");
AssertType(parrot, "<T extends Named>(T) => T");
AssertType({    name: "TypeScript",    age: function () { },}, "{ name: string; age: () => void; }");

    name: "TypeScript",
AssertType(name, "string");
AssertType("TypeScript", "string");

    age: function () { },
AssertType(age, "() => void");
AssertType(function () { }, "() => void");

});

parrot({
AssertType(parrot({    name: "TypeScript",    sayHello() {    },}), "{ name: string; sayHello(): void; }");
AssertType(parrot, "<T extends Named>(T) => T");
AssertType({    name: "TypeScript",    sayHello() {    },}, "{ name: string; sayHello(): void; }");

    name: "TypeScript",
AssertType(name, "string");
AssertType("TypeScript", "string");

    sayHello() {
AssertType(sayHello, "() => void");

    },
});


