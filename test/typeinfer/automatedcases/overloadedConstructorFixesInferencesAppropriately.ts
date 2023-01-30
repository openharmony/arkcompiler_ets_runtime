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

// === tests/cases/compiler/overloadedConstructorFixesInferencesAppropriately.ts ===
declare function AssertType(value:any, type:string):void;
interface Box<T> {
    v: T;
}

interface ErrorResult {
    readonly error: true
}

interface AsyncLoaderProps<TResult extends {}> {
    readonly asyncLoad: () => Box<TResult>;
    readonly children: (result: Exclude<TResult, ErrorResult>) => string;
}

class AsyncLoader<TResult extends {}> {
    constructor(props: string, context: any);
    constructor(props: AsyncLoaderProps<TResult>);
    constructor(...args: any[]) {}
}

function load(): Box<{ success: true } | ErrorResult> {
AssertType(null as any, "any");
AssertType(null, "null");
    return null as any;
}

new AsyncLoader({
AssertType(new AsyncLoader({    asyncLoad: load,    children: result => result.success as any,}), "AsyncLoader<union>");
AssertType(AsyncLoader, "typeof AsyncLoader");
AssertType({    asyncLoad: load,    children: result => result.success as any,}, "{ asyncLoad: () => Box<union>; children: ({ success: true; }) => any; }");

    asyncLoad: load,
AssertType(asyncLoad, "() => Box<union>");
AssertType(load, "() => Box<union>");

    children: result => result.success as any,
AssertType(children, "({ success: true; }) => any");
AssertType(result => result.success as any, "({ success: true; }) => any");
AssertType(result, "{ success: true; }");
AssertType(result.success as any, "any");
AssertType(result.success, "boolean");

}); // should work fine


