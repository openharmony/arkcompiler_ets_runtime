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

// === tests/cases/conformance/controlFlow/controlFlowInstanceOfGuardPrimitives.ts ===
declare function AssertType(value:any, type:string):void;
function distinguish(thing: string | number | Date) {
    if (thing instanceof Object) {
AssertType(thing instanceof Object, "boolean");
AssertType(thing, "union");
AssertType(Object, "ObjectConstructor");

        console.log("Aha!! It's a Date in " + thing.getFullYear());
AssertType(console.log("Aha!! It's a Date in " + thing.getFullYear()), "void");
AssertType(console.log, "(...any[]) => void");
AssertType("Aha!! It's a Date in " + thing.getFullYear(), "string");
AssertType("Aha!! It's a Date in ", "string");
AssertType(thing.getFullYear(), "number");
AssertType(thing.getFullYear, "() => number");

    } else if (typeof thing === 'string') {
AssertType(typeof thing === 'string', "boolean");
AssertType(typeof thing, "union");
AssertType(thing, "union");
AssertType('string', "string");

        console.log("Aha!! It's a string of length " + thing.length);
AssertType(console.log("Aha!! It's a string of length " + thing.length), "void");
AssertType(console.log, "(...any[]) => void");
AssertType("Aha!! It's a string of length " + thing.length, "string");
AssertType("Aha!! It's a string of length ", "string");
AssertType(thing.length, "number");

    } else {
        console.log("Aha!! It's the number " + thing.toPrecision(3));
AssertType(console.log("Aha!! It's the number " + thing.toPrecision(3)), "void");
AssertType(console.log, "(...any[]) => void");
AssertType("Aha!! It's the number " + thing.toPrecision(3), "string");
AssertType("Aha!! It's the number ", "string");
AssertType(thing.toPrecision(3), "string");
AssertType(thing.toPrecision, "(?number) => string");
AssertType(3, "int");
    }
}

distinguish(new Date());
AssertType(distinguish(new Date()), "void");
AssertType(distinguish, "(union) => void");
AssertType(new Date(), "Date");
AssertType(Date, "DateConstructor");

distinguish("beef");
AssertType(distinguish("beef"), "void");
AssertType(distinguish, "(union) => void");
AssertType("beef", "string");

distinguish(3.14159265);
AssertType(distinguish(3.14159265), "void");
AssertType(distinguish, "(union) => void");
AssertType(3.14159265, "double");


