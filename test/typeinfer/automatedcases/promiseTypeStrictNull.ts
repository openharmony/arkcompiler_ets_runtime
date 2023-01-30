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

// === tests/cases/compiler/promiseTypeStrictNull.ts ===
declare function AssertType(value:any, type:string):void;
declare let p: Promise<boolean>;
AssertType(p, "Promise<boolean>");

declare let x: any;
AssertType(x, "any");

async function A() {
    const a = await p;
AssertType(a, "boolean");
AssertType(await p, "boolean");
AssertType(p, "Promise<boolean>");

AssertType(a, "boolean");
    return a;
}

async function B() {
    const a = await p;
AssertType(a, "boolean");
AssertType(await p, "boolean");
AssertType(p, "Promise<boolean>");

AssertType(1, "int");
    return 1;
}

async function C() {
    try {
        const a = await p;
AssertType(a, "boolean");
AssertType(await p, "boolean");
AssertType(p, "Promise<boolean>");

AssertType(1, "int");
        return 1;
    }
    catch (e) {
AssertType(e, "any");

AssertType('error', "string");
        return 'error';
    }
}

async function D() {
    try {
        const a = await p;
AssertType(a, "boolean");
AssertType(await p, "boolean");
AssertType(p, "Promise<boolean>");

AssertType(1, "int");
        return 1;
    }
    catch (e) {
AssertType(e, "any");
    }
}

async function E() {
    try {
        const a = await p;
AssertType(a, "boolean");
AssertType(await p, "boolean");
AssertType(p, "Promise<boolean>");

AssertType(1, "int");
        return 1;
    }
    catch (e) {
AssertType(e, "any");

        throw Error();
AssertType(Error(), "Error");
AssertType(Error, "ErrorConstructor");
    }
}

