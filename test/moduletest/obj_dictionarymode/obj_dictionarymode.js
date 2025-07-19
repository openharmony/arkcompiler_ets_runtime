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


{
    let obj = {};
    for (let i = 0; i < 100; i++) {
        obj[i] = 0;
    }
    assert_equal(ArkTools.hasFastProperties(obj), true);
    delete obj[0];
    assert_equal(ArkTools.hasFastProperties(obj), false);
}

{
    let arr = [];
    for (let i = 0; i < 100; i++) {
        arr[i] = 0;
    }
    assert_equal(ArkTools.hasFastProperties(arr), true);
    delete arr[0];
    assert_equal(ArkTools.hasFastProperties(arr), false);
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
    assert_equal(ArkTools.hasFastProperties(obj5), false);
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
    assert_equal(ArkTools.hasFastProperties(arr3), false);
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
    assert_equal(ArkTools.hasFastProperties(arr5), true);
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
    assert_equal(ArkTools.hasFastProperties(arrayLike), false);
}
test_end();