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

// === tests/cases/compiler/tooFewArgumentsInGenericFunctionTypedArgument.ts ===
declare function AssertType(value:any, type:string):void;
interface Collection<T, U> {
    length: number;
    add(x: T, y: U): void;
    remove(x: T, y: U): boolean;
}
interface Combinators {
    map<T, U, V>(c: Collection<T,U>, f: (x: T, y: U) => V): Collection<T, V>;
    map<T, U>(c: Collection<T,U>, f: (x: T, y: U) => any): Collection<any, any>;
}
let c2: Collection<number, string>;
AssertType(c2, "Collection<number, string>");

let _: Combinators;
AssertType(_, "Combinators");

let r1a = _.map(c2, (x) => { 
AssertType(r1a, "Collection<number, string>");
AssertType(_.map(c2, (x) => { return x.toFixed() }), "Collection<number, string>");
AssertType(_.map, "{ <T, U, V>(Collection<T, U>, (T, U) => V): Collection<T, V>; <T, U>(Collection<T, U>, (T, U) => any): Collection<any, any>; }");
AssertType(c2, "Collection<number, string>");
AssertType((x) => { return x.toFixed() }, "(number) => string");
AssertType(x, "number");
AssertType(x.toFixed(), "string");
AssertType(x.toFixed, "(?number) => string");
return x.toFixed() }); 

let rf1 = (x: number) => { 
AssertType(rf1, "(number) => string");
AssertType((x: number) => { return x.toFixed() }, "(number) => string");
AssertType(x, "number");
AssertType(x.toFixed(), "string");
AssertType(x.toFixed, "(?number) => string");
return x.toFixed() };

let r1b = _.map(c2, rf1); 
AssertType(r1b, "Collection<number, string>");
AssertType(_.map(c2, rf1), "Collection<number, string>");
AssertType(_.map, "{ <T, U, V>(Collection<T, U>, (T, U) => V): Collection<T, V>; <T, U>(Collection<T, U>, (T, U) => any): Collection<any, any>; }");
AssertType(c2, "Collection<number, string>");
AssertType(rf1, "(number) => string");



