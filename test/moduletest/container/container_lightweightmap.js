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
import {testCommon}  from "./utility";

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
    res.set("test getKeyAt-1:", proxy.getKeyAt(1) == "b");
    res.set("test getKeyAt-2:", proxy.getKeyAt(Math.floor(1.3)) == "b");
    testCommon(proxy, res);

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
    res.set("test removeAt-1:", dProxy.removeAt(dProxy.getIndexOfKey("b")) && dProxy.length == 3);
    // test setValueAt: true
    res.set("test setValueAt-1:", dProxy.setValueAt(dProxy.getIndexOfKey("d"), "ee"));
    // test getValueAt: true
    res.set("test getValueAt:", dProxy.getValueAt(dProxy.getIndexOfKey("d")) === "ee");
    res.set("test getValueAt:", dProxy.getValueAt(Math.floor(0)) === "cc");
    res.set("test getValueAt:", dProxy.getValueAt(Math.floor(1)) === "ee");
    res.set("test setValueAt-2:", dProxy.setValueAt(Math.floor(1.3), "ff"));
    res.set("test getValueAt-2:", dProxy.getValueAt(Math.floor(1.3)) === "ff");
    // test toString: true
    res.set("test toString:", dProxy.toString() === "c:cc,d:ff,e:ee");
    res.set("test removeAt-2:", dProxy.removeAt(Math.floor(1.3)) && dProxy.length == 2);
    // test increaseCapacityTo: true
    dProxy.increaseCapacityTo(20)
    dProxy.increaseCapacityTo(Math.floor(1.3));
    res.set("test increaseCapacityTo:", true);
    // test clear: 0
    let ret = dProxy.clear();
    res.set("test clear:", dProxy.length == 0);
    res.set("test 'clear' ret:", ret === undefined);
    
    let empty_fm = new fastmap();
    try {
        empty_fm.getKeyAt(0);
    } catch(err) {
        res.set("test GetKeyAt exception when arraylist is empty:", err == "BusinessError: Container is empty")
    }
    try {
        empty_fm.getValueAt(0);
    } catch(err) {
        res.set("test GetValueAt exception when arraylist is empty:", err == "BusinessError: Container is empty")
    }
    try {
        empty_fm.setValueAt(0);
    } catch(err) {
        res.set("test SetValueAt exception when arraylist is empty:", err == "BusinessError: Container is empty")
    }

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

    // Math.floor as index input should not throw exception.
    let myLm = new fastmap();
    myLm.set("a", "aa");
    myLm.set("b", "bb");
    myLm.getKeyAt(Math.floor(1.5));
    myLm.getValueAt(Math.floor(1.5));
    myLm.setValueAt(Math.floor(1.5), "cc");
    myLm.removeAt(Math.floor(1.5));

    if (!flag) {
        print("Test LightWeightMap success!!!");
    } else {
        print("Test LightWeightMap fail: " + flag);
    }
}
export let lightweightmapRes = "Test LightWeightMap done";
