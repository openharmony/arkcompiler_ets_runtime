/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
 * @tc.name:mapget
 * @tc.desc: test Map.get
 * @tc.type: FUNC
 * @tc.require: issueI97M5M
 */
let map = new Map();

map.set('key', 'value');
print(map.get('key'))

for (let i = 0; i < 3; ++i) {
    map.set(i, -i);
}

for (let i = 0; i < 4; ++i) {
    let value = map.get(i);
    print(value);
}

map = new Map();
let key = Number.parseFloat("1392210229");
map.set(key, "success");
let value = map.get(key);
print(value);


function check(key) {
    let irHash = ArkTools.hashCode(key);
    let rtHash = ArkTools.hashCode(key, true);
    if (irHash != rtHash) {
        throw new Error("Mismatch hash for " + key + ": expected " + rtHash + ", but got " + irHash);
    }
}

check(0);
check(1);
check(-1);
check(1.5);
check(-1.5);
check(Number.EPSILON);
check(Number.NaN);
check(Number.MIN_VALUE);
check(Number.MAX_VALUE);
check(Number.MIN_SAFE_INTEGER);
check(Number.MIN_SAFE_INTEGER - 1);
check(Number.MAX_SAFE_INTEGER);
check(Number.MAX_SAFE_INTEGER + 1);
check(Number.NaN);
check(Number.POSITIVE_INFINITY);
check(Number.NEGATIVE_INFINITY);
check(Number.parseFloat("+0.0"));
check(Number.parseFloat("-0.0"));

// regression test
check(Number.parseFloat("1392210229"));
