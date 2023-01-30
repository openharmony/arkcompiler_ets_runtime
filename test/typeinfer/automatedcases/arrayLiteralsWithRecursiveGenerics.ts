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

// === tests/cases/conformance/types/typeRelationships/recursiveTypes/arrayLiteralsWithRecursiveGenerics.ts ===
declare function AssertType(value:any, type:string):void;
class List<T> {
    data: T;
    next: List<List<T>>;
}

class DerivedList<U> extends List<U> {
    foo: U;
    // next: List<List<U>>
}

class MyList<T> {
    data: T;
    next: MyList<MyList<T>>;
}

let list: List<number>;
AssertType(list, "List<number>");

let list2: List<string>;
AssertType(list2, "List<string>");

let myList: MyList<number>;
AssertType(myList, "MyList<number>");

let xs = [list, myList]; // {}[]
AssertType(xs, "(union)[]");
AssertType([list, myList], "(union)[]");
AssertType(list, "List<number>");
AssertType(myList, "MyList<number>");

let ys = [list, list2]; // {}[]
AssertType(ys, "(union)[]");
AssertType([list, list2], "(union)[]");
AssertType(list, "List<number>");
AssertType(list2, "List<string>");

let zs = [list, null]; // List<number>[]
AssertType(zs, "List<number>[]");
AssertType([list, null], "List<number>[]");
AssertType(list, "List<number>");
AssertType(null, "null");

let myDerivedList: DerivedList<number>;
AssertType(myDerivedList, "DerivedList<number>");

let as = [list, myDerivedList]; // List<number>[]
AssertType(as, "List<number>[]");
AssertType([list, myDerivedList], "List<number>[]");
AssertType(list, "List<number>");
AssertType(myDerivedList, "DerivedList<number>");


