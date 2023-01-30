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

// === tests/cases/compiler/vueLikeDataAndPropsInference.ts ===
declare function AssertType(value:any, type:string):void;
interface Instance {
    _instanceBrand: never
}

type DataDef<Data, Props> = (this: Readonly<Props> & Instance) => Data

type PropsDefinition<T> = {
    [K in keyof T]: T[K]
}

interface Options<
    Data = ((this: Instance) => object),
    PropsDef = {}
    > {
    data?: Data
    props?: PropsDef
    watch?: Record<string, WatchHandler<any>>
}

type WatchHandler<T> = (val: T, oldVal: T) => void;

type ThisTypedOptions<Data, Props> =
    Options<DataDef<Data, Props>, PropsDefinition<Props>> &
    ThisType<Data & Readonly<Props> & Instance>

declare function test<Data, Props>(fn: ThisTypedOptions<Data, Props>): void;
declare function test(fn: Options): void;

test({
AssertType(test, "{ <Data, Props>(ThisTypedOptions<Data, Props>): void; (Options<(Instance) => object, {}>): void; }");
AssertType({    props: {        foo: ''    },    data(): { bar: boolean } {        return {            bar: true        }    },    watch: {        foo(newVal: string, oldVal: string): void {            this.bar = false        }    }}, "{ props: { foo: string; }; data(Readonly<{ foo: string; }> & Instance): {    bar: boolean;}; watch: { foo(string, string): void; }; }");
AssertType(test({    props: {        foo: ''    },    data(): { bar: boolean } {        return {            bar: true        }    },    watch: {        foo(newVal: string, oldVal: string): void {            this.bar = false        }    }}), "void");

    props: {
AssertType(props, "{ foo: string; }");
AssertType({        foo: ''    }, "{ foo: string; }");

        foo: ''
AssertType(foo, "string");
AssertType('', "string");

    },

    data(): { bar: boolean } {
AssertType(data, "(Readonly<{ foo: string; }> & Instance) => {    bar: boolean;}");
AssertType(bar, "boolean");

AssertType({            bar: true        }, "{ bar: true; }");
        return {

            bar: true
AssertType(bar, "boolean");
AssertType(true, "boolean");
        }
    },

    watch: {
AssertType(watch, "{ foo(string, string): void; }");
AssertType({        foo(newVal: string, oldVal: string): void {            this.bar = false        }    }, "{ foo(string, string): void; }");

        foo(newVal: string, oldVal: string): void {
AssertType(foo, "(string, string) => void");
AssertType(newVal, "string");
AssertType(oldVal, "string");

            this.bar = false
AssertType(this.bar = false, "boolean");
AssertType(this.bar, "any");
AssertType(this, "any");
AssertType(false, "boolean");
        }
    }
})

