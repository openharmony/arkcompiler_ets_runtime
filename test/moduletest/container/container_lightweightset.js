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
var fastset = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    fastset = ArkPrivate.Load(ArkPrivate.LightWeightSet);

    let res = new Map();
    let set = new fastset();
    let proxy = new Proxy(set, {});
    // test isEmpty: true
    res.set("test isEmpty:", proxy.isEmpty());
    proxy.add(1);
    proxy.add(2);
    proxy.add(3);
    // test has: true
    res.set("test has 1:", proxy.has(1));
    res.set("test has 2:", proxy.has(2));
    res.set("test has 3:", proxy.has(3));
    // test has: false
    res.set("test has 4:", proxy.has(4) == false);
    // test values: true
    let iteratorValues1 = proxy.values();
    res.set("test values:", iteratorValues1.next().value == 1 && iteratorValues1.next().value == 2 &&
            iteratorValues1.next().value == 3 && iteratorValues1.next().value == undefined);
    // test entries: [c,cc], undefined
    let iteratorEntries1 = proxy.entries();
    iteratorEntries1.next().value;
    iteratorEntries1.next().value;
    res.set("test entries1:", iteratorEntries1.next().value != undefined);
    res.set("itest entries2:", iteratorEntries1.next().value == undefined);

    // test forof
    let arr1 = [1, 2, 3];
    let j = 0;
    for (const item of proxy) {
    }
    // test forin:
    for (const item in proxy) {
        res.set("test forin", true);
    }
    // test forEach:
    let flag = false;
    function TestForEach1(value, key, proxy) {
        flag = proxy.has(key) && proxy.has(value);
        res.set("test forEach" + key, flag);
    }
    proxy.forEach(TestForEach1);

    let dset = new fastset();
    let dProxy = new Proxy(dset, {});
    dProxy.add(4);
    dProxy.add(5);
    dProxy.add(6);
    dProxy.add(7);
    dProxy.add(8);
    dProxy.add(9);
    res.set("test addAll:", dProxy.addAll(proxy));
    res.set("test hasAll:", dProxy.hasAll(proxy));
    let obj = ["a", "b"]
    res.set("test equal:", !dProxy.equal(obj));
    // test remove: true
    res.set("test remove:", dProxy.remove(1) === 1 && dProxy.length === 8);
    // test removeAt: true
    res.set("test removeAt:", dProxy.removeAt(3) && dProxy.length === 7);
    // test setValueAt: true
    res.set("test getValueAt:", dProxy.getValueAt(3) === 6);
    // test setValueAt: true
    res.set("test getIndexOf:", dProxy.getIndexOf(2) === 0);
    // test toString: true
    res.set("test toString:", dProxy.toString() === "2,3,4,6,7,8,9");
    let arr = dProxy.toArray()
    res.set("test toArray:", true);
    // test increaseCapacityTo: true
    dProxy.increaseCapacityTo(20)
    res.set("test increaseCapacityTo:", true);
    // test clear: 0
    let ret = dProxy.clear();
    res.set("test clear:", dProxy.length == 0);
    res.set("test 'clear' ret:", ret === undefined)

    // test COW
    let LOOP_COUNT = 5;
    let myTest = new fastset();

    for (var i = 0; i < LOOP_COUNT; i++) {
        myTest.add(i);
    }

    let a = myTest.toArray();
    res.set("test COW - create a :", a.length == LOOP_COUNT);
    myTest.add(10);
    res.set("test COW - check a.length :", a.length == LOOP_COUNT);
    let b = myTest.toArray();
    res.set("test COW - check b.length :", b.length == (LOOP_COUNT + 1));

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
    res.set("test entries return type", flag);

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

    let de = new fastset();
    try {
        de.forEach(123);
    } catch(err) {
        if (err.name != "BusinessError") {
            print("LightWeightSet forEach throw error fail");
        }
    }

    // Math.floor be index input should not throw exception.
    let myLs = new fastset();
    myLs.add("a");
    myLs.add("b");
    myLs.getValueAt(Math.floor(1.5));
    myLs.removeAt(Math.floor(1.5));

    if (!flag) {
        print("Test LightWeightSet success!!!");
    } else {
        print("Test LightWeightSet fail: " + flag);
    }
}
export let lightweightsetRes = "Test LightWeightSet done";
