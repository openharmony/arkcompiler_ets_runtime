/*
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
 */
declare function print(arg:any):string;

function bar() {
}

function bar2() {
}

async function foo() {
    let a:number[] = [];
    for (let i = 0; i < 1; ++i) {
        await bar();
        for (let j = 0; j < 1; ++j) {
            await bar2();
        }
    }
    print("foo");
}

function createArray() {
    return new Array<number>(1);
}

async function foo2() {
    let a:Array<number> = createArray();
    for (let i = 0; i < a.length; ++i) {
        await bar();
        for (let j = 0; j < 1; ++j) {
            await bar2();
        }
    }
    print("foo2");
}

async function foo3() {
    for(let i = 0; i<1; ++i) {
        bar();
    }
    for (let i = 0; i < 1; ++i) {
        await bar();
        for (let j = 0; j < 1; ++j) {
            await bar2();
        }
    }
    print("foo3");
}

foo();

foo2();

foo3();
