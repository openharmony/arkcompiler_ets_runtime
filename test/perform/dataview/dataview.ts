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

declare function print(arg:any) : string;
declare interface ArkTools {
    timeInUs(arg:any):number
}

function testGetVievValue() {
    var buffer = new ArrayBuffer(8);
    var dataview = new DataView(buffer);
    let start = ArkTools.timeInUs();
    let res;
    for (let i = 0; i < 1_000_000; i++) {
        res = dataview.getInt32(2);
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print(res)
    print("DataView GetVievValue:\t" + String(time) + "\tms");
}

function testSetVievValue() {
    var buffer = new ArrayBuffer(8);
    var dataview = new DataView(buffer);
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        dataview.setInt32(1, 2);
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("DataView SetVievValue:\t" + String(time) + "\tms");
}

function testGetByteLength() {
    var buffer = new ArrayBuffer(8);
    var dataview = new DataView(buffer);
    let start = ArkTools.timeInUs();
    let res;
    for (let i = 0; i < 1_000_000; i++) {
        res = dataview.byteLength;
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print(res)
    print("DataView ByteLength:\t" + String(time) + "\tms");
}

testSetVievValue();
testGetVievValue();
testGetByteLength();

