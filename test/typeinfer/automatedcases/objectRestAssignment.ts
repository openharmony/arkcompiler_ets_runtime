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

// === tests/cases/conformance/types/rest/objectRestAssignment.ts ===
declare function AssertType(value:any, type:string):void;
let ka: any;
AssertType(ka, "any");

let nested: { ki };
AssertType(nested, "{ ki: any; }");
AssertType(ki, "any");

let other: number;
AssertType(other, "number");

let rest: { };
AssertType(rest, "{}");

let complex: { x: { ka, ki }, y: number };
AssertType(complex, "{ x: {    ka;    ki;}; y: number; }");
AssertType(x, "{ ka: any; ki: any; }");
AssertType(ka, "any");
AssertType(ki, "any");
AssertType(y, "number");

({x: { ka, ...nested }, y: other, ...rest} = complex);
AssertType(({x: { ka, ...nested }, y: other, ...rest} = complex), "{ x: { ka: any; ki: any; }; y: number; }");
AssertType({x: { ka, ...nested }, y: other, ...rest} = complex, "{ x: { ka: any; ki: any; }; y: number; }");
AssertType({x: { ka, ...nested }, y: other, ...rest}, "{ x: { ki: any; ka: any; }; y: number; }");
AssertType(x, "{ ki: any; ka: any; }");
AssertType({ ka, ...nested }, "{ ki: any; ka: any; }");
AssertType(ka, "any");
AssertType(nested, "{ ki: any; }");
AssertType(y, "number");
AssertType(other, "number");
AssertType(rest, "{}");
AssertType(complex, "{ x: { ka: any; ki: any; }; y: number; }");

// should be:
let overEmit: { a: { ka: string, x: string }[], b: { z: string, ki: string, ku: string }, ke: string, ko: string };
AssertType(overEmit, "{ a: {    ka: string;    x: string;}[]; b: {    z: string;    ki: string;    ku: string;}; ke: string; ko: string; }");
AssertType(a, "{ ka: string; x: string; }[]");
AssertType(ka, "string");
AssertType(x, "string");
AssertType(b, "{ z: string; ki: string; ku: string; }");
AssertType(z, "string");
AssertType(ki, "string");
AssertType(ku, "string");
AssertType(ke, "string");
AssertType(ko, "string");

let { a: [{ ...nested2 }, ...y], b: { z, ...c }, ...rest2 } = overEmit;
AssertType(a, "any");
AssertType(nested2, "{ ka: string; x: string; }");
AssertType(y, "{ ka: string; x: string; }[]");
AssertType(b, "any");
AssertType(z, "string");
AssertType(c, "{ ki: string; ku: string; }");
AssertType(rest2, "{ ke: string; ko: string; }");
AssertType(overEmit, "{ a: { ka: string; x: string; }[]; b: { z: string; ki: string; ku: string; }; ke: string; ko: string; }");

({ a: [{ ...nested2 }, ...y], b: { z, ...c }, ...rest2 } = overEmit);
AssertType(({ a: [{ ...nested2 }, ...y], b: { z, ...c }, ...rest2 } = overEmit), "{ a: { ka: string; x: string; }[]; b: { z: string; ki: string; ku: string; }; ke: string; ko: string; }");
AssertType({ a: [{ ...nested2 }, ...y], b: { z, ...c }, ...rest2 } = overEmit, "{ a: { ka: string; x: string; }[]; b: { z: string; ki: string; ku: string; }; ke: string; ko: string; }");
AssertType({ a: [{ ...nested2 }, ...y], b: { z, ...c }, ...rest2 }, "{ ke: string; ko: string; a: [{ ka: string; x: string; }, ...{ ka: string; x: string; }[]]; b: { ki: string; ku: string; z: string; }; }");
AssertType(a, "[{ ka: string; x: string; }, ...{ ka: string; x: string; }[]]");
AssertType([{ ...nested2 }, ...y], "[{ ka: string; x: string; }, ...{ ka: string; x: string; }[]]");
AssertType({ ...nested2 }, "{ ka: string; x: string; }");
AssertType(nested2, "{ ka: string; x: string; }");
AssertType(...y, "{ ka: string; x: string; }");
AssertType(y, "{ ka: string; x: string; }[]");
AssertType(b, "{ ki: string; ku: string; z: string; }");
AssertType({ z, ...c }, "{ ki: string; ku: string; z: string; }");
AssertType(z, "string");
AssertType(c, "{ ki: string; ku: string; }");
AssertType(rest2, "{ ke: string; ko: string; }");
AssertType(overEmit, "{ a: { ka: string; x: string; }[]; b: { z: string; ki: string; ku: string; }; ke: string; ko: string; }");


