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

declare function print(arg:any, arg1?: any):string;

function foo(arr: number[]) {
    for (let i = 0; i < 4; i++) {
        print(arr[i]);
    }
}

function test() {
    let a1: number[] = [0, 1, 2, 3];
    foo(a1);

    let a2: number[] = [1.2, 1.3, 2.4, 3];
    for (let i = 0; i < 4; i++) {
        a2[i] = i;
    }
    foo(a2);
}


test();

function foo1(M1: number[]): number[] {
    let M:number[] = new Array(4);
    for (let i = 0; i < 4; i++) {
        M[i] = M1[i];
    }
    return M;
}

function Init() {
    let T:number[] = [8,8,4,8];
    let arr: number[] = foo1(T);
    let arr1: number[] = foo1(arr);
    for (let i = 0; i < 4; i++) {
        print(arr1[i]);
    }
}

Init();
