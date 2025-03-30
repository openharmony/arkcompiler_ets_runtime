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
 * @tc.name:weaksetsymbolvalue
 * @tc.desc:test WeakSet allow the use of most Symbols as values
 * @tc.type: FUNC
 * @tc.require: issueI7J2VN
 */
let ws = new WeakSet();
let sym1 = Symbol("symbol1");
ws.add(sym1);

let sym2 = Symbol.for("symbol2");
try {
    ws.add(sym2);
} catch (err) {
    assert_equal(err instanceof TypeError, true);
}

assert_equal(ws.has(sym1),true);
assert_equal(ws.delete(sym1),true);
assert_equal(ws.has(sym1),false);

assert_equal(ws.has(sym2),false);
assert_equal(ws.delete(sym2),false);

const symbolFuncsValue = [
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

symbolFuncsValue.forEach(function (ctor, i) {
    ws.add(ctor, i);
});

assert_equal(ws.has(Symbol.match),true);

test_end();