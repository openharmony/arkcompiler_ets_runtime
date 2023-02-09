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

// === tests/cases/compiler/fallbackToBindingPatternForTypeInference.ts ===
declare function AssertType(value:any, type:string):void;
declare function trans<T>(f: (x: T) => string): number;
trans(({a}) => a);
AssertType(trans(({a}) => a), "number");
AssertType(trans, "<T>((T) => string) => number");
AssertType(({a}) => a, "({ a: any; }) => any");
AssertType(a, "any");
AssertType(a, "any");

trans(([b,c]) => 'foo');
AssertType(trans(([b,c]) => 'foo'), "number");
AssertType(trans, "<T>((T) => string) => number");
AssertType(([b,c]) => 'foo', "([any, any]) => string");
AssertType(b, "any");
AssertType(c, "any");
AssertType('foo', "string");

trans(({d: [e,f]}) => 'foo');
AssertType(trans(({d: [e,f]}) => 'foo'), "number");
AssertType(trans, "<T>((T) => string) => number");
AssertType(({d: [e,f]}) => 'foo', "({ d: [any, any]; }) => string");
AssertType(d, "any");
AssertType(e, "any");
AssertType(f, "any");
AssertType('foo', "string");

trans(([{g},{h}]) => 'foo');
AssertType(trans(([{g},{h}]) => 'foo'), "number");
AssertType(trans, "<T>((T) => string) => number");
AssertType(([{g},{h}]) => 'foo', "([{ g: any; }, { h: any; }]) => string");
AssertType(g, "any");
AssertType(h, "any");
AssertType('foo', "string");

trans(({a, b = 10}) => a);
AssertType(trans(({a, b = 10}) => a), "number");
AssertType(trans, "<T>((T) => string) => number");
AssertType(({a, b = 10}) => a, "({ a: any; b?: number; }) => any");
AssertType(a, "any");
AssertType(b, "number");
AssertType(10, "int");
AssertType(a, "any");


