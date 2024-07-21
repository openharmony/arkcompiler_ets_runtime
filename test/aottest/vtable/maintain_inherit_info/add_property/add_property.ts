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
    constructor() {
        this.x = 1;
        this.y = 2;
    }
}

class B extends A {
    z:string;
    constructor() {
        super();
        this.z = "z";
    }
}

let b = new B();
b.z1 = 123;
b.z2 = 456;
print(b.x);
print(b.y);
print(b.z);
print(b.z1);
print(b.z2);


A.prototype.t = 789;  // break inherit info
let b1 = new B();
b1.z1 = 321;         // transion success, has cleared inherit info
print(b1.x);  // occur depot
print(b1.y);
print(b1.z);
