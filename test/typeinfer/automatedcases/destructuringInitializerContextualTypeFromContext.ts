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

// === tests/cases/compiler/destructuringInitializerContextualTypeFromContext.ts ===
declare function AssertType(value:any, type:string):void;
interface SFC<P = {}> {
    (props: P & { children?: any }): any | null;
}

interface Props {
    name: "Apollo" | "Artemis" | "Dionysus" | "Persephone";
}

const Parent: SFC<Props> = ({
AssertType(Parent, "SFC<Props>");
AssertType(({    children,    name = "Artemis",    ...props}) => Child({name, ...props}), "(Props & { children?: any; }) => any");

    children,
AssertType(children, "any");

    name = "Artemis",
AssertType(name, "union");
AssertType("Artemis", "string");

    ...props
AssertType(props, "{}");

}) => Child({name, ...props});
AssertType(Child({name, ...props}), "any");
AssertType(Child, "SFC<Props>");
AssertType({name, ...props}, "{ name: union; }");
AssertType(name, "union");
AssertType(props, "{}");

const Child: SFC<Props> = ({
AssertType(Child, "SFC<Props>");
AssertType(({    children,    name = "Artemis",    ...props}) => `name: ${name} props: ${JSON.stringify(props)}`, "(Props & { children?: any; }) => string");

    children,
AssertType(children, "any");

    name = "Artemis",
AssertType(name, "union");
AssertType("Artemis", "string");

    ...props
AssertType(props, "{}");

}) => `name: ${name} props: ${JSON.stringify(props)}`;
AssertType(`name: ${name} props: ${JSON.stringify(props)}`, "string");
AssertType(name, "union");
AssertType(JSON.stringify(props), "string");
AssertType(JSON.stringify, "{ (any, ?union, ?union): string; (any, ?union, ?union): string; }");
AssertType(props, "{}");

// Repro from #29189

declare function f(g: (as: string[]) => void): void
f(([_1, _2 = undefined]) => undefined)
AssertType(f(([_1, _2 = undefined]) => undefined), "void");
AssertType(f, "((string[]) => void) => void");
AssertType(([_1, _2 = undefined]) => undefined, "(string[]) => undefined");
AssertType(_1, "string");
AssertType(_2, "union");
AssertType(undefined, "undefined");
AssertType(undefined, "undefined");


