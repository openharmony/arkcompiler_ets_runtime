/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

class A {
    a: number;
    b: number;
    c: string;
    constructor() {
        'use sendable';
        this.a = 1;
        this.b = 2;
        this.c = "a";
    }
}
class B extends A {
    d: number;
    e: number;
    f: string;
    constructor() {
        'use sendable';
        super();
        this.d = 3;
        this.e = 4;
        this.f = "b";
    }
}
let num = 0;
function Test(obj) {    
    num++;
    print(`------ Test ${num} Start ------`);
    for (let num = 0; num < 3; num ++) {
        for (let i in obj) {
            print(i);
        }
        print(Object.keys(obj))
        print(JSON.stringify(obj, null, 2));
    }
    print(`------ Test ${num} finish ------`);
}
let a = new A();
Test(a);
let b = new B();
Test(b);