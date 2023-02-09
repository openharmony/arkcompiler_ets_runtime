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

// === tests/cases/compiler/inferenceOfNullableObjectTypesWithCommonBase.ts ===
declare function AssertType(value:any, type:string):void;
function equal<T>(a: T, b: T) { }

let v = null!;
AssertType(v, "never");
AssertType(null!, "never");
AssertType(null, "null");

// Object types with common base types

type B = { foo: string }
type D = { foo: string; bar: number }

equal(v as B, v as undefined | D)
AssertType(equal(v as B, v as undefined | D), "void");
AssertType(equal, "<T>(T, T) => void");
AssertType(v as B, "B");
AssertType(v, "never");
AssertType(v as undefined | D, "union");
AssertType(v, "never");

equal(v as undefined | D, v as B)
AssertType(equal(v as undefined | D, v as B), "void");
AssertType(equal, "<T>(T, T) => void");
AssertType(v as undefined | D, "union");
AssertType(v, "never");
AssertType(v as B, "B");
AssertType(v, "never");

equal<undefined | B>(v as B, v as undefined | D)
AssertType(equal<undefined | B>(v as B, v as undefined | D), "void");
AssertType(equal, "<T>(T, T) => void");
AssertType(v as B, "B");
AssertType(v, "never");
AssertType(v as undefined | D, "union");
AssertType(v, "never");

equal<undefined | B>(v as undefined | D, v as B)
AssertType(equal<undefined | B>(v as undefined | D, v as B), "void");
AssertType(equal, "<T>(T, T) => void");
AssertType(v as undefined | D, "union");
AssertType(v, "never");
AssertType(v as B, "B");
AssertType(v, "never");

equal(v as B, v as undefined)
AssertType(equal(v as B, v as undefined), "void");
AssertType(equal, "<T>(T, T) => void");
AssertType(v as B, "B");
AssertType(v, "never");
AssertType(v as undefined, "undefined");
AssertType(v, "never");

equal(v as undefined, v as B)
AssertType(equal(v as undefined, v as B), "void");
AssertType(equal, "<T>(T, T) => void");
AssertType(v as undefined, "undefined");
AssertType(v, "never");
AssertType(v as B, "B");
AssertType(v, "never");

equal(v as B, v as D)
AssertType(equal(v as B, v as D), "void");
AssertType(equal, "<T>(T, T) => void");
AssertType(v as B, "B");
AssertType(v, "never");
AssertType(v as D, "D");
AssertType(v, "never");

equal(v as D, v as B)
AssertType(equal(v as D, v as B), "void");
AssertType(equal, "<T>(T, T) => void");
AssertType(v as D, "D");
AssertType(v, "never");
AssertType(v as B, "B");
AssertType(v, "never");

equal(v as B, v as B | undefined)
AssertType(equal(v as B, v as B | undefined), "void");
AssertType(equal, "<T>(T, T) => void");
AssertType(v as B, "B");
AssertType(v, "never");
AssertType(v as B | undefined, "union");
AssertType(v, "never");

equal(v as B | undefined, v as B)
AssertType(equal(v as B | undefined, v as B), "void");
AssertType(equal, "<T>(T, T) => void");
AssertType(v as B | undefined, "union");
AssertType(v, "never");
AssertType(v as B, "B");
AssertType(v, "never");

equal(v as 'a' | undefined, v as 'b');
AssertType(equal(v as 'a' | undefined, v as 'b'), "void");
AssertType(equal, "<T>(T, T) => void");
AssertType(v as 'a' | undefined, "union");
AssertType(v, "never");
AssertType(v as 'b', "string");
AssertType(v, "never");

equal(v as 'a', v as 'b' | undefined);
AssertType(equal(v as 'a', v as 'b' | undefined), "void");
AssertType(equal, "<T>(T, T) => void");
AssertType(v as 'a', "string");
AssertType(v, "never");
AssertType(v as 'b' | undefined, "union");
AssertType(v, "never");

equal(v as 'a' | undefined, v as 'b' | null);
AssertType(equal(v as 'a' | undefined, v as 'b' | null), "void");
AssertType(equal, "<T>(T, T) => void");
AssertType(v as 'a' | undefined, "union");
AssertType(v, "never");
AssertType(v as 'b' | null, "union");
AssertType(v, "never");
AssertType(null, "null");

equal(v as 'a' | null, v as 'b' | undefined);
AssertType(equal(v as 'a' | null, v as 'b' | undefined), "void");
AssertType(equal, "<T>(T, T) => void");
AssertType(v as 'a' | null, "union");
AssertType(v, "never");
AssertType(null, "null");
AssertType(v as 'b' | undefined, "union");
AssertType(v, "never");

equal(v as string, v as string & { tag: 'foo' } | undefined);
AssertType(equal(v as string, v as string & { tag: 'foo' } | undefined), "void");
AssertType(equal, "<T>(T, T) => void");
AssertType(v as string, "string");
AssertType(v, "never");
AssertType(v as string & { tag: 'foo' } | undefined, "union");
AssertType(v, "never");
AssertType(tag, "string");

equal(v as string & { tag: 'foo' } | undefined, v as string);
AssertType(equal(v as string & { tag: 'foo' } | undefined, v as string), "void");
AssertType(equal, "<T>(T, T) => void");
AssertType(v as string & { tag: 'foo' } | undefined, "union");
AssertType(v, "never");
AssertType(tag, "string");
AssertType(v as string, "string");
AssertType(v, "never");


