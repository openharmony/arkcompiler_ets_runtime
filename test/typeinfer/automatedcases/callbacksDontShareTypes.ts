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

// === tests/cases/compiler/callbacksDontShareTypes.ts ===
declare function AssertType(value:any, type:string):void;
interface Collection<T> {
    length: number;
    add(x: T): void;
    remove(x: T): boolean;
}
interface Combinators {
    map<T, U>(c: Collection<T>, f: (x: T) => U): Collection<U>;
    map<T>(c: Collection<T>, f: (x: T) => any): Collection<any>;
}

let _: Combinators;
AssertType(_, "Combinators");

let c2: Collection<number>;
AssertType(c2, "Collection<number>");

let rf1 = (x: number) => { 
AssertType(rf1, "(number) => string");
AssertType((x: number) => { return x.toFixed() }, "(number) => string");
AssertType(x, "number");
AssertType(x.toFixed(), "string");
AssertType(x.toFixed, "(?number) => string");
return x.toFixed() };

let r1a = _.map(c2, (x) => { 
AssertType(r1a, "Collection<string>");
AssertType(_.map(c2, (x) => { return x.toFixed() }), "Collection<string>");
AssertType(_.map, "{ <T, U>(Collection<T>, (T) => U): Collection<U>; <T>(Collection<T>, (T) => any): Collection<any>; }");
AssertType(c2, "Collection<number>");
AssertType((x) => { return x.toFixed() }, "(number) => string");
AssertType(x, "number");
AssertType(x.toFixed(), "string");
AssertType(x.toFixed, "(?number) => string");
return x.toFixed() });

let r1b = _.map(c2, rf1); // this line should not cause the following 2 to have errors 
AssertType(r1b, "Collection<string>");
AssertType(_.map(c2, rf1), "Collection<string>");
AssertType(_.map, "{ <T, U>(Collection<T>, (T) => U): Collection<U>; <T>(Collection<T>, (T) => any): Collection<any>; }");
AssertType(c2, "Collection<number>");
AssertType(rf1, "(number) => string");

let r5a = _.map<number, string>(c2, (x) => { 
AssertType(r5a, "Collection<string>");
AssertType(_.map<number, string>(c2, (x) => { return x.toFixed() }), "Collection<string>");
AssertType(_.map, "{ <T, U>(Collection<T>, (T) => U): Collection<U>; <T>(Collection<T>, (T) => any): Collection<any>; }");
AssertType(c2, "Collection<number>");
AssertType((x) => { return x.toFixed() }, "(number) => string");
AssertType(x, "number");
AssertType(x.toFixed(), "string");
AssertType(x.toFixed, "(?number) => string");
return x.toFixed() });

let r5b = _.map<number, string>(c2, rf1);
AssertType(r5b, "Collection<string>");
AssertType(_.map<number, string>(c2, rf1), "Collection<string>");
AssertType(_.map, "{ <T, U>(Collection<T>, (T) => U): Collection<U>; <T>(Collection<T>, (T) => any): Collection<any>; }");
AssertType(c2, "Collection<number>");
AssertType(rf1, "(number) => string");


