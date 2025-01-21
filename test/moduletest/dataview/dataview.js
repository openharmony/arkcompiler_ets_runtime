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
 * @tc.name:dataview
 * @tc.desc:test dataview
 * @tc.type: FUNC
 * @tc.require: issue#I7NUZM
 */
const buffer = new ArrayBuffer(16);
const view = new DataView(buffer);
view.setInt32({}, 0x1337, {});
print(view.getInt32({}, {}));

try {
    var buffer1 = new ArrayBuffer(64);
    var dataview = new DataView(buffer1, 8, 24);
    dataview.setInt32(0, 1n);
} catch(e) {
    print(e)
}

let ab = new ArrayBuffer(0x100);
try {
    let dv1 = new DataView(ab, 0x10, 0xfffffff8);
} catch(e) {
    print(e)
}
try {
    let dv2 = new DataView(ab, -1, 0xfffffff8);
} catch(e) {
    print(e)
}
try {
    let dv3 = new DataView(ab, 2**53, 0xfffffff8);
} catch(e) {
    print(e)
}
try {
    let dv4 = new DataView(ab, 0x10, -1);
} catch(e) {
    print(e)
}
try {
    let dv5 = new DataView(ab, 0x10, 2**53);
} catch(e) {
    print(e)
}

try {
    var dv6 = new DataView(ab, 64, 14);
    dv6.setFloat64(0x7fffffff, +254, true);
} catch(e) {
    print(e)
}