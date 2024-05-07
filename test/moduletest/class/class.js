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
        super[a]  = 1;
    }
}
var par = new A;
print(par.a);

for (let i = 0; i < 2; i++) {
    class Cls{
        foo() { }
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
} catch(err) {
    print(err.name);
}
try {
    Class2022.test(new Proxy(class2022, {}));
} catch(err) {
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
    static a = 1;  // expect no print
    static length = 1;  // expect no TypeError
}

let o = {
    toString() {
        class C {
            #p(a, b) { }
        }
    }
}
let o1 = {
    [o](a, b) { }
}
print("test privateproperty class sucecess")

// TypeError
try {
    const v13 = new ArrayBuffer(16);
    function F14(a16, a17) {
        if (!new.target) { throw 'must be called with new'; }
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
} catch(err) {
    print(err.name);
}

// TypeError
try {
    class C0 {
        constructor(a2) {
            const v5 = new ArrayBuffer(10);
            function F6(a8, a9) {
                if (!new.target) { throw 'must be called with new'; }
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
} catch(err) {
    print(err.name);
}

// TypeError
try {
    const v1 = new WeakSet();
    function f2() {
        return v1;
    }
    new SharedArrayBuffer(24);
    const o8 = {
    };
    for (let i11 = -57, i12 = 10; i11 < i12; i12--) {
        o8[i12] >>= i12;
    }
    class C20 extends f2 {
    }
    new C20(83.14, 4.14, -7.50);
    C20(54.1);
    BigUint64Array();
} catch(err) {
    print(err.name);
}

// {"0":0,"1":0,"2":0,"3":0,"4":0,"5":0,"6":0,"7":0,"8":0}
// {"0":0,"1":0,"2":0,"3":0,"4":0,"5":0,"6":0,"7":0,"8":0}
// {"0":0,"1":0,"2":0,"3":0,"4":0,"5":0,"6":0,"7":0,"8":0}
{
    function arkPrint(d) { print(JSON.stringify(d)); }
    function f6() {
        return -24;
    }
    let v7 = undefined;
    class C8 {
        constructor(a10, a11, a12, a13) {
            const v16 = new Uint32Array(9);
            v7 = arkPrint(v16);
            new Uint32Array(57);
            new Float32Array(10);
            new BigInt64Array(27);
            new Int32Array(54);
            new Int32Array(51);
        }
        static 10 = -25;
    }
    new C8(0.61, f6, -4, -4, -2, C8);
    new C8(0.38, v7, -24, -25);
    new C8(0.39, 0.61, -25, -16);
    [];
    const v38 = [-1.0,4.0,1.52];
    [Uint8Array,Uint8Array];
    const o43 = {
    };
    for (const v44 in o43) {
    }
    let v45 = new Float32Array();
    v38[53] = 6;
    let v47 = o43[53];
    let v55 = --v47;
    Math.pow(v55);
    v55--;
    11 >>> -7.0;
    new Uint8Array(6);
    const v65 = [-21,10,26];
    function f67(a68, a69, a70, a71) {
        const v75 = --v45;
        !-7.0;
        -7.0 >>> v45;
        Math.pow(17, v75);
        !179;
        return 92;
    }
    f67(-7.0, Uint8Array, v47, v65);
    class C81 {
    }
    class C84 {
        179;
    }
    const v85 = new C84();
    function f86(a87, a88) {
        const o93 = {
            [v85]: Map,
            p(a90, a91, a92) {
                return a87;
            },
            "a": a87,
        };
        return o93;
    }
    function f94() {
    }
    class C96 {
    }
    for (let v97 = 0; v97 < 25; v97++) {
        const v99 = new WeakSet();
        const o105 = {
            m(a101, a102, a103, a104) {
            },
            ...v99,
            ...C96,
        };
        new Float64Array(9);
        [26,7,-19,28,26,-29,5];
        function f110(a111, a112) {
        }
        function F113() {
            if (!new.target) { throw 'must be called with new'; }
            this.a = 10;
        }
        class C117 {
            #p(a119, a120, a121) {
                for (let v122 = 0; v122 < 86; v122++) {
                    const o123 = {
                    };
                    const o124 = {
                    };
                    o124.f = -2;
                    const o125 = {
                    };
                    const o126 = {
                    };
                    o126.h = v122;
                }
            }
        }
        new C117();
        class C129 {
            constructor(a131) {
                new Boolean(-6);
                try { new a131(a131); } catch (e) {}
            }
        }
        new C129(C129);
    }
    const o138 = {
    };
}
