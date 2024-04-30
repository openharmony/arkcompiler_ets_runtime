/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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
export default function testCommon(proxy, res) {
    // test keys: true
    let iteratorKey1 = proxy.keys();
    res.set("test keys:", iteratorKey1.next().value == "a" && iteratorKey1.next().value == "b" &&
            iteratorKey1.next().value == "c" && iteratorKey1.next().value == undefined);
    // test values: true
    let iteratorValues1 = proxy.values();
    res.set("test values:", iteratorValues1.next().value == "aa" && iteratorValues1.next().value == "bb" &&
            iteratorValues1.next().value == "cc" && iteratorValues1.next().value == undefined);
    // test entries: [c,cc], undefined
    let iteratorEntries1 = proxy.entries();
    iteratorEntries1.next().value;
    iteratorEntries1.next().value;
    res.set("test entries1:", iteratorEntries1.next().value != undefined);
    res.set("itest entries2:", iteratorEntries1.next().value == undefined);

    // test forof: [a, aa], [b, bb], [c, cc]
    let arr1 = ["aa", "bb", "cc"];
    let j = 0;
    for (const item of proxy) {
        res.set(arr1[j], item[1] == arr1[j]);
        j++;
    }
    // test forin:
    for (const item in proxy) {
        res.set("test forin", false);
    }
}
