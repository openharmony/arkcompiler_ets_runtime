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

// === tests/cases/compiler/three.ts ===
declare function AssertType(value:any, type:string):void;
// #42504
interface ResizeObserverCallback {
    (entries: ResizeObserverEntry[], observer: ResizeObserver): void;
}
interface ResizeObserverCallback { // duplicate for effect
    (entries: ResizeObserverEntry[], observer: ResizeObserver): void;
}

const resizeObserver = new ResizeObserver(([entry]) => {
AssertType(resizeObserver, "ResizeObserver");
AssertType(new ResizeObserver(([entry]) => {    entry}), "ResizeObserver");
AssertType(ResizeObserver, "{ new (globalThis.ResizeObserverCallback): ResizeObserver; prototype: ResizeObserver; }");
AssertType(([entry]) => {    entry}, "(ResizeObserverEntry[]) => void");
AssertType(entry, "ResizeObserverEntry");

    entry
AssertType(entry, "ResizeObserverEntry");

});
// comment in #35501
interface Callback<T> {
    (error: null, result: T): unknown
    (error: Error, result: null): unknown
}

interface Task<T> {
    (callback: Callback<T>): unknown
}

export function series<T>(tasks: Task<T>[], callback: Callback<T[]>): void {
    let index = 0
AssertType(index, "number");
AssertType(0, "int");

    let results: T[] = []
AssertType(results, "T[]");
AssertType([], "never[]");

    function next() {
AssertType(next, "() => void");

        let task = tasks[index]
AssertType(task, "Task<T>");
AssertType(tasks[index], "Task<T>");
AssertType(tasks, "Task<T>[]");
AssertType(index, "number");

        if (!task) {
AssertType(!task, "boolean");
AssertType(task, "Task<T>");

            callback(null, results)
AssertType(callback(null, results), "unknown");
AssertType(callback, "Callback<T[]>");
AssertType(null, "null");
AssertType(results, "T[]");

        } else {
            task((error, result) => {
AssertType(task((error, result) => {                if (error) {                    callback(error, null)                } else {                    // must use postfix-!, since `error` and `result` don't have a                    // causal relationship when the overloads are combined                    results.push(result!)                    next()                }            }), "unknown");
AssertType(task, "Task<T>");
AssertType((error, result) => {                if (error) {                    callback(error, null)                } else {                    // must use postfix-!, since `error` and `result` don't have a                    // causal relationship when the overloads are combined                    results.push(result!)                    next()                }            }, "(union, union) => void");
AssertType(error, "union");
AssertType(result, "union");

                if (error) {
AssertType(error, "union");

                    callback(error, null)
AssertType(callback(error, null), "unknown");
AssertType(callback, "Callback<T[]>");
AssertType(error, "Error");
AssertType(null, "null");

                } else {
                    // must use postfix-!, since `error` and `result` don't have a
                    // causal relationship when the overloads are combined
                    results.push(result!)
AssertType(results.push(result!), "number");
AssertType(results.push, "(...T[]) => number");
AssertType(result!, "NonNullable<T>");
AssertType(result, "union");

                    next()
AssertType(next(), "void");
AssertType(next, "() => void");
                }
            })
        }
    }
    next()
AssertType(next(), "void");
AssertType(next, "() => void");
}

series([
AssertType(series([    cb => setTimeout(() => cb(null, 1), 300),    cb => setTimeout(() => cb(null, 2), 200),    cb => setTimeout(() => cb(null, 3), 100),], (error, results) => {    if (error) {        console.error(error)    } else {        console.log(results)    }}), "void");
AssertType(series, "<T>(Task<T>[], Callback<T[]>) => void");
AssertType([    cb => setTimeout(() => cb(null, 1), 300),    cb => setTimeout(() => cb(null, 2), 200),    cb => setTimeout(() => cb(null, 3), 100),], "((Callback<unknown>) => number)[]");

    cb => setTimeout(() => cb(null, 1), 300),
AssertType(cb => setTimeout(() => cb(null, 1), 300), "(Callback<unknown>) => number");
AssertType(cb, "Callback<unknown>");
AssertType(setTimeout(() => cb(null, 1), 300), "number");
AssertType(setTimeout, "(TimerHandler, ?union, ...any[]) => number");
AssertType(() => cb(null, 1), "() => unknown");
AssertType(cb(null, 1), "unknown");
AssertType(cb, "Callback<unknown>");
AssertType(null, "null");
AssertType(1, "int");
AssertType(300, "int");

    cb => setTimeout(() => cb(null, 2), 200),
AssertType(cb => setTimeout(() => cb(null, 2), 200), "(Callback<unknown>) => number");
AssertType(cb, "Callback<unknown>");
AssertType(setTimeout(() => cb(null, 2), 200), "number");
AssertType(setTimeout, "(TimerHandler, ?union, ...any[]) => number");
AssertType(() => cb(null, 2), "() => unknown");
AssertType(cb(null, 2), "unknown");
AssertType(cb, "Callback<unknown>");
AssertType(null, "null");
AssertType(2, "int");
AssertType(200, "int");

    cb => setTimeout(() => cb(null, 3), 100),
AssertType(cb => setTimeout(() => cb(null, 3), 100), "(Callback<unknown>) => number");
AssertType(cb, "Callback<unknown>");
AssertType(setTimeout(() => cb(null, 3), 100), "number");
AssertType(setTimeout, "(TimerHandler, ?union, ...any[]) => number");
AssertType(() => cb(null, 3), "() => unknown");
AssertType(cb(null, 3), "unknown");
AssertType(cb, "Callback<unknown>");
AssertType(null, "null");
AssertType(3, "int");
AssertType(100, "int");

], (error, results) => {
AssertType((error, results) => {    if (error) {        console.error(error)    } else {        console.log(results)    }}, "(union, union) => void");
AssertType(error, "union");
AssertType(results, "union");

    if (error) {
AssertType(error, "union");

        console.error(error)
AssertType(console.error(error), "void");
AssertType(console.error, "(...any[]) => void");
AssertType(error, "Error");

    } else {
        console.log(results)
AssertType(console.log(results), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(results, "union");
    }
})


