/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 * @tc.name:releaseAddProperty
 * @tc.desc:test releaseAddProperty
 * @tc.type: FUNC
 */

let errorStr = "";
function F0() {
    if (!new.target) { throw 'must be called with new'; }
}
const v2 = new F0();
let v4 = 10000;
try {
    do {
        v2[v4] >>= 11;
        v4++;
    } while (0 < 8)
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, "RangeError: Invalid array length");


function TreeSet() { }
class C1 extends TreeSet {
    constructor() {
        super();
        for (let i7 = 0; i7 < 4; i7--) {
            super[i7] = i7;
        }
    }
}
try {
    new C1();
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, "RangeError: Invalid array length");
test_end();