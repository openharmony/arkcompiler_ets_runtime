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
 * @tc.name:fortest
 * @tc.desc:test for
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
let a = '*'
let s = ''

for (let i = 0; i < 10; i++) {
    s += a
    print(s)
}

let i = 10

while (i > 0) {
    print(i);
    i--;
}

function F0() {
}
const v5 = new F0();
for (let index = 0; index < 32; index++) {
    function F17() {
        this.__proto__ = v5;
    }
    const v28 = new F17();
    const v29 = "p" + index;
    v5[v29] = index;
}
for (let k in v5) {
    print(k, v5[k]);
}

function f0(v1, v2) {
    try {
        v1();
    } catch (v5) {
        print(v5)
    }
}

try {
    var v0 = function () { }.bind();
} catch (e) {
    print(e)
}

try {
    Object.defineProperty(v0, "prototype", {
        get() {
            throw Error("OK");
        }
    });
} catch (e) {
    print(e)
}

try {
[Array, Date, Boolean, Number, String, RegExp, Error, Uint8Array, ArrayBuffer, Map, WeakMap, Set, WeakSet].
  forEach(function (v6) {
    f0(() => {
      Reflect.construct(v6, [], v0);
    }, `Error: OK`);
  });
} catch (e) {
    print(e)
}
