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

// === tests/cases/compiler/contravariantInferenceAndTypeGuard.ts ===
declare function AssertType(value:any, type:string):void;
interface ListItem<TData> {
    prev: ListItem<TData> | null;
    next: ListItem<TData> | null;
    data: TData;
}
type IteratorFn<TData, TResult, TContext = List<TData>> = (this: TContext, item: TData, node: ListItem<TData>, list: List<TData>) => TResult;
type FilterFn<TData, TResult extends TData, TContext = List<TData>> = (this: TContext, item: TData, node: ListItem<TData>, list: List<TData>) => item is TResult;

declare class List<TData> {
    filter<TContext, TResult extends TData>(fn: FilterFn<TData, TResult, TContext>, context: TContext): List<TResult>;
    filter<TResult extends TData>(fn: FilterFn<TData, TResult>): List<TResult>;
    filter<TContext>(fn: IteratorFn<TData, boolean, TContext>, context: TContext): List<TData>;
    filter(fn: IteratorFn<TData, boolean>): List<TData>;
}
interface Test {
    a: string;
}
const list2 = new List<Test | null>();
AssertType(list2, "List<union>");
AssertType(new List<Test | null>(), "List<union>");
AssertType(List, "typeof List");
AssertType(null, "null");

const filter1 = list2.filter(function(item, node, list): item is Test {
AssertType(filter1, "List<Test>");
AssertType(list2.filter, "{ <TContext, TResult extends union>(FilterFn<union, TResult, TContext>, TContext): List<TResult>; <TResult extends union>(FilterFn<union, TResult, List<union>>): List<TResult>; <TContext>(IteratorFn<union, boolean, TContext>, TContext): List<union>; (IteratorFn<union, boolean, List<union>>): List<union>; }");
AssertType(function(item, node, list): item is Test {    this.b; // $ExpectType string    item; // $ExpectType Test | null    node; // $ExpectType ListItem<Test | null>    list; // $ExpectType List<Test | null>    return !!item;}, "({ b: string; }, union, ListItem<union>, List<union>) => item is Test");
AssertType(item, "union");
AssertType(node, "ListItem<union>");
AssertType(list, "List<union>");
AssertType(list2.filter(function(item, node, list): item is Test {    this.b; // $ExpectType string    item; // $ExpectType Test | null    node; // $ExpectType ListItem<Test | null>    list; // $ExpectType List<Test | null>    return !!item;}, {b: 'c'}), "List<Test>");

    this.b; // $ExpectType string
AssertType(this.b, "string");
AssertType(this, "{ b: string; }");

    item; // $ExpectType Test | null
AssertType(item, "union");

    node; // $ExpectType ListItem<Test | null>
AssertType(node, "ListItem<union>");

    list; // $ExpectType List<Test | null>
AssertType(list, "List<union>");

AssertType(!!item, "boolean");
AssertType(!item, "boolean");
AssertType(item, "union");
    return !!item;

}, {b: 'c'});
AssertType({b: 'c'}, "{ b: string; }");
AssertType(b, "string");
AssertType('c', "string");

const x: List<Test> = filter1; // $ExpectType List<Test>
AssertType(x, "List<Test>");
AssertType(filter1, "List<Test>");


