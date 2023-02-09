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

// === tests/cases/compiler/complicatedIndexesOfIntersectionsAreInferencable.ts ===
declare function AssertType(value:any, type:string):void;
interface FormikConfig<Values> {
    initialValues: Values;
    validate?: (props: Values) => void;
    validateOnChange?: boolean;
}

declare function Func<Values = object, ExtraProps = {}>(
    x: (string extends "validate" | "initialValues" | keyof ExtraProps
        ? Readonly<FormikConfig<Values> & ExtraProps>
        : Pick<Readonly<FormikConfig<Values> & ExtraProps>, "validate" | "initialValues" | Exclude<keyof ExtraProps, "validateOnChange">>
        & Partial<Pick<Readonly<FormikConfig<Values> & ExtraProps>, "validateOnChange" | Extract<keyof ExtraProps, "validateOnChange">>>)
): void;

Func({
AssertType(Func({    initialValues: {        foo: ""    },    validate: props => {        props.foo;    }}), "void");
AssertType(Func, "<Values = object, ExtraProps = {}>(string extends union ? Readonly<FormikConfig<Values> & ExtraProps> : Pick<Readonly<FormikConfig<Values> & ExtraProps>, union> & Partial<Pick<Readonly<FormikConfig<Values> & ExtraProps>, union>>) => void");
AssertType({    initialValues: {        foo: ""    },    validate: props => {        props.foo;    }}, "{ initialValues: { foo: string; }; validate: ({ foo: string; }) => void; }");

    initialValues: {
AssertType(initialValues, "{ foo: string; }");
AssertType({        foo: ""    }, "{ foo: string; }");

        foo: ""
AssertType(foo, "string");
AssertType("", "string");

    },
    validate: props => {
AssertType(validate, "({ foo: string; }) => void");
AssertType(props => {        props.foo;    }, "({ foo: string; }) => void");
AssertType(props, "{ foo: string; }");

        props.foo;
AssertType(props.foo, "string");
    }
});

