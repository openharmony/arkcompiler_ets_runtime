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

// === tests/cases/compiler/intersectionOfTypeVariableHasApparentSignatures.ts ===
declare function AssertType(value:any, type:string):void;
interface Component<P> {
    props: Readonly<P> & Readonly<{ children?: {} }>;
}

interface Props {
    children?: (items: {x: number}) => void
}

declare function f<T extends Props>(i: Component<T>): void;

f({
AssertType(f({    props: {        children: (({ x }) => { })    }}), "void");
AssertType(f, "<T extends Props>(Component<T>) => void");
AssertType({    props: {        children: (({ x }) => { })    }}, "{ props: { children: ({ x: number; }) => void; }; }");

    props: {
AssertType(props, "{ children: ({ x: number; }) => void; }");
AssertType({        children: (({ x }) => { })    }, "{ children: ({ x: number; }) => void; }");

        children: (({ x }) => { })
AssertType(children, "({ x: number; }) => void");
AssertType((({ x }) => { }), "({ x: number; }) => void");
AssertType(({ x }) => { }, "({ x: number; }) => void");
AssertType(x, "number");
    }
});

