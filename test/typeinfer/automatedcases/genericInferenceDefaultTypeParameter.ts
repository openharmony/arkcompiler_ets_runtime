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

// === tests/cases/compiler/genericInferenceDefaultTypeParameter.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #50858

type Type = {
    a: (e: string) => void;
    b: (e: number) => void;
}

declare function f1<T extends keyof Type = "a">(props: Type[T]): void;

f1(event => { });
AssertType(f1(event => { }), "void");
AssertType(f1, "<T extends keyof Type = "a">(Type[T]) => void");
AssertType(event => { }, "(string) => void");
AssertType(event, "string");

f1<"a">(event => { });
AssertType(f1<"a">(event => { }), "void");
AssertType(f1, "<T extends keyof Type = "a">(Type[T]) => void");
AssertType(event => { }, "(string) => void");
AssertType(event, "string");

f1<"b">(event => { });
AssertType(f1<"b">(event => { }), "void");
AssertType(f1, "<T extends keyof Type = "a">(Type[T]) => void");
AssertType(event => { }, "(number) => void");
AssertType(event, "number");


