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
 * @tc.name:releaseRegexp
 * @tc.desc:test releaseRegexp
 * @tc.type: FUNC
 */

{
    try {
        let pattern = "aaaaa".repeat(1000);
        let reg = new RegExp(pattern);
        let arr = new Array(100);
        for(let i=0; i < 1000; i++) {
            for(let j = 0; j < 100; j++) {
                arr[j] = Math.floor(Math.random() * 26) + 97;
            }
            let str = String.fromCharCode(...arr);
            let substr = str.substring(70, 100);
            reg.exec(substr);
        }   
    } catch (error) {
        assert_unreachable();
    }
}
test_end();