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

// === tests/cases/compiler/arrowFunctionParsingGenericInObject.ts ===
declare function AssertType(value:any, type:string):void;
const fn1 = () => ({
AssertType(fn1, "() => { test: <T = undefined>(T) => T; extraValue: () => void; }");
AssertType(() => ({    test: <T = undefined>(value: T): T => value,    extraValue: () => {},}), "() => { test: <T = undefined>(T) => T; extraValue: () => void; }");
AssertType(({    test: <T = undefined>(value: T): T => value,    extraValue: () => {},}), "{ test: <T = undefined>(T) => T; extraValue: () => void; }");
AssertType({    test: <T = undefined>(value: T): T => value,    extraValue: () => {},}, "{ test: <T = undefined>(T) => T; extraValue: () => void; }");

    test: <T = undefined>(value: T): T => value,
AssertType(test, "<T = undefined>(T) => T");
AssertType(<T = undefined>(value: T): T => value, "<T = undefined>(T) => T");
AssertType(value, "T");
AssertType(value, "T");

    extraValue: () => {},
AssertType(extraValue, "() => void");
AssertType(() => {}, "() => void");

})

const fn1async = () => ({
AssertType(fn1async, "() => { test: <T = undefined>(T) => Promise<T>; extraValue: () => void; }");
AssertType(() => ({    test: async <T = undefined>(value: T): Promise<T> => value,    extraValue: () => {},}), "() => { test: <T = undefined>(T) => Promise<T>; extraValue: () => void; }");
AssertType(({    test: async <T = undefined>(value: T): Promise<T> => value,    extraValue: () => {},}), "{ test: <T = undefined>(T) => Promise<T>; extraValue: () => void; }");
AssertType({    test: async <T = undefined>(value: T): Promise<T> => value,    extraValue: () => {},}, "{ test: <T = undefined>(T) => Promise<T>; extraValue: () => void; }");

    test: async <T = undefined>(value: T): Promise<T> => value,
AssertType(test, "<T = undefined>(T) => Promise<T>");
AssertType(async <T = undefined>(value: T): Promise<T> => value, "<T = undefined>(T) => Promise<T>");
AssertType(value, "T");
AssertType(value, "T");

    extraValue: () => {},
AssertType(extraValue, "() => void");
AssertType(() => {}, "() => void");

})

const fn2 = () => ({
AssertType(fn2, "() => { test: <T>(T) => T; extraValue: () => void; }");
AssertType(() => ({    test: <T>(value: T): T => value,    extraValue: () => {},}), "() => { test: <T>(T) => T; extraValue: () => void; }");
AssertType(({    test: <T>(value: T): T => value,    extraValue: () => {},}), "{ test: <T>(T) => T; extraValue: () => void; }");
AssertType({    test: <T>(value: T): T => value,    extraValue: () => {},}, "{ test: <T>(T) => T; extraValue: () => void; }");

    test: <T>(value: T): T => value,
AssertType(test, "<T>(T) => T");
AssertType(<T>(value: T): T => value, "<T>(T) => T");
AssertType(value, "T");
AssertType(value, "T");

    extraValue: () => {},
AssertType(extraValue, "() => void");
AssertType(() => {}, "() => void");

})

const fn2async = () => ({
AssertType(fn2async, "() => { test: <T>(T) => Promise<T>; extraValue: () => void; }");
AssertType(() => ({    test: async <T>(value: T): Promise<T> => value,    extraValue: () => {},}), "() => { test: <T>(T) => Promise<T>; extraValue: () => void; }");
AssertType(({    test: async <T>(value: T): Promise<T> => value,    extraValue: () => {},}), "{ test: <T>(T) => Promise<T>; extraValue: () => void; }");
AssertType({    test: async <T>(value: T): Promise<T> => value,    extraValue: () => {},}, "{ test: <T>(T) => Promise<T>; extraValue: () => void; }");

    test: async <T>(value: T): Promise<T> => value,
AssertType(test, "<T>(T) => Promise<T>");
AssertType(async <T>(value: T): Promise<T> => value, "<T>(T) => Promise<T>");
AssertType(value, "T");
AssertType(value, "T");

    extraValue: () => {},
AssertType(extraValue, "() => void");
AssertType(() => {}, "() => void");

})

