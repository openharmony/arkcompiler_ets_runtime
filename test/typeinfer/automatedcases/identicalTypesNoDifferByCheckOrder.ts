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

// === tests/cases/compiler/identicalTypesNoDifferByCheckOrder.ts ===
declare function AssertType(value:any, type:string):void;
interface SomeProps {
    x?: string;
    y?: number;
    renderAs?: FunctionComponent1<SomeProps>
}

type SomePropsX = Required<Pick<SomeProps, "x">> & Omit<SomeProps, "x">;

interface SomePropsClone {
    x?: string;
    y?: number;
    renderAs?: FunctionComponent2<SomeProps>
}

type SomePropsCloneX = Required<Pick<SomePropsClone, "x">> & Omit<SomePropsClone, "x">;

type Validator<T> = {(): boolean, opt?: T};
type WeakValidationMap<T> = {[K in keyof T]?: null extends T[K] ? Validator<T[K] | null | undefined> : Validator<T[K]>};

interface FunctionComponent1<P> {
    (props: P & { children?: unknown }): void;
    propTypes?: WeakValidationMap<P>;
}

interface FunctionComponent2<P> {
    (props: P & { children?: unknown }): void;
    propTypes?: WeakValidationMap<P>;
}

function needsComponentOfSomeProps3(...x: SomePropsClone[]): void {}
const comp3: FunctionComponent2<SomePropsCloneX> = null as any;
AssertType(comp3, "FunctionComponent2<SomePropsCloneX>");
AssertType(null as any, "any");
AssertType(null, "null");

needsComponentOfSomeProps3({ renderAs: comp3 });
AssertType(needsComponentOfSomeProps3({ renderAs: comp3 }), "void");
AssertType(needsComponentOfSomeProps3, "(...SomePropsClone[]) => void");
AssertType({ renderAs: comp3 }, "{ renderAs: FunctionComponent2<SomePropsCloneX>; }");
AssertType(renderAs, "FunctionComponent2<SomePropsCloneX>");
AssertType(comp3, "FunctionComponent2<SomePropsCloneX>");

function needsComponentOfSomeProps2(...x: SomeProps[]): void {}
const comp2: FunctionComponent1<SomePropsX> = null as any;
AssertType(comp2, "FunctionComponent1<SomePropsX>");
AssertType(null as any, "any");
AssertType(null, "null");

needsComponentOfSomeProps2({ renderAs: comp2 });
AssertType(needsComponentOfSomeProps2({ renderAs: comp2 }), "void");
AssertType(needsComponentOfSomeProps2, "(...SomeProps[]) => void");
AssertType({ renderAs: comp2 }, "{ renderAs: FunctionComponent1<SomePropsX>; }");
AssertType(renderAs, "FunctionComponent1<SomePropsX>");
AssertType(comp2, "FunctionComponent1<SomePropsX>");


