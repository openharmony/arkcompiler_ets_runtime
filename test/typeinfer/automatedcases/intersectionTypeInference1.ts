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

// === tests/cases/compiler/intersectionTypeInference1.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #8801

function alert(s: string) {}

const parameterFn = (props:{store:string}) => alert(props.store)
AssertType(parameterFn, "({    store: string;}) => void");
AssertType((props:{store:string}) => alert(props.store), "({    store: string;}) => void");
AssertType(props, "{ store: string; }");
AssertType(store, "string");
AssertType(alert(props.store), "void");
AssertType(alert, "(string) => void");
AssertType(props.store, "string");

const brokenFunction = <OwnProps>(f: (p: {dispatch: number} & OwnProps) => void) => (o: OwnProps) => o
AssertType(brokenFunction, "<OwnProps>(({ dispatch: number; } & OwnProps) => void) => (OwnProps) => OwnProps");
AssertType(<OwnProps>(f: (p: {dispatch: number} & OwnProps) => void) => (o: OwnProps) => o, "<OwnProps>(({ dispatch: number; } & OwnProps) => void) => (OwnProps) => OwnProps");
AssertType(f, "({    dispatch: number;} & OwnProps) => void");
AssertType(p, "{ dispatch: number; } & OwnProps");
AssertType(dispatch, "number");
AssertType((o: OwnProps) => o, "(OwnProps) => OwnProps");
AssertType(o, "OwnProps");
AssertType(o, "OwnProps");

export const Form3 = brokenFunction(parameterFn)({store: "hello"})
AssertType(Form3, "{ store: string; }");
AssertType(brokenFunction(parameterFn)({store: "hello"}), "{ store: string; }");
AssertType(brokenFunction(parameterFn), "({ store: string; }) => { store: string; }");
AssertType(brokenFunction, "<OwnProps>(({ dispatch: number; } & OwnProps) => void) => (OwnProps) => OwnProps");
AssertType(parameterFn, "({ store: string; }) => void");
AssertType({store: "hello"}, "{ store: string; }");
AssertType(store, "string");
AssertType("hello", "string");


