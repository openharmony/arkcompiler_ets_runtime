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

// === tests/cases/compiler/contravariantTypeAliasInference.ts ===
declare function AssertType(value:any, type:string):void;
type Func1<T> = (x: T) => void;
type Func2<T> = ((x: T) => void) | undefined;

declare let f1: Func1<string>;
AssertType(f1, "Func1<string>");

declare let f2: Func1<"a">;
AssertType(f2, "Func1<"a">");

declare function foo<T>(f1: Func1<T>, f2: Func1<T>): void;

foo(f1, f2);
AssertType(foo(f1, f2), "void");
AssertType(foo, "<T>(Func1<T>, Func1<T>) => void");
AssertType(f1, "Func1<string>");
AssertType(f2, "Func1<"a">");

declare let g1: Func2<string>;
AssertType(g1, "Func2<string>");

declare let g2: Func2<"a">;
AssertType(g2, "Func2<"a">");

declare function bar<T>(g1: Func2<T>, g2: Func2<T>): void;

bar(f1, f2);
AssertType(bar(f1, f2), "void");
AssertType(bar, "<T>(Func2<T>, Func2<T>) => void");
AssertType(f1, "Func1<string>");
AssertType(f2, "Func1<"a">");

bar(g1, g2);
AssertType(bar(g1, g2), "void");
AssertType(bar, "<T>(Func2<T>, Func2<T>) => void");
AssertType(g1, "Func2<string>");
AssertType(g2, "Func2<"a">");


