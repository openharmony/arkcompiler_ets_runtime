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

declare function print(str:any):string;

class A {
    constructor() {}

    foo?(): string;

    bar(): string {
        return "bar";
    }

    bar2(): string {
        return "bar2";
    }
}

let a = new A();
print(a.bar());
print(a.bar2());
print(Reflect.ownKeys(A.prototype)); //constructor,bar,bar2

class A2 {
    constructor() {}

    foo?(): string;
    foo(): string {
        return "foo";
    }

    bar(): string {
        return "bar";
    }

    bar2(): string {
        return "bar2";
    }
}

let a2 = new A2();
print(a2.foo());
print(a2.bar());
print(a2.bar2());
print(Reflect.ownKeys(A2.prototype)); //constructor,foo,bar,bar2


class B {
    constructor() {}

    bar(): string {
        return "bar";
    }

    foo?(): string;

    bar2(): string {
        return "bar2";
    }
}

let b = new B();
print(b.bar());
print(b.bar2());
print(Reflect.ownKeys(B.prototype)); //constructor,bar,bar2

class B2 {
    constructor() {}

    bar(): string {
        return "bar";
    }

    foo?(): string;
    foo(): string {
        return "foo";
    }

    bar2(): string {
        return "bar2";
    }
}

let b2 = new B2();
print(b2.foo());
print(b2.bar());
print(b2.bar2());
print(Reflect.ownKeys(B2.prototype)); //constructor,bar,foo,bar2

// one signature but no body
class C {
    constructor() {}

    foo?(): string;

    bar(): string {
        return "test one signature but no body";
    }
}

let c = new C();
print(c.bar());

// multi-signatures but one body
class D {
    constructor() {}

    foo?(a: string): string;

    foo?(a: string): string {
        return a;
    }

    foo?(a: string, b?: string): string;

    bar(): string {
        return "test multi-signatures but one body";
    }
}

let d = new D();
print(d.foo!("D"));
print(d.bar());

// multi-signature but no body.
class E {
    constructor() {}

    foo?(): string;

    foo?(a: string): string;

    foo?(a: string, b: string): string;

    bar(): string {
        return "test multi-signatures but no body";
    }
}

E.prototype.foo = function(): string {
    return "E";
}

let e = new E();
print(e.foo!());
print(e.bar());

