/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 * @tc.name:obj_dictionarymode
 * @tc.desc:test dictionary elements
 * @tc.type: FUNC
 * @tc.require: issueICNDYR
 */

function assertDictionaryElements(obj, expected) {
    assert_equal(ArkTools.hasDictionaryElements(obj), expected);
}

// Deletion throttle is length/16 (min 1). Reinsert an element before each delete
// so every iteration reaches DeleteElementInHolder and advances the counter.
function triggerElementsNormalizeScan(arr, index) {
    const threshold = Math.max(1, Math.floor(arr.length / 16));
    for (let i = 0; i <= threshold; i++) {
        arr[index] = i;
        delete arr[index];
    }
}

{
    let obj = {};
    for (let i = 0; i < 100; i++) {
        obj[i] = 0;
    }
    assert_equal(ArkTools.hasFastProperties(obj), true);
    delete obj[0];
    assert_equal(ArkTools.hasFastProperties(obj), true);
}

{
    let arr = [];
    for (let i = 0; i < 100; i++) {
        arr[i] = 0;
    }
    assert_equal(ArkTools.hasFastProperties(arr), true);
    delete arr[0];
    assert_equal(ArkTools.hasFastProperties(arr), true);
}

{
    let arr = new Array(2048);
    assert_equal(ArkTools.hasFastProperties(arr), false);
    for(let i=0;i<arr.length;i++) {
        arr[i] = 0;
    }
    assert_equal(ArkTools.hasFastProperties(arr), true);
}

{
    let obj1 = {};
    assert_equal(ArkTools.hasFastProperties(obj1), true);
}

{
    let obj2 = {};
    for (let i = 0; i < 1000; i++) {
        obj2['prop' + i] = i;
    }
    assert_equal(ArkTools.hasFastProperties(obj2), true);
}

{
    let obj3 = { a: 1, b: 2, c: 3 };
    delete obj3.b;
    assert_equal(ArkTools.hasFastProperties(obj3), false);
}

{
    let obj4 = {};
    obj4['a'] = 1;
    obj4['b'] = 2;
    delete obj4['a'];
    assert_equal(ArkTools.hasFastProperties(obj4), false);
}

{
    let obj5 = {};
    for (let i = 0; i < 100; i++) {
        obj5[i] = i;
        obj5['prop' + i] = 'value' + i;
    }
    assert_equal(ArkTools.hasFastProperties(obj5), true);
    delete obj5[50];
    assert_equal(ArkTools.hasFastProperties(obj5), true);
}

{
    let arr1 = [];
    assert_equal(ArkTools.hasFastProperties(arr1), true);
}

{
    let arr2 = new Array(100).fill(0);
    assert_equal(ArkTools.hasFastProperties(arr2), true);
}

{
    let arr3 = new Array(100).fill(0);
    delete arr3[50];
    assert_equal(ArkTools.hasFastProperties(arr3), true);
}

{
    let arr4 = [];
    arr4[0] = 0;
    arr4[1000] = 1000;
    assert_equal(ArkTools.hasFastProperties(arr4), true);
}

{
    let arr5 = new Array(100).fill(0);
    arr5.prop = 'value';
    assert_equal(ArkTools.hasFastProperties(arr5), true);
    delete arr5.prop;
    assert_equal(ArkTools.hasFastProperties(arr5), false);
    delete arr5[50];
    assert_equal(ArkTools.hasFastProperties(arr5), false);
    arr5.fill(0);
    assert_equal(ArkTools.hasFastProperties(arr5), false);
}

{
    let arr6 = new Array(100).fill(0);
    arr6.length = 1000;
    assert_equal(ArkTools.hasFastProperties(arr6), true);
    delete arr6[500];
    assert_equal(ArkTools.hasFastProperties(arr6), true);
}

{
    let arr7 = new Array(1024);
    assert_equal(ArkTools.hasFastProperties(arr7), true);
}

{
    let arr8 = new Array(2048);
    assert_equal(ArkTools.hasFastProperties(arr8), false);
}

