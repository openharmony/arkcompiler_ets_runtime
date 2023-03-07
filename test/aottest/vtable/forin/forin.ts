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

// @ts-nocheck
declare function print(a0:any, a1?:any):string;

class A {
    x:number;
    y:number;
    z:number;
    constructor() {
        this.y = 3;
        this.z = 5;
    }
}

let a = new A();
for (let key in a) {
    print(key, a[key]);
}

// delete local property (hole)
delete a.x;
for (let key in a) {
    print(key, a[key]);
}

// delete local property (no hole)
delete a.y;
for (let key in a) {
    print(key, a[key]);
}

// add property
a.m = 6;
for (let key in a) {
    print(key, a[key]);
}

// change prototype
Object.defineProperty(A.prototype, "foo", { value: function() {}})
for (let key in a) {
    print(key, a[key]);
}
