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

// === tests/cases/conformance/types/typeRelationships/recursiveTypes/recursiveTypesUsedAsFunctionParameters.ts ===
declare function AssertType(value:any, type:string):void;
class List<T> {
    data: T;
    next: List<List<T>>;
}

class MyList<T> {
    data: T;
    next: MyList<MyList<T>>;
}

function foo<T>(x: List<T>);
function foo<U>(x: List<U>); // error, duplicate
function foo<T>(x: List<T>) {
}

function foo2<T>(x: List<T>);
function foo2<U>(x: MyList<U>); // ok, nominally compared with first overload
function foo2<T>(x: any) {
}

function other<T extends List<U>, U>() {
    // error but wrong error
    // BUG 838247
    function foo3<V>(x: T);
AssertType(foo3, "<V>(T) => any");
AssertType(x, "T");

    function foo3<V>(x: MyList<V>) { 
AssertType(foo3, "<V>(T) => any");

AssertType(x, "MyList<V>");
}

    // should be error
    // BUG 838247
    function foo4<V>(x: T);
AssertType(foo4, "<V>(T) => any");
AssertType(x, "T");

    function foo4<V>(x: List<V>) { 
AssertType(foo4, "<V>(T) => any");

AssertType(x, "List<V>");
}

    // ok
    function foo5<V>(x: T): string;
AssertType(foo5, "{ <V>(T): string; <V>(List<V>): number; <V>(MyList<V>): boolean; }");
AssertType(x, "T");

    function foo5<V>(x: List<V>): number;
AssertType(foo5, "{ <V>(T): string; <V>(List<V>): number; <V>(MyList<V>): boolean; }");
AssertType(x, "List<V>");

    function foo5<V>(x: MyList<V>): boolean;
AssertType(foo5, "{ <V>(T): string; <V>(List<V>): number; <V>(MyList<V>): boolean; }");
AssertType(x, "MyList<V>");

    function foo5<V>(x: any): any { 
AssertType(foo5, "{ <V>(T): string; <V>(List<V>): number; <V>(MyList<V>): boolean; }");
return null; 

AssertType(x, "any");

AssertType(null, "null");
}

    let list: List<string>;
AssertType(list, "List<string>");

    let myList: MyList<string>;
AssertType(myList, "MyList<string>");

    let r = foo5(list);
AssertType(r, "number");
AssertType(foo5(list), "number");
AssertType(foo5, "{ <V>(T): string; <V>(List<V>): number; <V>(MyList<V>): boolean; }");
AssertType(list, "List<string>");

    let r2 = foo5(myList);
AssertType(r2, "number");
AssertType(foo5(myList), "number");
AssertType(foo5, "{ <V>(T): string; <V>(List<V>): number; <V>(MyList<V>): boolean; }");
AssertType(myList, "MyList<string>");
}

