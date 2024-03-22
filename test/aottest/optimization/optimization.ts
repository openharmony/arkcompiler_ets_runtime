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
declare function print(arg:any):void;
let F : number[] = [0, 1, 2]
let G : number[] = [0, 1, 2]

function tryLoopOpt(f: number[], v : number): void {
    let idx = 1 - 1;
    let ret = f[idx];
    for (let i = 0; i < f.length; ++i) {
        f[i] += 1;
        f[i] += v;
    }
    print(ret);
    print(f[idx]);
    print(f[idx + 1]);
}

tryLoopOpt(F, <number><Object>'a');

function tryMergeOpt(g: number[], v: number): void {
    let idx = 1 - 1;
    let ret = g[idx];
    if (g[idx] < 10) {
        g[idx] -= 10;
    } else {
        v++;
    }
    print(ret + v);
    print(g[idx]);
}

tryMergeOpt(G, <number><Object>'b');

function binarySearch(array: number[], target: number): number {
    let low : number = 0;
    let high : number = array.length - 1;

    while (low <= high) {
        let mid: number = (low + high) >>> 1;
        if (array[mid] == target) {
            return mid;
        } else if (array[mid] < target) {
            low = mid + 1;
        } else if (array[mid] > target) {
            high = mid - 1;
        }
    }
    return -1;
}

let array: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
let ret = binarySearch(array, 3);
print(ret);

function foo(arr : number[]) {
    let t = 0 + 0;
    let sum = arr[t];
    for (let i = 0; i < 2; ++i) {
        sum += arr[t];
        arr[t] = 999;
    }
    return sum;
}
print(foo(array));

class Obj {
    a:number;
    b:number;
    constructor() {
        this.a = 1;
        this.b = 4;
    }
}
function LoadPropertyOpt(obj : Obj) : number {
    let x1 = obj.a;  // 1
    let y1 = obj.b;  // 4
    obj.a = 16;
    let x2 = obj.a;
    let y2 = obj.b;
    return x1 + y1 + x2 + y2;
}
let obj : Obj = new Obj();
print(LoadPropertyOpt(obj));

let x = 1;
const size = (x === undefined) ? 1 : 2;
const cleanupFunc = (x === undefined) ? 1 : 2;
print(size);
print(cleanupFunc);

function foo1() : void {
    print(0);
}
function foo2(i : number) : void {
    let a = {x: null, y:233, z:466};
    let b = {y: 5};
    a.x = b;
    if (i > 10) {
        a.x.y = i;
    }
    foo1()
    return a.x.y;
}
function foo3(i : number) : void {
    let a = {x: null, y:233, z:466};
    let b = {y: 5};
    a.x = b;
    if (i > 10) {
        a.x.y = i;
    }
    foo1()
    return a.x;
}
print(foo2(1))
print(foo2(5))
print(foo2(10))
print(foo2(100))
print(foo2(11))
print(foo3(1))
print(foo3(11))
