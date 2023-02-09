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

// === tests/cases/compiler/indexedAccessTypeConstraints.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #14557

interface IData<T> {
    content: T;
}

type Data<T> = {
    get: <K extends keyof T>(prop: K) => T[K];
};

class Parent<M> {
    constructor(private data: Data<M>) {}
    getData(): Data<M> {
AssertType(this.data, "Data<M>");
AssertType(this, "this");
        return this.data;
    }
}

export class Foo<C> extends Parent<IData<C>> {
    getContent(): C {
AssertType(this.getData().get('content'), "C");
AssertType(this.getData().get, "<K extends "content">(K) => IData<C>[K]");
AssertType(this.getData(), "Data<IData<C>>");
AssertType(this.getData, "() => Data<IData<C>>");
AssertType(this, "this");
AssertType('content', "string");
        return this.getData().get('content');
    }
}

export class Bar<C, T extends IData<C>> extends Parent<T> {
    getContent(): C {
AssertType(this.getData().get('content'), "T["content"]");
AssertType(this.getData().get, "<K extends keyof T>(K) => T[K]");
AssertType(this.getData(), "Data<T>");
AssertType(this.getData, "() => Data<T>");
AssertType(this, "this");
AssertType('content', "string");
        return this.getData().get('content');
    }
}

// Repro from #14557

function foo<C, T extends { content: C }>(x: C, y: T['content']) {
    x = y;
AssertType(x = y, "T["content"]");
AssertType(x, "C");
AssertType(y, "T["content"]");
}


