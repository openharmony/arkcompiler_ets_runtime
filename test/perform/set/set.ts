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

function testAdd() {
    let key = "key";
    let mySet = new Set();
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        mySet.add(key);
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("Set Add:\t" + String(time) + "\tms");
}

function testDelete() {
    const mySet = new Set();
    let key = "key";
    mySet.add(key);
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        mySet.delete(key);
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("Set Delete:\t" + String(time) + "\tms");
}

function testHas() {
    const mySet = new Set();
    let key = "key";
    mySet.add(key);
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        mySet.has(key);
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("Set Has:\t" + String(time) + "\tms");
}

function setElements(value1, value2, set) {}
function testForEach() {
    const mySet = new Set();
    let key = "key";
    mySet.add(key);
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        mySet.forEach(setElements);
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("Set ForEach:\t" + String(time) + "\tms");
}

function testClear() {
    const mySet = new Set();
    let key = "key";
    mySet.add(key);
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        mySet.clear();
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("Set Clear:\t" + String(time) + "\tms");
}

function testValues() {
    const mySet = new Set();
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        mySet.values();
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("Set Values:\t" + String(time) + "\tms");
}

function testEntries() {
    const mySet = new Set();
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        mySet.entries();
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("Set Entries:\t" + String(time) + "\tms");
}

testAdd();
testDelete();
testHas();
testForEach();
testClear();
testValues();
testEntries();
