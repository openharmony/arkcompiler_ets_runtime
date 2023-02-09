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

// === tests/cases/compiler/asyncFunctionReturnType.ts ===
declare function AssertType(value:any, type:string):void;
async function fAsync() {
    // Without explicit type annotation, this is just an array.
AssertType([1, true], "(union)[]");
AssertType(1, "int");
AssertType(true, "boolean");
    return [1, true];
}

async function fAsyncExplicit(): Promise<[number, boolean]> {
    // This is contextually typed as a tuple.
AssertType([1, true], "[number, true]");
AssertType(1, "int");
AssertType(true, "boolean");
    return [1, true];
}

// https://github.com/Microsoft/TypeScript/issues/13128
interface Obj {
    stringProp: string;
    anyProp: any;
}

async function fIndexedTypeForStringProp(obj: Obj): Promise<Obj["stringProp"]> {
AssertType(obj.stringProp, "string");
    return obj.stringProp;
}

async function fIndexedTypeForPromiseOfStringProp(obj: Obj): Promise<Obj["stringProp"]> {
AssertType(Promise.resolve(obj.stringProp), "Promise<string>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(obj.stringProp, "string");
    return Promise.resolve(obj.stringProp);
}

async function fIndexedTypeForExplicitPromiseOfStringProp(obj: Obj): Promise<Obj["stringProp"]> {
AssertType(Promise.resolve<Obj["stringProp"]>(obj.stringProp), "Promise<string>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(obj.stringProp, "string");
    return Promise.resolve<Obj["stringProp"]>(obj.stringProp);
}

async function fIndexedTypeForAnyProp(obj: Obj): Promise<Obj["anyProp"]> {
AssertType(obj.anyProp, "any");
    return obj.anyProp;
}

async function fIndexedTypeForPromiseOfAnyProp(obj: Obj): Promise<Obj["anyProp"]> {
AssertType(Promise.resolve(obj.anyProp), "Promise<any>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(obj.anyProp, "any");
    return Promise.resolve(obj.anyProp);
}

async function fIndexedTypeForExplicitPromiseOfAnyProp(obj: Obj): Promise<Obj["anyProp"]> {
AssertType(Promise.resolve<Obj["anyProp"]>(obj.anyProp), "Promise<any>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(obj.anyProp, "any");
    return Promise.resolve<Obj["anyProp"]>(obj.anyProp);
}

async function fGenericIndexedTypeForStringProp<TObj extends Obj>(obj: TObj): Promise<TObj["stringProp"]> {
AssertType(obj.stringProp, "string");
    return obj.stringProp;
}

async function fGenericIndexedTypeForPromiseOfStringProp<TObj extends Obj>(obj: TObj): Promise<TObj["stringProp"]> {
AssertType(Promise.resolve(obj.stringProp), "Promise<string>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(obj.stringProp, "string");
    return Promise.resolve(obj.stringProp);
}

async function fGenericIndexedTypeForExplicitPromiseOfStringProp<TObj extends Obj>(obj: TObj): Promise<TObj["stringProp"]> {
AssertType(Promise.resolve<TObj["stringProp"]>(obj.stringProp), "Promise<Awaited<TObj["stringProp"]>>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(obj.stringProp, "string");
    return Promise.resolve<TObj["stringProp"]>(obj.stringProp);
}

async function fGenericIndexedTypeForAnyProp<TObj extends Obj>(obj: TObj): Promise<TObj["anyProp"]> {
AssertType(obj.anyProp, "any");
    return obj.anyProp;
}

async function fGenericIndexedTypeForPromiseOfAnyProp<TObj extends Obj>(obj: TObj): Promise<TObj["anyProp"]> {
AssertType(Promise.resolve(obj.anyProp), "Promise<any>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(obj.anyProp, "any");
    return Promise.resolve(obj.anyProp);
}

async function fGenericIndexedTypeForExplicitPromiseOfAnyProp<TObj extends Obj>(obj: TObj): Promise<TObj["anyProp"]> {
AssertType(Promise.resolve<TObj["anyProp"]>(obj.anyProp), "Promise<Awaited<TObj["anyProp"]>>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(obj.anyProp, "any");
    return Promise.resolve<TObj["anyProp"]>(obj.anyProp);
}

async function fGenericIndexedTypeForKProp<TObj extends Obj, K extends keyof TObj>(obj: TObj, key: K): Promise<TObj[K]> {
AssertType(obj[key], "TObj[K]");
AssertType(obj, "TObj");
AssertType(key, "K");
    return obj[key];
}

async function fGenericIndexedTypeForPromiseOfKProp<TObj extends Obj, K extends keyof TObj>(obj: TObj, key: K): Promise<TObj[K]> {
AssertType(Promise.resolve(obj[key]), "Promise<Awaited<TObj[K]>>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(obj[key], "TObj[K]");
AssertType(obj, "TObj");
AssertType(key, "K");
    return Promise.resolve(obj[key]);
}

async function fGenericIndexedTypeForExplicitPromiseOfKProp<TObj extends Obj, K extends keyof TObj>(obj: TObj, key: K): Promise<TObj[K]> {
AssertType(Promise.resolve<TObj[K]>(obj[key]), "Promise<Awaited<TObj[K]>>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(obj[key], "TObj[K]");
AssertType(obj, "TObj");
AssertType(key, "K");
    return Promise.resolve<TObj[K]>(obj[key]);
}

