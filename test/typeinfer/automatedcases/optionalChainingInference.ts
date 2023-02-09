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

// === tests/cases/conformance/expressions/optionalChaining/optionalChainingInference.ts ===
declare function AssertType(value:any, type:string):void;
// https://github.com/microsoft/TypeScript/issues/34579
declare function unbox<T>(box: { value: T | undefined }): T;
declare const su: string | undefined;
AssertType(su, "string");

declare const fnu: (() => number) | undefined;
AssertType(fnu, "() => number");

declare const osu: { prop: string } | undefined;
AssertType(osu, "{ prop: string; }");
AssertType(prop, "string");

declare const ofnu: { prop: () => number } | undefined;
AssertType(ofnu, "{ prop: () => number; }");
AssertType(prop, "() => number");

const b1 = { value: su?.length };
AssertType(b1, "{ value: number; }");
AssertType({ value: su?.length }, "{ value: number; }");
AssertType(value, "number");
AssertType(su?.length, "number");

const v1: number = unbox(b1);
AssertType(v1, "number");
AssertType(unbox(b1), "number");
AssertType(unbox, "<T>({ value: T; }) => T");
AssertType(b1, "{ value: number; }");

const b2 = { value: su?.length as number | undefined };
AssertType(b2, "{ value: number; }");
AssertType({ value: su?.length as number | undefined }, "{ value: number; }");
AssertType(value, "number");
AssertType(su?.length as number | undefined, "number");
AssertType(su?.length, "number");

const v2: number = unbox(b2);
AssertType(v2, "number");
AssertType(unbox(b2), "number");
AssertType(unbox, "<T>({ value: T; }) => T");
AssertType(b2, "{ value: number; }");

const b3: { value: number | undefined } = { value: su?.length };
AssertType(b3, "{ value: union; }");
AssertType(value, "number");
AssertType({ value: su?.length }, "{ value: number; }");
AssertType(value, "number");
AssertType(su?.length, "number");

const v3: number = unbox(b3);
AssertType(v3, "number");
AssertType(unbox(b3), "number");
AssertType(unbox, "<T>({ value: T; }) => T");
AssertType(b3, "{ value: number; }");

const b4 = { value: fnu?.() };
AssertType(b4, "{ value: number; }");
AssertType({ value: fnu?.() }, "{ value: number; }");
AssertType(value, "number");
AssertType(fnu?.(), "number");
AssertType(fnu, "() => number");

const v4: number = unbox(b4);
AssertType(v4, "number");
AssertType(unbox(b4), "number");
AssertType(unbox, "<T>({ value: T; }) => T");
AssertType(b4, "{ value: number; }");

const b5 = { value: su?.["length"] };
AssertType(b5, "{ value: number; }");
AssertType({ value: su?.["length"] }, "{ value: number; }");
AssertType(value, "number");
AssertType(su?.["length"], "number");
AssertType(su, "string");
AssertType("length", "string");

const v5: number = unbox(b5);
AssertType(v5, "number");
AssertType(unbox(b5), "number");
AssertType(unbox, "<T>({ value: T; }) => T");
AssertType(b5, "{ value: number; }");

const b6 = { value: osu?.prop.length };
AssertType(b6, "{ value: number; }");
AssertType({ value: osu?.prop.length }, "{ value: number; }");
AssertType(value, "number");
AssertType(osu?.prop.length, "number");

const v6: number = unbox(b6);
AssertType(v6, "number");
AssertType(unbox(b6), "number");
AssertType(unbox, "<T>({ value: T; }) => T");
AssertType(b6, "{ value: number; }");

const b7 = { value: osu?.prop["length"] };
AssertType(b7, "{ value: number; }");
AssertType({ value: osu?.prop["length"] }, "{ value: number; }");
AssertType(value, "number");
AssertType(osu?.prop["length"], "number");
AssertType(osu?.prop, "string");
AssertType("length", "string");

const v7: number = unbox(b7);
AssertType(v7, "number");
AssertType(unbox(b7), "number");
AssertType(unbox, "<T>({ value: T; }) => T");
AssertType(b7, "{ value: number; }");

const b8 = { value: ofnu?.prop() };
AssertType(b8, "{ value: number; }");
AssertType({ value: ofnu?.prop() }, "{ value: number; }");
AssertType(value, "number");
AssertType(ofnu?.prop(), "number");
AssertType(ofnu?.prop, "() => number");

const v8: number = unbox(b8);
AssertType(v8, "number");
AssertType(unbox(b8), "number");
AssertType(unbox, "<T>({ value: T; }) => T");
AssertType(b8, "{ value: number; }");



