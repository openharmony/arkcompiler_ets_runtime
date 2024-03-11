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

declare function assert_equal(a: Object, b: Object):void;

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
assert_equal(a.bar(), "bar");
assert_equal(a.bar2(), "bar2");
assert_equal(Reflect.ownKeys(A.prototype), ["constructor", "bar", "bar2"]); //constructor,bar,bar2

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
assert_equal(a2.foo(), "foo");
assert_equal(a2.bar(), "bar");
assert_equal(a2.bar2(), "bar2");
assert_equal(Reflect.ownKeys(A2.prototype), ["constructor", "foo", "bar", "bar2"]); //constructor,foo,bar,bar2


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
assert_equal(b.bar(), "bar");
assert_equal(b.bar2(), "bar2");
assert_equal(Reflect.ownKeys(B.prototype), ["constructor", "bar", "bar2"]); //constructor,bar,bar2

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
assert_equal(b2.foo(), "foo");
assert_equal(b2.bar(), "bar");
assert_equal(b2.bar2(), "bar2");
assert_equal(Reflect.ownKeys(B2.prototype), ["constructor", "bar", "foo", "bar2"]); //constructor,bar,foo,bar2

// one signature but no body
class C {
    constructor() {}

    foo?(): string;

    bar(): string {
        return "test one signature but no body";
    }
}

let c = new C();
assert_equal(c.bar(), "test one signature but no body");

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
assert_equal(d.foo!("D"), "D");
assert_equal(d.bar(), "test multi-signatures but one body");

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
assert_equal(e.foo!(), "E");
assert_equal(e.bar(), "test multi-signatures but no body");

