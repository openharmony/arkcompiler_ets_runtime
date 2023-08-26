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
 * @tc.name:Flatten
 * @tc.desc:test Flatten
 * @tc.type: FUNC
 * @tc.require: issueI7CTF7
 */
const v14 = new Uint8ClampedArray(521);
let v16 = v14[1973679951];
v16 ||= v14;
const v17 = new Int32Array();
const v18 = v14.join(v16);
try {
    Int32Array();
} catch(e21) {
    e21.message = v18;
    print(e21);
    v17.set(e21, e21);
}