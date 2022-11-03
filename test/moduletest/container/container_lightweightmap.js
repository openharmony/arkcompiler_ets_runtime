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
 * @tc.require: issueI5NO8G
 */
var fastmap = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    fastmap = ArkPrivate.Load(ArkPrivate.LightWeightMap);

    let res = new Map();
    let map = new fastmap();
    let proxy = new Proxy(map, {});
    // test isEmpty: true
    res.set("test isEmpty:", proxy.isEmpty());
    proxy.set("a", "aa");
    proxy.set("b", "bb");

    // test get: true
    res.set("test get:", proxy.length == 2 && proxy.get("a") == "aa" && proxy.get("b") == "bb");
    // test hasKey and hasValue: true
    res.set("test hasKey and hasValue:", proxy.hasKey("a") && proxy.hasKey("b") && proxy.hasValue("aa") &&
            proxy.hasValue("bb") && !proxy.hasKey("c") && !proxy.hasValue("cc"));

    proxy.set("c", "cc");
    // test getIndexOfKey and getIndexOfValue: true
    res.set("test getIndexOfKey and getIndexOfValue:", proxy.getIndexOfKey("a") === 0 && proxy.getIndexOfValue("bb") === 1);
    // test getKeyAt: true
    res.set("test getKeyAt:", proxy.getKeyAt(1) == "b");

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
    // test forEach:
    let flag = false;
    function TestForEach(value, key, proxy) {
        flag = proxy.get(key) === value;
        res.set("test forEach" + key, flag)
    }
    proxy.forEach(TestForEach);

    let dmap = new fastmap();
    let dProxy = new Proxy(dmap, {});
    dProxy.set("a", "aa");
    dProxy.set("b", "bb");
    dProxy.set("c", "cc");
    dProxy.set("d", "dd");
    dProxy.set("e", "ee");
    // test setAll:
    dProxy.setAll(proxy);
    res.set("test setAll:", dProxy.length === 5);
    res.set("test hasAll:", dProxy.hasAll(proxy));
    // test remove: true
    res.set("test remove:", dProxy.remove("a") == "aa" && dProxy.length == 4);
    // test removeAt: true
    res.set("test removeAt:", dProxy.removeAt(dProxy.getIndexOfKey("b")) && dProxy.length == 3);
    // test setValueAt: true
    res.set("test setValueAt:", dProxy.setValueAt(dProxy.getIndexOfKey("d"), "ee"));
    // test setValueAt: true
    res.set("test getValueAt:", dProxy.getValueAt(dProxy.getIndexOfKey("d")) === "ee");
    // test toString: true
    res.set("test toString:", dProxy.toString() === "c:cc,d:ee,e:ee");
    // test increaseCapacityTo: true
    dProxy.increaseCapacityTo(20)
    res.set("test increaseCapacityTo:", true);
    // test clear: 0
    dProxy.clear();
    res.set("test clear:", dProxy.length == 0);

    flag = false;
    try {
        proxy["aa"] = 3;
    } catch (e) {
        flag = true;
    }
    res.set("test map throw error", flag);
    flag = undefined;
    function elements(value, key, res) {
        if (!value) {
            if (!flag) {
                flag = [];
            }
            flag.push(key);
        }
    }
    res.forEach(elements);

    let de = new fastmap();
    try {
        de.forEach(123);
    } catch(err) {
        if (err.name != "BusinessError") {
            print("LightWeightMap forEach throw error fail");
        }
    }
    if (!flag) {
        print("Test LightWeightMap success!!!");
    } else {
        print("Test LightWeightMap fail: " + flag);
    }
}
