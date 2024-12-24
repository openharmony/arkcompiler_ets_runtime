/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
 * @tc.name:stackoverflow
 * @tc.desc:test stack overflow
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
try {
    var array = new Uint8Array(1000000);
    var res = String.fromCharCode.apply(null, array);
} catch (e) {
    assert_equal(error instanceof RangeError, true);
}

try {
    var array = new Uint8Array(15000);
    var res = String.fromCharCode.apply(null, array);
    assert_equal(res.length,15000);
} catch (e) {
    if ((e instanceof RangeError)) {
        assert_unreachable();
    }
}

function runNearStackLimit(f) {
    function t() {
        try {
            t();
        } catch (e) {
            f();
        }
    };
    try {
        t();
    } catch (e) {
    }
}
const v3 = new Proxy(Boolean, []);
const v4 = v3.bind();
function f5(a6, a7) {
    4294967296n + v4;
    return runNearStackLimit(f5);
}
f5();

test_end();