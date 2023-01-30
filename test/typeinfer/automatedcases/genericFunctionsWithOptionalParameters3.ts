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

// === tests/cases/compiler/genericFunctionsWithOptionalParameters3.ts ===
declare function AssertType(value:any, type:string):void;
class Collection<T> {
    public add(x: T) { }
}
interface Utils {
    fold<T, S>(c?: Collection<T>, folder?: (s: S, t: T) => T, init?: S): T;
    mapReduce<T, U, V>(c: Collection<T>, mapper: (x: T) => U, reducer: (y: U) => V): Collection<V>;
}
let utils: Utils;
AssertType(utils, "Utils");

let c = new Collection<string>();
AssertType(c, "Collection<string>");
AssertType(new Collection<string>(), "Collection<string>");
AssertType(Collection, "typeof Collection");

let r3 = utils.mapReduce(c, (x) => { 
AssertType(r3, "Collection<Date>");
AssertType(utils.mapReduce(c, (x) => { return 1 }, (y) => { return new Date() }), "Collection<Date>");
AssertType(utils.mapReduce, "<T, U, V>(Collection<T>, (T) => U, (U) => V) => Collection<V>");
AssertType(c, "Collection<string>");
AssertType((x) => { return 1 }, "(string) => number");
AssertType(x, "string");
AssertType(1, "int");
AssertType((y) => { return new Date() }, "(number) => Date");
AssertType(y, "number");
AssertType(new Date(), "Date");
AssertType(Date, "DateConstructor");
return 1 }, (y) => { return new Date() });

let r4 = utils.mapReduce(c, (x: string) => { 
AssertType(r4, "Collection<Date>");
AssertType(utils.mapReduce(c, (x: string) => { return 1 }, (y: number) => { return new Date() }), "Collection<Date>");
AssertType(utils.mapReduce, "<T, U, V>(Collection<T>, (T) => U, (U) => V) => Collection<V>");
AssertType(c, "Collection<string>");
AssertType((x: string) => { return 1 }, "(string) => number");
AssertType(x, "string");
AssertType(1, "int");
AssertType((y: number) => { return new Date() }, "(number) => Date");
AssertType(y, "number");
AssertType(new Date(), "Date");
AssertType(Date, "DateConstructor");
return 1 }, (y: number) => { return new Date() });

let f1 = (x: string) => { 
AssertType(f1, "(string) => number");
AssertType((x: string) => { return 1 }, "(string) => number");
AssertType(x, "string");
AssertType(1, "int");
return 1 };

let f2 = (y: number) => { 
AssertType(f2, "(number) => Date");
AssertType((y: number) => { return new Date() }, "(number) => Date");
AssertType(y, "number");
AssertType(new Date(), "Date");
AssertType(Date, "DateConstructor");
return new Date() };

let r5 = utils.mapReduce(c, f1, f2);
AssertType(r5, "Collection<Date>");
AssertType(utils.mapReduce(c, f1, f2), "Collection<Date>");
AssertType(utils.mapReduce, "<T, U, V>(Collection<T>, (T) => U, (U) => V) => Collection<V>");
AssertType(c, "Collection<string>");
AssertType(f1, "(string) => number");
AssertType(f2, "(number) => Date");