const fn3 = () => ({
AssertType(fn3, "() => { extraValue: () => void; test: <T = undefined>(T) => T; }");
AssertType(() => ({    extraValue: () => {},    test: <T = undefined>(value: T): T => value,}), "() => { extraValue: () => void; test: <T = undefined>(T) => T; }");
AssertType(({    extraValue: () => {},    test: <T = undefined>(value: T): T => value,}), "{ extraValue: () => void; test: <T = undefined>(T) => T; }");
AssertType({    extraValue: () => {},    test: <T = undefined>(value: T): T => value,}, "{ extraValue: () => void; test: <T = undefined>(T) => T; }");

    extraValue: () => {},
AssertType(extraValue, "() => void");
AssertType(() => {}, "() => void");

    test: <T = undefined>(value: T): T => value,
AssertType(test, "<T = undefined>(T) => T");
AssertType(<T = undefined>(value: T): T => value, "<T = undefined>(T) => T");
AssertType(value, "T");
AssertType(value, "T");

})

const fn3async = () => ({
AssertType(fn3async, "() => { extraValue: () => void; test: <T = undefined>(T) => Promise<T>; }");
AssertType(() => ({    extraValue: () => {},    test: async <T = undefined>(value: T): Promise<T> => value,}), "() => { extraValue: () => void; test: <T = undefined>(T) => Promise<T>; }");
AssertType(({    extraValue: () => {},    test: async <T = undefined>(value: T): Promise<T> => value,}), "{ extraValue: () => void; test: <T = undefined>(T) => Promise<T>; }");
AssertType({    extraValue: () => {},    test: async <T = undefined>(value: T): Promise<T> => value,}, "{ extraValue: () => void; test: <T = undefined>(T) => Promise<T>; }");

    extraValue: () => {},
AssertType(extraValue, "() => void");
AssertType(() => {}, "() => void");

    test: async <T = undefined>(value: T): Promise<T> => value,
AssertType(test, "<T = undefined>(T) => Promise<T>");
AssertType(async <T = undefined>(value: T): Promise<T> => value, "<T = undefined>(T) => Promise<T>");
AssertType(value, "T");
AssertType(value, "T");

})

const fn4 = () => ({
AssertType(fn4, "() => { extraValue: string; test: <T = undefined>(T) => T; }");
AssertType(() => ({    extraValue: '',    test: <T = undefined>(value: T): T => value,}), "() => { extraValue: string; test: <T = undefined>(T) => T; }");
AssertType(({    extraValue: '',    test: <T = undefined>(value: T): T => value,}), "{ extraValue: string; test: <T = undefined>(T) => T; }");
AssertType({    extraValue: '',    test: <T = undefined>(value: T): T => value,}, "{ extraValue: string; test: <T = undefined>(T) => T; }");

    extraValue: '',
AssertType(extraValue, "string");
AssertType('', "string");

    test: <T = undefined>(value: T): T => value,
AssertType(test, "<T = undefined>(T) => T");
AssertType(<T = undefined>(value: T): T => value, "<T = undefined>(T) => T");
AssertType(value, "T");
AssertType(value, "T");

})

const fn4async = () => ({
AssertType(fn4async, "() => { extraValue: string; test: <T = undefined>(T) => Promise<T>; }");
AssertType(() => ({    extraValue: '',    test: async <T = undefined>(value: T): Promise<T> => value,}), "() => { extraValue: string; test: <T = undefined>(T) => Promise<T>; }");
AssertType(({    extraValue: '',    test: async <T = undefined>(value: T): Promise<T> => value,}), "{ extraValue: string; test: <T = undefined>(T) => Promise<T>; }");
AssertType({    extraValue: '',    test: async <T = undefined>(value: T): Promise<T> => value,}, "{ extraValue: string; test: <T = undefined>(T) => Promise<T>; }");

    extraValue: '',
AssertType(extraValue, "string");
AssertType('', "string");

    test: async <T = undefined>(value: T): Promise<T> => value,
AssertType(test, "<T = undefined>(T) => Promise<T>");
AssertType(async <T = undefined>(value: T): Promise<T> => value, "<T = undefined>(T) => Promise<T>");
AssertType(value, "T");
AssertType(value, "T");

})