{
    let objProto = { a: 1 };
    let objEdge1 = Object.create(objProto);
    assert_equal(ArkTools.hasFastProperties(objEdge1), true);
    delete objProto.a;
    assert_equal(ArkTools.hasFastProperties(objEdge1), true);
}

{
    let objEdge2 = {};
    Object.defineProperty(objEdge2, 'accessor', {
        get() { return this.value; },
        set(v) { this.value = v; }
    });
    assert_equal(ArkTools.hasFastProperties(objEdge2), true);
    delete objEdge2.value;
    assert_equal(ArkTools.hasFastProperties(objEdge2), true);
}

{
    let arrayLike = { 0: 'a', 1: 'b', 2: 'c', length: 3 };
    assert_equal(ArkTools.hasFastProperties(arrayLike), true);
    delete arrayLike[1];
    assert_equal(ArkTools.hasFastProperties(arrayLike), true);
}

{
    let arr = new Array(32).fill(0);
    arr.named = 1;
    delete arr[0];
    assert_equal(arr[0], undefined);
    assert_equal(arr[1], 0);
    assert_equal(arr.named, 1);
    assert_equal(ArkTools.hasFastProperties(arr), true);
    assertDictionaryElements(arr, false);
}

{
    let arr = new Array(128);
    for (let i = 124; i < 128; i++) {
        arr[i] = i;
    }
    arr.named = 'keep-fast-properties';
    assertDictionaryElements(arr, false);
    // Four live elements remain. At most threshold+1 valid deletes are needed to
    // force a scan, independent of the thread-local counter's initial value.
    triggerElementsNormalizeScan(arr, 0);
    assert_equal(arr.named, 'keep-fast-properties');
    assert_equal(arr[127], 127);
    assert_equal(arr[0], undefined);
    assert_equal(ArkTools.hasFastProperties(arr), true);
    assertDictionaryElements(arr, true);

    arr.addedAfterNormalize = 2;
    assert_equal(arr.addedAfterNormalize, 2);
    assert_equal(ArkTools.hasFastProperties(arr), true);
    assertDictionaryElements(arr, true);

    delete arr.named;
    assert_equal(arr.named, undefined);
    assert_equal(ArkTools.hasFastProperties(arr), false);
    assertDictionaryElements(arr, true);
    assert_equal(arr[127], 127);
}

{
    let arr = new Array(128).fill(0);
    arr.named = 2;
    for (let i = 0; i < 9; i++) {
        delete arr[i];
    }
    assert_equal(arr[9], 0);
    assert_equal(arr.named, 2);
    assert_equal(ArkTools.hasFastProperties(arr), true);
    assertDictionaryElements(arr, false);
}

{
    let obj = {};
    for (let i = 0; i < 10; i++) {
        obj[i] = i;
    }
    obj.named = 'x';
    delete obj[9];
    assert_equal(obj[9], undefined);
    assert_equal(obj[8], 8);
    assert_equal(obj.named, 'x');
    assert_equal(ArkTools.hasFastProperties(obj), true);
}

{
    let obj = {};
    for (let i = 0; i < 5; i++) {
        obj[i] = i;
    }
    for (let i = 4; i >= 0; i--) {
        delete obj[i];
    }
    assert_equal(obj[0], undefined);
    assert_equal(ArkTools.hasFastProperties(obj), true);
    obj[0] = 7;
    assert_equal(obj[0], 7);
}

{
    let obj = {};
    for (let i = 0; i < 20; i++) {
        obj[i] = i;
    }
    obj.prop = 'x';
    delete obj[10];
    assert_equal(obj[10], undefined);
    assert_equal(obj[11], 11);
    assert_equal(obj.prop, 'x');
    assert_equal(ArkTools.hasFastProperties(obj), true);
}

{
    let arr = new Array(128);
    for (let i = 124; i < 128; i++) {
        arr[i] = i;
    }
    assertDictionaryElements(arr, false);
    delete arr[127];
    triggerElementsNormalizeScan(arr, 0);
    assert_equal(arr[127], undefined);
    assert_equal(arr[126], 126);
    assert_equal(ArkTools.hasFastProperties(arr), true);
    assertDictionaryElements(arr, true);
}

test_end();
