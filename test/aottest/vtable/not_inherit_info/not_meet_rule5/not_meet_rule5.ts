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
// rule-5 check

/*
 * Chain Rules:
 * 1: local_N(B) ∩ vtable_N(B) = ∅
 * 2: local(A) ⊆ local(B)
 * 3: vtable_NT(A) ⊆ vtable_NT(B)
 * 4: local_N(A) ∩ vtable_N(B) = ∅
 * 5: vtable_N(A) ∩ local_N(B) = ∅
 */

declare function print(arg:any):string;

class A {
    x() {
        return "x";
    }
}

class B extends A {
    x:number = 1;
}

let a = new A();
print(a.x());


let b = new B();
print(b.x);

// Base class and Object check
class C {
    hasOwnProperty:number;
    constructor() {
        this.hasOwnProperty = 1;
    }
}

let c = new C();
print(c.hasOwnProperty);

class C2 {
    isPrototypeOf:number;
    constructor() {
        this.isPrototypeOf = 2;
    }
}

let c2 = new C2();
print(c2.isPrototypeOf);

class C3 {
    propertyIsEnumerable:number;
    constructor() {
        this.propertyIsEnumerable = 3;
    }
}

let c3 = new C3();
print(c3.propertyIsEnumerable);

class C4 {
    toLocaleString:number;
    constructor() {
        this.toLocaleString = 4;
    }
}

let c4 = new C4();
print(c4.toLocaleString);

class C5 {
    toString:number;
    constructor() {
        this.toString = 5;
    }
}

let c5 = new C5();
print(c5.toString);

class C6 {
    valueOf:number;
    constructor() {
        this.valueOf = 6;
    }
}

let c6 = new C6();
print(c6.valueOf);
