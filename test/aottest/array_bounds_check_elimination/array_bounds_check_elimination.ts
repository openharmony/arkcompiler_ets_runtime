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

function get(a: number[], p: number): number {
    if (p <= a.length && p > 0) {
        return a[p - 1];
    }
    return 0;
}

function get2(a: number[], p: number): number {
    if (p <= a.length - 1 && p >= 0) {
        return a[p];
    }
    return 0;
}

function clear1(a: number[], x: number) {
    for (let i = 0; i < x; i++) {
        a[i] = 0;
    }
}

function clear2(b: Int32Array[], x: number) {
    for (let i = 0; i < x; i++) {
        b[i] = 0;
    }
}

function triple(a: number[], i: number) {
    a[i + 3] = 0;
    a[i + 1] = 0;
    a[i + 2] = 0;
}

function triple2(a: number[]) {
    a[0] = 0;
    a[1] = 0;
    a[2] = 0;
}

let a: number[] = [1, 2, 3];
let b: Int32Array[] = [1, 2, 3];
let x = 3;
assert_equal(get(a, x), 3);
assert_equal(get2(a, x), 0);

clear1(a, 3);
clear2(b, 3);

clear1(a, 4);
clear2(b, 4);

let c: number[] = [1, 2, 3, 4];
triple(c, 0);
triple2(a);
assert_equal(a[2], 0);
assert_equal(c[2], 0);

function TestIfCase() {
    let arr: Int32Array =  new Int32Array(1000);
    let arr2: Int32Array = new Int32Array(1000);

    for (let i:number = 0; i < arr2.length; i++) {
        if (i < arr.length) {
            arr[i];
            arr2[i];
        }
    }
}

TestIfCase();