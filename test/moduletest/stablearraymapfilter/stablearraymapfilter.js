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
 * @tc.name:stablearraymapfilter
 * @tc.desc:test stable array map/filter fast property creation guards
 * @tc.type: FUNC
 * @tc.require:
 */

function t(name, fn) {
    try {
        print(name + ": OK " + String(fn()));
    } catch (e) {
        print(name + ": THROW " + e.name + " :: " + e.message);
    }
}

t("map-basic", () => [1, 2, 3].map(x => x * 2).join(","));
t("filter-basic", () => [1, 2, 3, 4].filter(x => x % 2 === 0).join(","));

t("map-nonextensible-species", () => {
    const src = [1];
    src.constructor = { [Symbol.species]: function (len) { const out = new Array(len); Object.preventExtensions(out); return out; } };
    return src.map(x => x).join(",");
});
t("filter-nonextensible-species", () => {
    const src = [1, 2];
    src.constructor = { [Symbol.species]: function (len) { const out = new Array(len); Object.preventExtensions(out); return out; } };
    return src.filter(x => true).join(",");
});
t("map-frozen-species", () => {
    const src = [1];
    src.constructor = { [Symbol.species]: function (len) { return Object.freeze(new Array(len)); } };
    return src.map(x => x * 3).join(",");
});
t("map-species-nonwritable-index", () => {
    const src = [1, 2];
    src.constructor = { [Symbol.species]: function (len) {
        const out = new Array(len);
        Object.defineProperty(out, 0, { value: "locked", writable: false, configurable: false });
        return out;
    } };
    return src.map(x => x * 10).join(",");
});
t("map-species-accessor-index", () => {
    const log = [];
    const src = [5];
    src.constructor = { [Symbol.species]: function (len) {
        const out = {};
        Object.defineProperty(out, "length", { value: len, writable: true });
        Object.defineProperty(out, 0, { set(v) { log.push("set0=" + v); }, configurable: true });
        return out;
    } };
    const r = src.map(x => x + 1);
    return log.join(";") + "|" + r[0];
});
t("map-species-proxy", () => {
    const ops = [];
    const src = [1, 2];
    src.constructor = { [Symbol.species]: function (len) {
        return new Proxy(new Array(len), {
            defineProperty(tgt, k, d) { ops.push("def:" + String(k)); return Reflect.defineProperty(tgt, k, d); }
        });
    } };
    const r = src.map(x => x);
    return ops.join(",") + "|" + r.join(",");
});
t("map-holes", () => {
    const a = [1, , 3];
    const r = a.map(x => x * 2);
    return r.join(",") + "|" + (1 in r);
});
t("map-mutating-callback-shrink", () => {
    const a = [1, 2, 3, 4, 5];
    const r = a.map((x, i) => { if (i === 0) a.length = 2; return x * 10; });
    return r.length + "|" + r.join(",");
});
t("filter-mutating-callback-grow", () => {
    const a = [1, 2, 3];
    const r = a.filter((x, i) => { if (i === 0) a.push(99); return true; });
    return r.join(",");
});
t("map-callback-throws", () => {
    const a = [1, 2, 3];
    try { a.map((x, i) => { if (i === 1) throw new RangeError("cb boom"); return x; }); return "NO THROW"; }
    catch (e) { return "caught " + e.name; }
});
t("map-subclass", () => {
    class MyArr extends Array {}
    const a = MyArr.from([1, 2]);
    const r = a.map(x => x + 1);
    return (r instanceof MyArr) + "|" + r.join(",");
});
t("map-bigarray", () => {
    const a = new Array(10000).fill(0).map((_, i) => i);
    const r = a.map(x => x * 2);
    return r[0] + "|" + r[9999] + "|" + r.length;
});
t("filter-all-rejected", () => [1, 2, 3].filter(() => false).length);
t("map-preserves-minus0", () => { const r = [-0].map(x => x); return Object.is(r[0], -0); });
print("SUITE_END");