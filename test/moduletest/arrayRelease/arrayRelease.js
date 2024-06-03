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
 * @tc.name:array
 * @tc.desc:test Array
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

{
    try {
        let obj = {
            length: 65536,
        };
        let str = "*".repeat(65536);
        Array.prototype.fill.call(obj, str);
        let res = Array.prototype.join.call(obj);
        print(res.length);
    }
    catch (e) {
        print(e.name)
    }
}

{
    try {
        let arr = new Array(47237);
        arr.fill(0);
        arr.join("*".repeat(94473))
    } catch (e) {
        print(e.name);
    }
}

{
    try {
        let arr = new Uint8Array(47237);
        arr.join("*".repeat(94473))
    } catch (e) {
        print(e.name);
    }
}