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
declare function print(a:any):string;

// Proxy
class A {
    x:number;
    constructor() {
        this.x = 10;
    }
}

let p = new Proxy(new A(), {
    get() {
        return 5;
    }
})

function foo(a:A) {
    print(a.x);
}

foo(p);


// Undefined
class B {
    x:number;
    static getInstance():B {
        return undefined;
    }
}

try {
    B.getInstance().x;
} catch(e) {
    print(e instanceof TypeError);
}