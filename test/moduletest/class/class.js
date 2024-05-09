/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

/*
 * @tc.name:class
 * @tc.desc:test class
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
class Parent {
    constructor(x) {
        this.x = x;
    }

    static toString() {
        return 'parent';
    }
}

class Child extends Parent {
    constructor(x, y) {
        super(x);
        this.y = y;
    }

    value() {
        return this.x * this.y;
    }

    static toString() {
        return super.toString() + ' child';
    }
}

var c = new Child(2, 3);
print(c.value());
print(Child.toString());

try {
    class C {
        a = 1;
    }

    class D extends C {
        constructo() {
            delete super.a;
        }
    }

    d = new D();
} catch (err) {
    print("PASS");
}

class A {
    a = 10;
}

class B extends A {
    constructor() {
        let a = "a";
        super[a] = 1;
    }
}

var par = new A;
print(par.a);

for (let i = 0; i < 2; i++) {
    class Cls {
        foo() {
        }
    }

    if (i == 0) {
        Cls.prototype.foo.x = 1;
    }
    print(Cls.prototype.foo.x);
}

class Class2022 {
    public = 12;
    #private = 34;
    static static_public = 56;
    static #static_private = 78;

    static test(obj) {
        print(obj.public);
        print(obj.#private);
        print(obj.static_public);
        print(obj.#static_private);
    }
}

var class2022 = new Class2022();
try {
    Class2022.test(class2022);
} catch (err) {
    print(err.name);
}
try {
    Class2022.test(new Proxy(class2022, {}));
} catch (err) {
    print(err.name);
}

function foo() {

}

class Class2024 {
    static #g;
    a = [1, 2, 3];
    #c = foo;
}

var class2024 = new Class2024();
print("test successful!");

class StaticTest {
    static set a(a) {
        print(a);
    }

    static a = 1; // expect no print
    static length = 1; // expect no TypeError
}

let o = {
    toString() {
        class C {
            #p(a, b) {
            }
        }
    }
}
let o1 = {
    [o](a, b) {
    }
}
print("test privateproperty class sucecess")

// TypeError
try {
    const v13 = new ArrayBuffer(16);

    function F14(a16, a17) {
        if (!new.target) {
            throw 'must be called with new';
        }

        function f18(a19, a20, a21) {
        }

        const v23 = `-2`;

        async function f25(a26, a27, a28, a29) {
            return v23;
        }

        f25();
        const v33 = new BigUint64Array(31);
        const o34 = {
            ...v33,
            ...v33,
        };
        Object.defineProperty(o34, 4, { set: f18 });
    }

    const v35 = new F14(ArrayBuffer);
    const t33 = 64n;
    new t33(v35, v13);
} catch (err) {
    print(err.name);
}

// TypeError
try {
    class C0 {
        constructor(a2) {
            const v5 = new ArrayBuffer(10);

            function F6(a8, a9) {
                if (!new.target) {
                    throw 'must be called with new';
                }
                const v12 = new BigUint64Array(32);
                const o13 = {
                    ...v12,
                };
                Object.defineProperty(o13, 4, { set: F6 });
            }

            new F6(ArrayBuffer, v5);
            new a2();
        }
    }

    new C0(C0);
} catch (err) {
    print(err.name);
}

// TypeError
try {
    const v1 = new WeakSet();

    function f2() {
        return v1;
    }

    new SharedArrayBuffer(24);
    const o8 = {};
    for (let i11 = -57, i12 = 10; i11 < i12; i12--) {
        o8[i12] >>= i12;
    }

    class C20 extends f2 {
    }

    new C20(83.14, 4.14, -7.50);
    C20(54.1);
    BigUint64Array();
} catch (err) {
    print(err.name);
}
