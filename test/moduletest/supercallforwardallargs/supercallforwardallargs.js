/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
 * @tc.name:supercallforwardallargs
 * @tc.desc:test super call forward all args and super this validation fast paths
 * @tc.type: FUNC
 * @tc.require:
 */

function t(name, fn) {
    try {
        var r = fn();
        print(name + ": OK " + String(r));
    } catch (e) {
        print(name + ": THROW " + e.name + " :: " + e.message);
    }
}

class B1 { constructor(v) { this.value = v; } }
class D1 extends B1 {}
t("default-derived-basic", () => { const o = new D1(7); return o.value + "|" + (o instanceof D1) + "|" + (o instanceof B1); });

class B2 { constructor(v) { this.value = v; return 99; } }
class D2 extends B2 {}
t("base-returns-primitive", () => { const o = new D2(23); return o.value + "|" + (o instanceof D2); });

class B3 { constructor(v) { this.value = v; return { custom: v * 2 }; } }
class D3 extends B3 {}
t("base-returns-object", () => { const o = new D3(5); return o.custom + "|" + (o instanceof D3) + "|" + (o.value === undefined); });

function Replacement(v) { this.value = v; }
class B4 {}
class D4 extends B4 {}
Object.setPrototypeOf(D4, Replacement);
t("proto-swapped-function-base", () => { const o = new D4(43); return o.value; });

class B5 { constructor() { throw new RangeError("base boom"); } }
class D5 extends B5 {}
t("base-throws", () => new D5());

class B6 { constructor(...a) { this.n = a.length; this.sum = a.reduce((x, y) => x + y, 0); } }
class D6 extends B6 {}
t("forward-0-args", () => new D6().n);
t("forward-3-args", () => { const o = new D6(1, 2, 3); return o.n + "|" + o.sum; });
t("forward-many-args", () => { const a = new Array(10000).fill(1); const o = new D6(...a); return o.n + "|" + o.sum; });

class D7 extends B1 { constructor(v) { super(v); } }
class E7 extends D7 {}
t("derived-of-derived-default", () => new E7(11).value);

const PB = new Proxy(B1, { construct(T, args, nt) { return Reflect.construct(T, args, nt); } });
class DP extends PB {}
t("proxy-base", () => new DP(13).value);

function boundTarget(v) { this.value = v; }
t("bound-function-base", () => { const BoundBase = boundTarget.bind(null); class DBound extends BoundBase {}; return new DBound(17).value; });

class B8 { constructor() { this.marks = []; } }
let earlyName = "none";
class D8 extends B8 {
    constructor() {
        try { this.x = 1; } catch (e) { earlyName = e.name + ":" + e.message; }
        super();
        return void 0;
    }
}
t("this-before-super-caught", () => { earlyName = "none"; const o = new D8(); return earlyName + "|" + (o instanceof D8); });
class D8u extends B8 {
    constructor() {
        this.y = 2;
        super();
    }
}
t("this-before-super-uncaught", () => new D8u());

class D9 extends B1 {
    constructor() {
        super(1);
        super(2);
    }
}
t("double-super", () => new D9());

class B10 { constructor() { return 5; } }
class D10 extends B10 { constructor() { super(); return 5; } }
t("derived-returns-primitive", () => new D10());

class B11 {}
class D11 extends B11 { constructor() { super(); return { z: 3 }; } }
t("derived-returns-object", () => new D11().z);

class B12 {}
class D12 extends B12 { constructor() { return { w: 4 }; } }
t("derived-returns-object-no-super", () => new D12().w);

class B13 {}
class D13 extends B13 { constructor() { super(); } }
t("explicit-super-no-args", () => (new D13() instanceof D13));

let sideEffects = [];
class B14 { constructor(a, b) { sideEffects.push("base:" + a + "," + b); } }
class D14 extends B14 {}
t("arg-eval-order", () => { sideEffects = []; new D14(sideEffects.push("a1"), sideEffects.push("a2")); return sideEffects.join(";"); });

t("null-proto-derived", () => { class NullBase { } class DNull extends NullBase {}; Object.setPrototypeOf(DNull, null); return new DNull(); });

t("non-ctor-base-arrow", () => { const arrow = () => {}; class DA extends B1 {}; Object.setPrototypeOf(DA, arrow); return new DA(1); });

class B15 { constructor() { this.tag = new.target === D15 ? "nt-derived" : "nt-other"; } }
class D15 extends B15 {}
t("newtarget-through-super", () => new D15().tag);

class B16 { static get [Symbol.species]() { return null; } constructor(v) { this.v = v; } }
class D16 extends B16 {}
t("species-noise-irrelevant", () => new D16(3).v);

class B17 { constructor() { if (new.target === B17) { this.direct = true; } this.k = 1; } }
class D17 extends B17 {}
t("reflect-construct-forward", () => { const o = Reflect.construct(D17, [], D17); return o.k + "|" + (o.direct === undefined); });

t("native-base-throws-symbol", () => { class DSym extends Symbol {}; return new DSym(); });

class DNum extends Number {}
t("native-base-number", () => { const o = new DNum(42); return (o instanceof DNum) + "|" + (o instanceof Number) + "|" + o.valueOf(); });

class DArr extends Array {}
t("native-base-array", () => { const o = new DArr(1, 2, 3); return o.length + "|" + (o instanceof DArr) + "|" + o.join(","); });

class DDate extends Date {}
t("native-base-date", () => { const o = new DDate(0); return (o instanceof DDate) + "|" + o.getTime(); });

t("nonctor-native-base-bigint", () => { class DBig extends BigInt {}; return new DBig(1); });

class DErr extends TypeError {}
t("native-base-error", () => { const o = new DErr("boom"); return (o instanceof DErr) + "|" + (o instanceof TypeError) + "|" + o.message; });

class DRegExp extends RegExp {}
t("native-base-regexp-badflags", () => new DRegExp("a", "bogusflags"));

print("SUITE_END");