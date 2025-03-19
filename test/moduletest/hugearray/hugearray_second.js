/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * @tc.name:hugearray
 * @tc.desc:test Array that is a huge object
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
let v2 = new Array(2);
let t = 0
do {
    try {
        v2["lastIndexOf"](...16);
    } catch (e) {}
} while (t++ < 18000)
assert_equal(t, 18001);

test_end();
