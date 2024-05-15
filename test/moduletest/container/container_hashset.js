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
 * @tc.name:container
 * @tc.desc:test container
 * @tc.type: FUNC
 * @tc.require: 
 */
var fastset = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    fastset = ArkPrivate.Load(ArkPrivate.HashSet);

    let map = new Map();
    let set = new fastset();
    set.add("aa");
    set.add("bb");

    // test has: true
    map.set("test has:", set.length == 2 && set.has("aa") && set.has("bb") && !set.has("cc"));

    set.add("cc");

    // test values: true
    let iteratorSetValues = set.values();
    map.set("test values:", iteratorSetValues.next().value == "aa" && iteratorSetValues.next().value == "bb" &&
            iteratorSetValues.next().value == "cc" && iteratorSetValues.next().value == undefined);
    // test entries: [cc, cc], undefined
    let iteratorSetEntries = set.entries();
    iteratorSetEntries.next().value;
    iteratorSetEntries.next().value;
    map.set("test entries1:", iteratorSetEntries.next().value != undefined);
    map.set("test entries2:", iteratorSetEntries.next().value == undefined);

    // test forof: aa, bb, cc
    let arr = ["aa", "bb", "cc"];
    let i = 0;
    for (const item of set) {
        map.set(arr[i], item == arr[i]);
        i++;
    }

    // test forin:
    for (const item in set) {
        map.set("test forin:", item);
    }

    // test forEach:
    set.forEach((i, d) => {
    });
    map.set("test forEach:", true);
    // test isEmpty: false
    map.set("test isEmpty:", !set.isEmpty());

    set.add("ee");
    set.add("dd");
    // test remove: true
    map.set("test remove:", set.remove("bb"));
    // test clear: true
    set.clear();
    map.set("test clear:", set.length == 0 && !set.has("cc") && set.isEmpty());

    let flag = false;
    try {
        set["aa"] = 3;
    } catch (e) {
        flag = true;
    }
    map.set("test set throw error", flag);

    let set1 = new fastset();
    let proxy = new Proxy(set1, {});
    proxy.add("aa");
    proxy.add("bb");

    // test has: true
    map.set("test has:", proxy.length == 2 && proxy.has("aa") && proxy.has("bb") && !proxy.has("cc"));

    proxy.add("cc");

    // test values: true
    let iteratorSetValues1 = proxy.values();
    map.set("test values:", iteratorSetValues1.next().value == "aa" && iteratorSetValues1.next().value == "bb" &&
            iteratorSetValues1.next().value == "cc" && iteratorSetValues1.next().value == undefined);
    // test entries: [cc, cc], undefined
    let iteratorSetEntries1 = proxy.entries();
    iteratorSetEntries1.next().value;
    iteratorSetEntries1.next().value;
    map.set("test entries1:", iteratorSetEntries1.next().value != undefined);
    map.set("test entries2:", iteratorSetEntries1.next().value == undefined);

    // test forof: aa, bb, cc
    let arr1 = ["aa", "bb", "cc"];
    let j = 0;
    for (const item of proxy) {
        map.set(arr1[j], item == arr1[j]);
        j++;
    }

    // test forin:
    for (const item in proxy) {
        map.set("test forin:", item);
    }

    // test forEach:
    proxy.forEach((i, d) => {
    })
    map.set("test forEach:", true);

    // test isEmpty: false
    map.set("test isEmpty:", !proxy.isEmpty());

    proxy.add("ee");
    proxy.add("dd");
    // test remove: true
    map.set("test remove:", proxy.remove("bb"));
    // test clear: true
    proxy.clear();
    map.set("test clear:", proxy.length == 0 && !proxy.has("cc") && proxy.isEmpty());

    flag = false;
    let seten = new fastset();
    seten.add(1);
    seten.add(2);
    seten.add(3);
    seten.add(4);
    seten.add(5);
    let iter = seten.entries();
    let temp = iter.next();
    while(!temp.done) {
        if ((temp.value[0]) == (temp.value[1])) {
            flag = true;
        }
        temp = iter.next();
    }
    map.set("test entries return type", flag);
    flag = false;
    try {
        proxy["aa"] = 3;
    } catch (e) {
        flag = true;
    }
    map.set("test set throw error", flag);

    flag = undefined;
    function elementsHashSet(valueHash, keyHash, map) {
        if (!valueHash) {
            if (!flag) {
                flag = [];
            }
            flag.push(keyHash);
        }
    }
    map.forEach(elementsHashSet);
    let de = new fastset();
    try {
        de.forEach(123);
    } catch(err) {
        if (err.name != "BusinessError") {
            print("HashSet forEach throw error fail");
        }
    }
    if (!flag) {
        print("Test HashSet success!!!");
    } else {
        print("Test HashSet fail: " + flag);
    }
}
export let hashsetRes = "Test hashset done";