/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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
 * @tc.name:weakmapsymbolkey
 * @tc.desc:test WeakMap allow the use of most Symbols as keys
 * @tc.type: FUNC
 * @tc.require: issueI7J2VN
 */
print("weakmapsymbolkey test start");
let wm = new WeakMap();
let o = {};
wm.set(o, 0);
let s1 = Symbol("symbol1");
wm.set(s1, 1);

let s2 = Symbol.for("symbol2");
try {
    wm.set(s2, 2);
} catch (err) {
    print(err.name);
}

print("wm.has(s1) " + wm.has(s1));
print("wm.get(s1) " + wm.get(s1));
print("wm.delete(s1) " + wm.delete(s1));
print("wm.has(s1) " + wm.has(s1));

print("wm.has(s2) " + wm.has(s2));
print("wm.get(s2) " + wm.get(s2));
print("wm.delete(s2) " + wm.delete(s2));

const symbolFuncsSet = [
    Symbol.asyncIterator,
    Symbol.hasInstance,
    Symbol.isConcatSpreadable,
    Symbol.iterator,
    Symbol.match,
    Symbol.matchAll,
    Symbol.replace,
    Symbol.search,
    Symbol.species,
    Symbol.split,
    Symbol.toPrimitive,
    Symbol.toStringTag,
    Symbol.unscopables
];

symbolFuncsSet.forEach(function (ctor, i) {
    wm.set(ctor, i);
});

print("wm.get(Symbol.match) == 4 " + (wm.get(Symbol.match) == 4));
print("weakmapsymbolkey test end");