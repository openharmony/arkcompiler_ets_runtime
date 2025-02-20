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
 * @tc.name:nextinternalslowpath
 * @tc.desc:test JSForInIterator::NextInternalSlowpath
 * @tc.type: FUNC
 * @tc.require: issueIBJL5S
 */

// This case aims to test if JSForInIterator::NextInternalSlowpath could return exception.
{
    function test_func() {}
    function f0() {}
    let v12 = new Proxy(Object.create(null, {x: {enumerable: true}}), {
        getOwnPropertyDescriptor(v13, v14) {
            if (v13 != null && typeof v13 == "object") {
                Object.defineProperty(v13, test_func(), {get: function () {}});
            }
            return Reflect.getOwnPropertyDescriptor(v13, v14);
        }
    });
    try {
        for (let v15 in v12) {}
        f0([1]);
    } catch (e) {
        print(e);
    }
}
