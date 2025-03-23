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

declare function print(arg: any): string;

declare interface ArkTools {
    timeInUs(arg: any): number;
}

const REP_COUNT = 1_000_000;

// Tests optimization of JSArray::CreateArrayFromList() in js_array.cpp
function testOwnKeys() {
    const obj = {};
    const arr = [];

    let start: number;
    let end: number;

    start = ArkTools.timeInUs();
    for (let i = 0; i < REP_COUNT; i++) {
        Reflect.ownKeys(obj); // Triggers JSArray::CreateArrayFromList()
    }
    end = ArkTools.timeInUs();
    print(`Reflect.ownKeys(obj):\t${((end - start) / 1000).toFixed(3)}\tms`);

    start = ArkTools.timeInUs();
    for (let i = 0; i < REP_COUNT; i++) {
        Reflect.ownKeys(arr); // Triggers JSArray::CreateArrayFromList()
    }
    end = ArkTools.timeInUs();
    print(`Reflect.ownKeys(arr):\t${((end - start) / 1000).toFixed(3)}\tms`);
}

testOwnKeys();
