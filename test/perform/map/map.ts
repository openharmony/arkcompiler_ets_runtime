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

function testSet() {
    let key = "key";
    let myMap = new Map();
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        myMap.set(key, i);
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("Map Set:\t" + String(time) + "\tms");
}

function testClear() {
    let myMap = new Map();
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        myMap.clear();
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("Map Clear:\t" + String(time) + "\tms");
}

function testDelete() {
    let myMap = new Map();
    let key = "key";
    myMap.set(key, key);
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        myMap.delete(key);
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("Map Delete:\t" + String(time) + "\tms");
}

function testHas() {
    let myMap = new Map();
    let key = "key";
    myMap.set(key, key);
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        myMap.has(key);
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("Map Has:\t" + String(time) + "\tms");
}

function setElements(value, key, map) {}
function testForEach() {
    let myMap = new Map();
    let key = "key";
    myMap.set(key, key);
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        myMap.forEach(setElements);
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("Map ForEach:\t" + String(time) + "\tms");
}

function testKeys() {
    let myMap = new Map();
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        myMap.keys();
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("Map Keys:\t" + String(time) + "\tms");
}

function testValues() {
    let myMap = new Map();
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        myMap.values();
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("Map Values:\t" + String(time) + "\tms");
}

function testEntries() {
    let myMap = new Map();
    let start = ArkTools.timeInUs();
    for (let i = 0; i < 1_000_000; i++) {
        myMap.entries();
    }
    let end = ArkTools.timeInUs();
    let time = (end - start) / 1000
    print("Map Entries:\t" + String(time) + "\tms");
}

testSet();
testClear();
testDelete();
testHas();
testForEach();
testKeys();
testValues();
testEntries();
