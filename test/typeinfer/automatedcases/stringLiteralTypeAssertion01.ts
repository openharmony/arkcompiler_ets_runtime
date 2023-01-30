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

// === tests/cases/conformance/types/stringLiteral/stringLiteralTypeAssertion01.ts ===
declare function AssertType(value:any, type:string):void;
type S = "a" | "b";
type T = S[] | S;

let s: S;
AssertType(s, "S");

let t: T;
AssertType(t, "T");

let str: string;
AssertType(str, "string");

////////////////

s = <S>t;
AssertType(s = <S>t, "S");
AssertType(s, "S");
AssertType(<S>t, "S");
AssertType(t, "T");

s = t as S;
AssertType(s = t as S, "S");
AssertType(s, "S");
AssertType(t as S, "S");
AssertType(t, "T");

s = <S>str;
AssertType(s = <S>str, "S");
AssertType(s, "S");
AssertType(<S>str, "S");
AssertType(str, "string");

s = str as S;
AssertType(s = str as S, "S");
AssertType(s, "S");
AssertType(str as S, "S");
AssertType(str, "string");

////////////////

t = <T>s;
AssertType(t = <T>s, "T");
AssertType(t, "T");
AssertType(<T>s, "T");
AssertType(s, "S");

t = s as T;
AssertType(t = s as T, "T");
AssertType(t, "T");
AssertType(s as T, "T");
AssertType(s, "S");

t = <T>str;
AssertType(t = <T>str, "T");
AssertType(t, "T");
AssertType(<T>str, "T");
AssertType(str, "string");

t = str as T;
AssertType(t = str as T, "T");
AssertType(t, "T");
AssertType(str as T, "T");
AssertType(str, "string");

////////////////

str = <string>s;
AssertType(str = <string>s, "string");
AssertType(str, "string");
AssertType(<string>s, "string");
AssertType(s, "S");

str = s as string;
AssertType(str = s as string, "string");
AssertType(str, "string");
AssertType(s as string, "string");
AssertType(s, "S");

str = <string>t;
AssertType(str = <string>t, "string");
AssertType(str, "string");
AssertType(<string>t, "string");
AssertType(t, "T");

str = t as string;
AssertType(str = t as string, "string");
AssertType(str, "string");
AssertType(t as string, "string");
AssertType(t, "T");


