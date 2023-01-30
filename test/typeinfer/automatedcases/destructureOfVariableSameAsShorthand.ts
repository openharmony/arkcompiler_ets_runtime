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

// === tests/cases/compiler/destructureOfVariableSameAsShorthand.ts ===
declare function AssertType(value:any, type:string):void;
// https://github.com/microsoft/TypeScript/issues/38969
interface AxiosResponse<T = never> {
    data: T;
}

declare function get<T = never, R = AxiosResponse<T>>(): Promise<R>;

async function main() {
    // These work examples as expected
    get().then((response) => {
AssertType(get().then((response) => {        // body is never        const body = response.data;    }), "Promise<void>");
AssertType(get().then, "<TResult1 = AxiosResponse<never>, TResult2 = never>(?(AxiosResponse<never>) => union, ?(any) => union) => Promise<union>");
AssertType(get(), "Promise<AxiosResponse<never>>");
AssertType(get, "<T = never, R = AxiosResponse<T>>() => Promise<R>");
AssertType((response) => {        // body is never        const body = response.data;    }, "(AxiosResponse<never>) => void");
AssertType(response, "AxiosResponse<never>");

        // body is never
        const body = response.data;
AssertType(body, "never");
AssertType(response.data, "never");

    })
    get().then(({ data }) => {
AssertType(get().then(({ data }) => {        // data is never    }), "Promise<void>");
AssertType(get().then, "<TResult1 = AxiosResponse<never>, TResult2 = never>(?(AxiosResponse<never>) => union, ?(any) => union) => Promise<union>");
AssertType(get(), "Promise<AxiosResponse<never>>");
AssertType(get, "<T = never, R = AxiosResponse<T>>() => Promise<R>");
AssertType(({ data }) => {        // data is never    }, "(AxiosResponse<never>) => void");
AssertType(data, "never");

        // data is never
    })
    const response = await get()
AssertType(response, "AxiosResponse<never>");
AssertType(await get(), "AxiosResponse<never>");
AssertType(get(), "Promise<AxiosResponse<never>>");
AssertType(get, "<T = never, R = AxiosResponse<T>>() => Promise<R>");

    // body is never
    const body = response.data;
AssertType(body, "never");
AssertType(response.data, "never");

    // data is never
    const { data } = await get<never>();
AssertType(data, "never");
AssertType(await get<never>(), "AxiosResponse<never>");
AssertType(get<never>(), "Promise<AxiosResponse<never>>");
AssertType(get, "<T = never, R = AxiosResponse<T>>() => Promise<R>");

    // The following did not work as expected.
    // shouldBeNever should be never, but was any
    const { data: shouldBeNever } = await get();
AssertType(data, "any");
AssertType(shouldBeNever, "never");
AssertType(await get(), "AxiosResponse<never>");
AssertType(get(), "Promise<AxiosResponse<never>>");
AssertType(get, "<T = never, R = AxiosResponse<T>>() => Promise<R>");
}