async function F() {
    try {
        const a = await p;
AssertType(a, "boolean");
AssertType(await p, "boolean");
AssertType(p, "Promise<boolean>");

AssertType(1, "int");
        return 1;
    }
    catch (e) {
AssertType(e, "any");

AssertType(Promise.reject(Error()), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType(Error(), "Error");
AssertType(Error, "ErrorConstructor");
        return Promise.reject(Error());
    }
}

async function G() {
    try {
        const a = await p;
AssertType(a, "boolean");
AssertType(await p, "boolean");
AssertType(p, "Promise<boolean>");

AssertType(a, "boolean");
        return a;
    }
    catch (e) {
AssertType(e, "any");

        return;
    }
}

async function H() {
    try {
        const a = await p;
AssertType(a, "boolean");
AssertType(await p, "boolean");
AssertType(p, "Promise<boolean>");

AssertType(a, "boolean");
        return a;
    }
    catch (e) {
AssertType(e, "any");

        throw Error();
AssertType(Error(), "Error");
AssertType(Error, "ErrorConstructor");
    }
}

async function I() {
    try {
        const a = await p;
AssertType(a, "boolean");
AssertType(await p, "boolean");
AssertType(p, "Promise<boolean>");

AssertType(a, "boolean");
        return a;
    }
    catch (e) {
AssertType(e, "any");

AssertType(Promise.reject(Error()), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType(Error(), "Error");
AssertType(Error, "ErrorConstructor");
        return Promise.reject(Error());
    }
}

// addresses github issue #4903:

const p00 = p.catch();
AssertType(p00, "Promise<boolean>");
AssertType(p.catch(), "Promise<boolean>");
AssertType(p.catch, "<TResult = never>(?union) => Promise<union>");

const p01 = p.then();
AssertType(p01, "Promise<boolean>");
AssertType(p.then(), "Promise<boolean>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");

const p10 = p.catch(undefined);
AssertType(p10, "Promise<boolean>");
AssertType(p.catch(undefined), "Promise<boolean>");
AssertType(p.catch, "<TResult = never>(?union) => Promise<union>");
AssertType(undefined, "undefined");

const p11 = p.catch(null);
AssertType(p11, "Promise<boolean>");
AssertType(p.catch(null), "Promise<boolean>");
AssertType(p.catch, "<TResult = never>(?union) => Promise<union>");
AssertType(null, "null");

const p12 = p.catch(() => 1);
AssertType(p12, "Promise<union>");
AssertType(p.catch(() => 1), "Promise<union>");
AssertType(p.catch, "<TResult = never>(?union) => Promise<union>");
AssertType(() => 1, "() => number");
AssertType(1, "int");

const p13 = p.catch(() => x);
AssertType(p13, "Promise<any>");
AssertType(p.catch(() => x), "Promise<any>");
AssertType(p.catch, "<TResult = never>(?union) => Promise<union>");
AssertType(() => x, "() => any");
AssertType(x, "any");

const p14 = p.catch(() => undefined);
AssertType(p14, "Promise<union>");
AssertType(p.catch(() => undefined), "Promise<union>");
AssertType(p.catch, "<TResult = never>(?union) => Promise<union>");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");

const p15 = p.catch(() => null);
AssertType(p15, "Promise<union>");
AssertType(p.catch(() => null), "Promise<union>");
AssertType(p.catch, "<TResult = never>(?union) => Promise<union>");
AssertType(() => null, "() => null");
AssertType(null, "null");

const p16 = p.catch(() => {});
AssertType(p16, "Promise<union>");
AssertType(p.catch(() => {}), "Promise<union>");
AssertType(p.catch, "<TResult = never>(?union) => Promise<union>");
AssertType(() => {}, "() => void");

const p17 = p.catch(() => {throw 1});
AssertType(p17, "Promise<boolean>");
AssertType(p.catch(() => {throw 1}), "Promise<boolean>");
AssertType(p.catch, "<TResult = never>(?union) => Promise<union>");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");

const p18 = p.catch(() => Promise.reject(1));
AssertType(p18, "Promise<boolean>");
AssertType(p.catch(() => Promise.reject(1)), "Promise<boolean>");
AssertType(p.catch, "<TResult = never>(?union) => Promise<union>");
AssertType(() => Promise.reject(1), "() => Promise<never>");
AssertType(Promise.reject(1), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType(1, "int");

const p19 = p.catch(() => Promise.resolve(1));
AssertType(p19, "Promise<union>");
AssertType(p.catch(() => Promise.resolve(1)), "Promise<union>");
AssertType(p.catch, "<TResult = never>(?union) => Promise<union>");
AssertType(() => Promise.resolve(1), "() => Promise<number>");
AssertType(Promise.resolve(1), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(1, "int");

const p20 = p.then(undefined);
AssertType(p20, "Promise<boolean>");
AssertType(p.then(undefined), "Promise<boolean>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(undefined, "undefined");

const p21 = p.then(null);
AssertType(p21, "Promise<boolean>");
AssertType(p.then(null), "Promise<boolean>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(null, "null");

const p22 = p.then(() => 1);
AssertType(p22, "Promise<number>");
AssertType(p.then(() => 1), "Promise<number>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => 1, "() => number");
AssertType(1, "int");

const p23 = p.then(() => x);
AssertType(p23, "Promise<any>");
AssertType(p.then(() => x), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => x, "() => any");
AssertType(x, "any");

const p24 = p.then(() => undefined);
AssertType(p24, "Promise<undefined>");
AssertType(p.then(() => undefined), "Promise<undefined>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");

const p25 = p.then(() => null);
AssertType(p25, "Promise<null>");
AssertType(p.then(() => null), "Promise<null>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => null, "() => null");
AssertType(null, "null");

const p26 = p.then(() => {});
AssertType(p26, "Promise<void>");
AssertType(p.then(() => {}), "Promise<void>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {}, "() => void");

const p27 = p.then(() => {throw 1});
AssertType(p27, "Promise<never>");
AssertType(p.then(() => {throw 1}), "Promise<never>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");

const p28 = p.then(() => Promise.resolve(1));
AssertType(p28, "Promise<number>");
AssertType(p.then(() => Promise.resolve(1)), "Promise<number>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.resolve(1), "() => Promise<number>");
AssertType(Promise.resolve(1), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(1, "int");

const p29 = p.then(() => Promise.reject(1));
AssertType(p29, "Promise<never>");
AssertType(p.then(() => Promise.reject(1)), "Promise<never>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.reject(1), "() => Promise<never>");
AssertType(Promise.reject(1), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType(1, "int");

const p30 = p.then(undefined, undefined);
AssertType(p30, "Promise<boolean>");
AssertType(p.then(undefined, undefined), "Promise<boolean>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(undefined, "undefined");
AssertType(undefined, "undefined");

const p31 = p.then(undefined, null);
AssertType(p31, "Promise<boolean>");
AssertType(p.then(undefined, null), "Promise<boolean>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(undefined, "undefined");
AssertType(null, "null");

const p32 = p.then(undefined, () => 1);
AssertType(p32, "Promise<union>");
AssertType(p.then(undefined, () => 1), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(undefined, "undefined");
AssertType(() => 1, "() => number");
AssertType(1, "int");

const p33 = p.then(undefined, () => x);
AssertType(p33, "Promise<any>");
AssertType(p.then(undefined, () => x), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(undefined, "undefined");
AssertType(() => x, "() => any");
AssertType(x, "any");

const p34 = p.then(undefined, () => undefined);
AssertType(p34, "Promise<union>");
AssertType(p.then(undefined, () => undefined), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(undefined, "undefined");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");

const p35 = p.then(undefined, () => null);
AssertType(p35, "Promise<union>");
AssertType(p.then(undefined, () => null), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(undefined, "undefined");
AssertType(() => null, "() => null");
AssertType(null, "null");

const p36 = p.then(undefined, () => {});
AssertType(p36, "Promise<union>");
AssertType(p.then(undefined, () => {}), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(undefined, "undefined");
AssertType(() => {}, "() => void");

const p37 = p.then(undefined, () => {throw 1});
AssertType(p37, "Promise<boolean>");
AssertType(p.then(undefined, () => {throw 1}), "Promise<boolean>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(undefined, "undefined");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");

const p38 = p.then(undefined, () => Promise.resolve(1));
AssertType(p38, "Promise<union>");
AssertType(p.then(undefined, () => Promise.resolve(1)), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(undefined, "undefined");
AssertType(() => Promise.resolve(1), "() => Promise<number>");
AssertType(Promise.resolve(1), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(1, "int");

const p39 = p.then(undefined, () => Promise.reject(1));
AssertType(p39, "Promise<boolean>");
AssertType(p.then(undefined, () => Promise.reject(1)), "Promise<boolean>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(undefined, "undefined");
AssertType(() => Promise.reject(1), "() => Promise<never>");
AssertType(Promise.reject(1), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType(1, "int");

const p40 = p.then(null, undefined);
AssertType(p40, "Promise<boolean>");
AssertType(p.then(null, undefined), "Promise<boolean>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(null, "null");
AssertType(undefined, "undefined");

const p41 = p.then(null, null);
AssertType(p41, "Promise<boolean>");
AssertType(p.then(null, null), "Promise<boolean>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(null, "null");
AssertType(null, "null");

const p42 = p.then(null, () => 1);
AssertType(p42, "Promise<union>");
AssertType(p.then(null, () => 1), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(null, "null");
AssertType(() => 1, "() => number");
AssertType(1, "int");

const p43 = p.then(null, () => x);
AssertType(p43, "Promise<any>");
AssertType(p.then(null, () => x), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(null, "null");
AssertType(() => x, "() => any");
AssertType(x, "any");

const p44 = p.then(null, () => undefined);
AssertType(p44, "Promise<union>");
AssertType(p.then(null, () => undefined), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(null, "null");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");

const p45 = p.then(null, () => null);
AssertType(p45, "Promise<union>");
AssertType(p.then(null, () => null), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(null, "null");
AssertType(() => null, "() => null");
AssertType(null, "null");

const p46 = p.then(null, () => {});
AssertType(p46, "Promise<union>");
AssertType(p.then(null, () => {}), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(null, "null");
AssertType(() => {}, "() => void");

const p47 = p.then(null, () => {throw 1});
AssertType(p47, "Promise<boolean>");
AssertType(p.then(null, () => {throw 1}), "Promise<boolean>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(null, "null");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");

const p48 = p.then(null, () => Promise.resolve(1));
AssertType(p48, "Promise<union>");
AssertType(p.then(null, () => Promise.resolve(1)), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(null, "null");
AssertType(() => Promise.resolve(1), "() => Promise<number>");
AssertType(Promise.resolve(1), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(1, "int");

const p49 = p.then(null, () => Promise.reject(1));
AssertType(p49, "Promise<boolean>");
AssertType(p.then(null, () => Promise.reject(1)), "Promise<boolean>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(null, "null");
AssertType(() => Promise.reject(1), "() => Promise<never>");
AssertType(Promise.reject(1), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType(1, "int");

const p50 = p.then(() => "1", undefined);
AssertType(p50, "Promise<string>");
AssertType(p.then(() => "1", undefined), "Promise<string>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => "1", "() => string");
AssertType("1", "string");
AssertType(undefined, "undefined");

const p51 = p.then(() => "1", null);
AssertType(p51, "Promise<string>");
AssertType(p.then(() => "1", null), "Promise<string>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => "1", "() => string");
AssertType("1", "string");
AssertType(null, "null");

const p52 = p.then(() => "1", () => 1);
AssertType(p52, "Promise<union>");
AssertType(p.then(() => "1", () => 1), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => "1", "() => string");
AssertType("1", "string");
AssertType(() => 1, "() => number");
AssertType(1, "int");

const p53 = p.then(() => "1", () => x);
AssertType(p53, "Promise<any>");
AssertType(p.then(() => "1", () => x), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => "1", "() => string");
AssertType("1", "string");
AssertType(() => x, "() => any");
AssertType(x, "any");

const p54 = p.then(() => "1", () => undefined);
AssertType(p54, "Promise<union>");
AssertType(p.then(() => "1", () => undefined), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => "1", "() => string");
AssertType("1", "string");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");

const p55 = p.then(() => "1", () => null);
AssertType(p55, "Promise<union>");
AssertType(p.then(() => "1", () => null), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => "1", "() => string");
AssertType("1", "string");
AssertType(() => null, "() => null");
AssertType(null, "null");

const p56 = p.then(() => "1", () => {});
AssertType(p56, "Promise<union>");
AssertType(p.then(() => "1", () => {}), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => "1", "() => string");
AssertType("1", "string");
AssertType(() => {}, "() => void");

const p57 = p.then(() => "1", () => {throw 1});
AssertType(p57, "Promise<string>");
AssertType(p.then(() => "1", () => {throw 1}), "Promise<string>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => "1", "() => string");
AssertType("1", "string");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");

const p58 = p.then(() => "1", () => Promise.resolve(1));
AssertType(p58, "Promise<union>");
AssertType(p.then(() => "1", () => Promise.resolve(1)), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => "1", "() => string");
AssertType("1", "string");
AssertType(() => Promise.resolve(1), "() => Promise<number>");
AssertType(Promise.resolve(1), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(1, "int");

const p59 = p.then(() => "1", () => Promise.reject(1));
AssertType(p59, "Promise<string>");
AssertType(p.then(() => "1", () => Promise.reject(1)), "Promise<string>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => "1", "() => string");
AssertType("1", "string");
AssertType(() => Promise.reject(1), "() => Promise<never>");
AssertType(Promise.reject(1), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType(1, "int");

const p60 = p.then(() => x, undefined);
AssertType(p60, "Promise<any>");
AssertType(p.then(() => x, undefined), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => x, "() => any");
AssertType(x, "any");
AssertType(undefined, "undefined");

const p61 = p.then(() => x, null);
AssertType(p61, "Promise<any>");
AssertType(p.then(() => x, null), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => x, "() => any");
AssertType(x, "any");
AssertType(null, "null");

const p62 = p.then(() => x, () => 1);
AssertType(p62, "Promise<any>");
AssertType(p.then(() => x, () => 1), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => x, "() => any");
AssertType(x, "any");
AssertType(() => 1, "() => number");
AssertType(1, "int");

const p63 = p.then(() => x, () => x);
AssertType(p63, "Promise<any>");
AssertType(p.then(() => x, () => x), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => x, "() => any");
AssertType(x, "any");
AssertType(() => x, "() => any");
AssertType(x, "any");

const p64 = p.then(() => x, () => undefined);
AssertType(p64, "Promise<any>");
AssertType(p.then(() => x, () => undefined), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => x, "() => any");
AssertType(x, "any");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");

const p65 = p.then(() => x, () => null);
AssertType(p65, "Promise<any>");
AssertType(p.then(() => x, () => null), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => x, "() => any");
AssertType(x, "any");
AssertType(() => null, "() => null");
AssertType(null, "null");

const p66 = p.then(() => x, () => {});
AssertType(p66, "Promise<any>");
AssertType(p.then(() => x, () => {}), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => x, "() => any");
AssertType(x, "any");
AssertType(() => {}, "() => void");

const p67 = p.then(() => x, () => {throw 1});
AssertType(p67, "Promise<any>");
AssertType(p.then(() => x, () => {throw 1}), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => x, "() => any");
AssertType(x, "any");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");

const p68 = p.then(() => x, () => Promise.resolve(1));
AssertType(p68, "Promise<any>");
AssertType(p.then(() => x, () => Promise.resolve(1)), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => x, "() => any");
AssertType(x, "any");
AssertType(() => Promise.resolve(1), "() => Promise<number>");
AssertType(Promise.resolve(1), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(1, "int");

const p69 = p.then(() => x, () => Promise.reject(1));
AssertType(p69, "Promise<any>");
AssertType(p.then(() => x, () => Promise.reject(1)), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => x, "() => any");
AssertType(x, "any");
AssertType(() => Promise.reject(1), "() => Promise<never>");
AssertType(Promise.reject(1), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType(1, "int");

const p70 = p.then(() => undefined, undefined);
AssertType(p70, "Promise<undefined>");
AssertType(p.then(() => undefined, undefined), "Promise<undefined>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");
AssertType(undefined, "undefined");

const p71 = p.then(() => undefined, null);
AssertType(p71, "Promise<undefined>");
AssertType(p.then(() => undefined, null), "Promise<undefined>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");
AssertType(null, "null");

const p72 = p.then(() => undefined, () => 1);
AssertType(p72, "Promise<union>");
AssertType(p.then(() => undefined, () => 1), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");
AssertType(() => 1, "() => number");
AssertType(1, "int");

const p73 = p.then(() => undefined, () => x);
AssertType(p73, "Promise<any>");
AssertType(p.then(() => undefined, () => x), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");
AssertType(() => x, "() => any");
AssertType(x, "any");

const p74 = p.then(() => undefined, () => undefined);
AssertType(p74, "Promise<undefined>");
AssertType(p.then(() => undefined, () => undefined), "Promise<undefined>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");

const p75 = p.then(() => undefined, () => null);
AssertType(p75, "Promise<union>");
AssertType(p.then(() => undefined, () => null), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");
AssertType(() => null, "() => null");
AssertType(null, "null");

const p76 = p.then(() => undefined, () => {});
AssertType(p76, "Promise<union>");
AssertType(p.then(() => undefined, () => {}), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");
AssertType(() => {}, "() => void");

const p77 = p.then(() => undefined, () => {throw 1});
AssertType(p77, "Promise<undefined>");
AssertType(p.then(() => undefined, () => {throw 1}), "Promise<undefined>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");

const p78 = p.then(() => undefined, () => Promise.resolve(1));
AssertType(p78, "Promise<union>");
AssertType(p.then(() => undefined, () => Promise.resolve(1)), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");
AssertType(() => Promise.resolve(1), "() => Promise<number>");
AssertType(Promise.resolve(1), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(1, "int");

const p79 = p.then(() => undefined, () => Promise.reject(1));
AssertType(p79, "Promise<undefined>");
AssertType(p.then(() => undefined, () => Promise.reject(1)), "Promise<undefined>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");
AssertType(() => Promise.reject(1), "() => Promise<never>");
AssertType(Promise.reject(1), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType(1, "int");

const p80 = p.then(() => null, undefined);
AssertType(p80, "Promise<null>");
AssertType(p.then(() => null, undefined), "Promise<null>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => null, "() => null");
AssertType(null, "null");
AssertType(undefined, "undefined");

const p81 = p.then(() => null, null);
AssertType(p81, "Promise<null>");
AssertType(p.then(() => null, null), "Promise<null>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => null, "() => null");
AssertType(null, "null");
AssertType(null, "null");

const p82 = p.then(() => null, () => 1);
AssertType(p82, "Promise<union>");
AssertType(p.then(() => null, () => 1), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => null, "() => null");
AssertType(null, "null");
AssertType(() => 1, "() => number");
AssertType(1, "int");

const p83 = p.then(() => null, () => x);
AssertType(p83, "Promise<any>");
AssertType(p.then(() => null, () => x), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => null, "() => null");
AssertType(null, "null");
AssertType(() => x, "() => any");
AssertType(x, "any");

const p84 = p.then(() => null, () => undefined);
AssertType(p84, "Promise<union>");
AssertType(p.then(() => null, () => undefined), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => null, "() => null");
AssertType(null, "null");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");

const p85 = p.then(() => null, () => null);
AssertType(p85, "Promise<null>");
AssertType(p.then(() => null, () => null), "Promise<null>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => null, "() => null");
AssertType(null, "null");
AssertType(() => null, "() => null");
AssertType(null, "null");

const p86 = p.then(() => null, () => {});
AssertType(p86, "Promise<union>");
AssertType(p.then(() => null, () => {}), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => null, "() => null");
AssertType(null, "null");
AssertType(() => {}, "() => void");

const p87 = p.then(() => null, () => {throw 1});
AssertType(p87, "Promise<null>");
AssertType(p.then(() => null, () => {throw 1}), "Promise<null>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => null, "() => null");
AssertType(null, "null");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");

const p88 = p.then(() => null, () => Promise.resolve(1));
AssertType(p88, "Promise<union>");
AssertType(p.then(() => null, () => Promise.resolve(1)), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => null, "() => null");
AssertType(null, "null");
AssertType(() => Promise.resolve(1), "() => Promise<number>");
AssertType(Promise.resolve(1), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(1, "int");

const p89 = p.then(() => null, () => Promise.reject(1));
AssertType(p89, "Promise<null>");
AssertType(p.then(() => null, () => Promise.reject(1)), "Promise<null>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => null, "() => null");
AssertType(null, "null");
AssertType(() => Promise.reject(1), "() => Promise<never>");
AssertType(Promise.reject(1), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType(1, "int");

const p90 = p.then(() => {}, undefined);
AssertType(p90, "Promise<void>");
AssertType(p.then(() => {}, undefined), "Promise<void>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {}, "() => void");
AssertType(undefined, "undefined");

const p91 = p.then(() => {}, null);
AssertType(p91, "Promise<void>");
AssertType(p.then(() => {}, null), "Promise<void>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {}, "() => void");
AssertType(null, "null");

const p92 = p.then(() => {}, () => 1);
AssertType(p92, "Promise<union>");
AssertType(p.then(() => {}, () => 1), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {}, "() => void");
AssertType(() => 1, "() => number");
AssertType(1, "int");

const p93 = p.then(() => {}, () => x);
AssertType(p93, "Promise<any>");
AssertType(p.then(() => {}, () => x), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {}, "() => void");
AssertType(() => x, "() => any");
AssertType(x, "any");

const p94 = p.then(() => {}, () => undefined);
AssertType(p94, "Promise<union>");
AssertType(p.then(() => {}, () => undefined), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {}, "() => void");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");

const p95 = p.then(() => {}, () => null);
AssertType(p95, "Promise<union>");
AssertType(p.then(() => {}, () => null), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {}, "() => void");
AssertType(() => null, "() => null");
AssertType(null, "null");

const p96 = p.then(() => {}, () => {});
AssertType(p96, "Promise<void>");
AssertType(p.then(() => {}, () => {}), "Promise<void>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {}, "() => void");
AssertType(() => {}, "() => void");

const p97 = p.then(() => {}, () => {throw 1});
AssertType(p97, "Promise<void>");
AssertType(p.then(() => {}, () => {throw 1}), "Promise<void>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {}, "() => void");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");

const p98 = p.then(() => {}, () => Promise.resolve(1));
AssertType(p98, "Promise<union>");
AssertType(p.then(() => {}, () => Promise.resolve(1)), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {}, "() => void");
AssertType(() => Promise.resolve(1), "() => Promise<number>");
AssertType(Promise.resolve(1), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(1, "int");

const p99 = p.then(() => {}, () => Promise.reject(1));
AssertType(p99, "Promise<void>");
AssertType(p.then(() => {}, () => Promise.reject(1)), "Promise<void>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {}, "() => void");
AssertType(() => Promise.reject(1), "() => Promise<never>");
AssertType(Promise.reject(1), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType(1, "int");

const pa0 = p.then(() => {throw 1}, undefined);
AssertType(pa0, "Promise<never>");
AssertType(p.then(() => {throw 1}, undefined), "Promise<never>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");
AssertType(undefined, "undefined");

const pa1 = p.then(() => {throw 1}, null);
AssertType(pa1, "Promise<never>");
AssertType(p.then(() => {throw 1}, null), "Promise<never>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");
AssertType(null, "null");

const pa2 = p.then(() => {throw 1}, () => 1);
AssertType(pa2, "Promise<number>");
AssertType(p.then(() => {throw 1}, () => 1), "Promise<number>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");
AssertType(() => 1, "() => number");
AssertType(1, "int");

const pa3 = p.then(() => {throw 1}, () => x);
AssertType(pa3, "Promise<any>");
AssertType(p.then(() => {throw 1}, () => x), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");
AssertType(() => x, "() => any");
AssertType(x, "any");

const pa4 = p.then(() => {throw 1}, () => undefined);
AssertType(pa4, "Promise<undefined>");
AssertType(p.then(() => {throw 1}, () => undefined), "Promise<undefined>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");

const pa5 = p.then(() => {throw 1}, () => null);
AssertType(pa5, "Promise<null>");
AssertType(p.then(() => {throw 1}, () => null), "Promise<null>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");
AssertType(() => null, "() => null");
AssertType(null, "null");

const pa6 = p.then(() => {throw 1}, () => {});
AssertType(pa6, "Promise<void>");
AssertType(p.then(() => {throw 1}, () => {}), "Promise<void>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");
AssertType(() => {}, "() => void");

const pa7 = p.then(() => {throw 1}, () => {throw 1});
AssertType(pa7, "Promise<never>");
AssertType(p.then(() => {throw 1}, () => {throw 1}), "Promise<never>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");

const pa8 = p.then(() => {throw 1}, () => Promise.resolve(1));
AssertType(pa8, "Promise<number>");
AssertType(p.then(() => {throw 1}, () => Promise.resolve(1)), "Promise<number>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");
AssertType(() => Promise.resolve(1), "() => Promise<number>");
AssertType(Promise.resolve(1), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(1, "int");

const pa9 = p.then(() => {throw 1}, () => Promise.reject(1));
AssertType(pa9, "Promise<never>");
AssertType(p.then(() => {throw 1}, () => Promise.reject(1)), "Promise<never>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");
AssertType(() => Promise.reject(1), "() => Promise<never>");
AssertType(Promise.reject(1), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType(1, "int");

const pb0 = p.then(() => Promise.resolve("1"), undefined);
AssertType(pb0, "Promise<string>");
AssertType(p.then(() => Promise.resolve("1"), undefined), "Promise<string>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.resolve("1"), "() => Promise<string>");
AssertType(Promise.resolve("1"), "Promise<string>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType("1", "string");
AssertType(undefined, "undefined");

const pb1 = p.then(() => Promise.resolve("1"), null);
AssertType(pb1, "Promise<string>");
AssertType(p.then(() => Promise.resolve("1"), null), "Promise<string>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.resolve("1"), "() => Promise<string>");
AssertType(Promise.resolve("1"), "Promise<string>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType("1", "string");
AssertType(null, "null");

const pb2 = p.then(() => Promise.resolve("1"), () => 1);
AssertType(pb2, "Promise<union>");
AssertType(p.then(() => Promise.resolve("1"), () => 1), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.resolve("1"), "() => Promise<string>");
AssertType(Promise.resolve("1"), "Promise<string>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType("1", "string");
AssertType(() => 1, "() => number");
AssertType(1, "int");

const pb3 = p.then(() => Promise.resolve("1"), () => x);
AssertType(pb3, "Promise<any>");
AssertType(p.then(() => Promise.resolve("1"), () => x), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.resolve("1"), "() => Promise<string>");
AssertType(Promise.resolve("1"), "Promise<string>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType("1", "string");
AssertType(() => x, "() => any");
AssertType(x, "any");

const pb4 = p.then(() => Promise.resolve("1"), () => undefined);
AssertType(pb4, "Promise<union>");
AssertType(p.then(() => Promise.resolve("1"), () => undefined), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.resolve("1"), "() => Promise<string>");
AssertType(Promise.resolve("1"), "Promise<string>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType("1", "string");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");

const pb5 = p.then(() => Promise.resolve("1"), () => null);
AssertType(pb5, "Promise<union>");
AssertType(p.then(() => Promise.resolve("1"), () => null), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.resolve("1"), "() => Promise<string>");
AssertType(Promise.resolve("1"), "Promise<string>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType("1", "string");
AssertType(() => null, "() => null");
AssertType(null, "null");

const pb6 = p.then(() => Promise.resolve("1"), () => {});
AssertType(pb6, "Promise<union>");
AssertType(p.then(() => Promise.resolve("1"), () => {}), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.resolve("1"), "() => Promise<string>");
AssertType(Promise.resolve("1"), "Promise<string>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType("1", "string");
AssertType(() => {}, "() => void");

const pb7 = p.then(() => Promise.resolve("1"), () => {throw 1});
AssertType(pb7, "Promise<string>");
AssertType(p.then(() => Promise.resolve("1"), () => {throw 1}), "Promise<string>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.resolve("1"), "() => Promise<string>");
AssertType(Promise.resolve("1"), "Promise<string>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType("1", "string");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");

const pb8 = p.then(() => Promise.resolve("1"), () => Promise.resolve(1));
AssertType(pb8, "Promise<union>");
AssertType(p.then(() => Promise.resolve("1"), () => Promise.resolve(1)), "Promise<union>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.resolve("1"), "() => Promise<string>");
AssertType(Promise.resolve("1"), "Promise<string>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType("1", "string");
AssertType(() => Promise.resolve(1), "() => Promise<number>");
AssertType(Promise.resolve(1), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(1, "int");

const pb9 = p.then(() => Promise.resolve("1"), () => Promise.reject(1));
AssertType(pb9, "Promise<string>");
AssertType(p.then(() => Promise.resolve("1"), () => Promise.reject(1)), "Promise<string>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.resolve("1"), "() => Promise<string>");
AssertType(Promise.resolve("1"), "Promise<string>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType("1", "string");
AssertType(() => Promise.reject(1), "() => Promise<never>");
AssertType(Promise.reject(1), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType(1, "int");

const pc0 = p.then(() => Promise.reject("1"), undefined);
AssertType(pc0, "Promise<never>");
AssertType(p.then(() => Promise.reject("1"), undefined), "Promise<never>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.reject("1"), "() => Promise<never>");
AssertType(Promise.reject("1"), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType("1", "string");
AssertType(undefined, "undefined");

const pc1 = p.then(() => Promise.reject("1"), null);
AssertType(pc1, "Promise<never>");
AssertType(p.then(() => Promise.reject("1"), null), "Promise<never>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.reject("1"), "() => Promise<never>");
AssertType(Promise.reject("1"), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType("1", "string");
AssertType(null, "null");

const pc2 = p.then(() => Promise.reject("1"), () => 1);
AssertType(pc2, "Promise<number>");
AssertType(p.then(() => Promise.reject("1"), () => 1), "Promise<number>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.reject("1"), "() => Promise<never>");
AssertType(Promise.reject("1"), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType("1", "string");
AssertType(() => 1, "() => number");
AssertType(1, "int");

const pc3 = p.then(() => Promise.reject("1"), () => x);
AssertType(pc3, "Promise<any>");
AssertType(p.then(() => Promise.reject("1"), () => x), "Promise<any>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.reject("1"), "() => Promise<never>");
AssertType(Promise.reject("1"), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType("1", "string");
AssertType(() => x, "() => any");
AssertType(x, "any");

const pc4 = p.then(() => Promise.reject("1"), () => undefined);
AssertType(pc4, "Promise<undefined>");
AssertType(p.then(() => Promise.reject("1"), () => undefined), "Promise<undefined>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.reject("1"), "() => Promise<never>");
AssertType(Promise.reject("1"), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType("1", "string");
AssertType(() => undefined, "() => undefined");
AssertType(undefined, "undefined");

const pc5 = p.then(() => Promise.reject("1"), () => null);
AssertType(pc5, "Promise<null>");
AssertType(p.then(() => Promise.reject("1"), () => null), "Promise<null>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.reject("1"), "() => Promise<never>");
AssertType(Promise.reject("1"), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType("1", "string");
AssertType(() => null, "() => null");
AssertType(null, "null");

const pc6 = p.then(() => Promise.reject("1"), () => {});
AssertType(pc6, "Promise<void>");
AssertType(p.then(() => Promise.reject("1"), () => {}), "Promise<void>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.reject("1"), "() => Promise<never>");
AssertType(Promise.reject("1"), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType("1", "string");
AssertType(() => {}, "() => void");

const pc7 = p.then(() => Promise.reject("1"), () => {throw 1});
AssertType(pc7, "Promise<never>");
AssertType(p.then(() => Promise.reject("1"), () => {throw 1}), "Promise<never>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.reject("1"), "() => Promise<never>");
AssertType(Promise.reject("1"), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType("1", "string");
AssertType(() => {throw 1}, "() => never");
AssertType(1, "int");

const pc8 = p.then(() => Promise.reject("1"), () => Promise.resolve(1));
AssertType(pc8, "Promise<number>");
AssertType(p.then(() => Promise.reject("1"), () => Promise.resolve(1)), "Promise<number>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.reject("1"), "() => Promise<never>");
AssertType(Promise.reject("1"), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType("1", "string");
AssertType(() => Promise.resolve(1), "() => Promise<number>");
AssertType(Promise.resolve(1), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(1, "int");

const pc9 = p.then(() => Promise.reject("1"), () => Promise.reject(1));
AssertType(pc9, "Promise<never>");
AssertType(p.then(() => Promise.reject("1"), () => Promise.reject(1)), "Promise<never>");
AssertType(p.then, "<TResult1 = boolean, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(() => Promise.reject("1"), "() => Promise<never>");
AssertType(Promise.reject("1"), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType("1", "string");
AssertType(() => Promise.reject(1), "() => Promise<never>");
AssertType(Promise.reject(1), "Promise<never>");
AssertType(Promise.reject, "<T = never>(?any) => Promise<T>");
AssertType(1, "int");


