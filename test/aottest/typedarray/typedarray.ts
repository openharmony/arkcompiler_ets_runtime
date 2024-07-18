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

let a = new Int32Array([2, 3])
a[1] = 1 < 2
print(typeof a[1])

class B {
    x
    constructor(x) {
        this.x = x;
    }
}

let b = new B(1);
b.x = 1 < 2
print(typeof b.x)

function callback(a1, a2, a3, a4) {
    a4[a2] &= 1;
    const arr2 = new Array(a3);
    try {
        arr2.find(callback);
    } catch (e) { }
}

function testTypedArrayUsedAsUndefined() {
    const arr = new Float32Array(20);
    arr.reduceRight(callback);
    print("testTypedArrayUsedAsUndefined success");
}

testTypedArrayUsedAsUndefined();