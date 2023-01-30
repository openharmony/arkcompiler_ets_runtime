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

// === tests/cases/compiler/conditionalTypeBasedContextualTypeReturnTypeWidening.ts ===
declare function AssertType(value:any, type:string):void;
declare function useState1<S>(initialState: (S extends (() => any) ? never : S) | (() => S)): S; // No args
declare function useState2<S>(initialState: (S extends ((...args: any[]) => any) ? never : S) | (() => S)): S; // Any args

const func1 = useState1(() => () => 0);
AssertType(func1, "() => 0");
AssertType(useState1(() => () => 0), "() => 0");
AssertType(useState1, "<S>(union) => S");
AssertType(() => () => 0, "() => () => 0");
AssertType(() => 0, "() => 0");
AssertType(0, "int");

const func2 = useState2(() => () => 0);
AssertType(func2, "() => 0");
AssertType(useState2(() => () => 0), "() => 0");
AssertType(useState2, "<S>(union) => S");
AssertType(() => () => 0, "() => () => 0");
AssertType(() => 0, "() => 0");
AssertType(0, "int");

declare function useState3<S, T extends S>(initialState: (T extends (() => any) ? never : T) | (() => S)): S; // No args
declare function useState4<S, T extends S>(initialState: (T extends ((...args: any[]) => any) ? never : T) | (() => S)): S; // Any args

const func3 = useState1(() => () => 0);
AssertType(func3, "() => 0");
AssertType(useState1(() => () => 0), "() => 0");
AssertType(useState1, "<S>(union) => S");
AssertType(() => () => 0, "() => () => 0");
AssertType(() => 0, "() => 0");
AssertType(0, "int");

const func4 = useState2(() => () => 0);
AssertType(func4, "() => 0");
AssertType(useState2(() => () => 0), "() => 0");
AssertType(useState2, "<S>(union) => S");
AssertType(() => () => 0, "() => () => 0");
AssertType(() => 0, "() => 0");
AssertType(0, "int");


