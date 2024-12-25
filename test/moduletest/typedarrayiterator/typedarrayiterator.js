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
 * @tc.name:typedarrayiterator
 * @tc.desc:test TypedArray.iterator
 * @tc.type: FUNC
 */

let v2 = new Uint8Array([1, 2, 3]);
let v3 = v2[Symbol.iterator]();
let nextBak = v3.__proto__["next"];
v3.__proto__["next"] = null;
try {
    Uint8Array.from(v2);
	assert_unreachable();
} catch (e) {
    assert_equal(e instanceof TypeError,true);
}
v3.__proto__["next"] = nextBak;

test_end();
