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

class A {
    x: number;
    constructor() {
        // Mega state
        this.x = 5;
    }
}

class B extends A {
    constructor() {
        super();
    }
}
class C extends A {
    constructor() {
        super();
    }
}

class D extends A {
    constructor() {
        super();
    }
}

class E extends A {
    constructor() {
        super();
    }
}

class F extends A {
    constructor() {
        super();
    }
}

class G {
    x: number;
    constructor() {
        this.x = 5;
    }
}

function foo(a) {
    // Mega state
    a.x;
}

function bar(t) {
    print(t.x);
}

let a = new A();
let b = new B();
let c = new C();
let d = new D();
let e = new E();
let f = new F();

foo(a);
foo(b);
for (let i = 0; i < 1000; i++) {
    foo(c);
}
foo(d);
foo(e);
foo(f);

bar(b);
bar(c);

function getPrototypeOf(a)
{
    return a.prototype
}

getPrototypeOf(A);
getPrototypeOf(B);

A.prototype.foo = function() {
    print("foo");
}

a.foo();
