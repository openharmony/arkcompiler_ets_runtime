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

// === tests/cases/conformance/types/mapped/mappedTypeConstraints.ts ===
declare function AssertType(value:any, type:string):void;
function f0<T extends { a: string, b: string }>(obj: Pick<T, Extract<keyof T, 'b'>>) {
    obj.b;
AssertType(obj.b, "T["b"]");
}

function f1<T extends { a: string, b: string }>(obj: Pick<T, Exclude<keyof T, 'a'>>) {
    obj.b;
AssertType(obj.b, "T["b"]");
}

function f2<T extends { a: string, b: string }, U extends { b: string, c: string }>(obj: Pick<T | U, keyof (T | U)>) {
    obj.b;
AssertType(obj.b, "(union)["b"]");
}

function f3<T extends { a: string, b: string }, U extends { b: string, c: string }>(obj: Pick<T & U, keyof (T & U)>) {
    obj.a;
AssertType(obj.a, "(T & U)["a"]");

    obj.b;
AssertType(obj.b, "(T & U)["b"]");

    obj.c;
AssertType(obj.c, "(T & U)["c"]");
}

function f4<T extends { a: string, b: string }>(obj: Record<Exclude<keyof T, 'b'> | 'c', string>) {
    obj.a;
AssertType(obj.a, "string");

    obj.c;
AssertType(obj.c, "string");
}

// Repro from #28821

type TargetProps = {
    foo: string,
    bar: string
};

const modifier = <T extends TargetProps>(targetProps: T) => {
AssertType(modifier, "<T extends TargetProps>(T) => void");
AssertType(<T extends TargetProps>(targetProps: T) => {    let {bar, ...rest} = targetProps;    rest.foo;}, "<T extends TargetProps>(T) => void");
AssertType(targetProps, "T");

    let {bar, ...rest} = targetProps;
AssertType(bar, "string");
AssertType(rest, "Omit<T, "bar">");
AssertType(targetProps, "T");

    rest.foo;
AssertType(rest.foo, "T["foo"]");

};


