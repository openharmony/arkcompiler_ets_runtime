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
 * @tc.name:allocatearraybuffer
 * @tc.desc:test ArrayBuffer
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
var newTarget = function() {}.bind(null);
var arrayBuffer = Reflect.construct(ArrayBuffer, [16], newTarget);
print(arrayBuffer.length);

/*
 * @tc.name:allocatearraybuffer
 * @tc.desc:test DataView
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
const dataview = new DataView(new ArrayBuffer(64));
dataview.setInt16(0,-1);
print(dataview.getFloat64("cas"));

const v32 = new Int32Array(10);
for (let i = 0; i < v32.length; i++) {
    v32[i] = i + 1;
}
const v36 = this.Atomics;
print(v36.and(v32, this, v36));

function SendableArrayBufferTest() {
    try {
        const o1 = {};
        const v3 = new Proxy(SendableArrayBuffer, o1);
        new v3();
    } catch (error) {
        print(error);
    }
}
SendableArrayBufferTest()