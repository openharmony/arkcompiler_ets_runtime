/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
 * @tc.name:arraytospliced
 * @tc.desc:test Array.toSpliced
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

{
    const arr0 = [1, , 3, 4, , 6, undefined];

    assert_equal(arr0.indexOf(undefined), 6);
    const arr1 = arr0.toSpliced(1, 2);
    assert_equal(arr0.indexOf(undefined), 6);
    assert_equal(arr1.indexOf(undefined), 2);

    const arr2 = new Array(1025);
    for (let i = 0; i < 1025; i = i + 1)
        arr2[i] = i;
    const arr3 = arr2.toSpliced(0, 0);
}

{
    var arr4 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
    var arr5 = new Array();
    for (let i = 0; i < 10; i++) arr5[i] = i;
    var result1 = arr4.toSpliced(0, 2, 6);
    assert_equal(result1, [6, 2, 3, 4, 5, 6, 7, 8, 9]);
    var result2 = arr5.toSpliced(0, 2, 6);
    assert_equal(result2, [6, 2, 3, 4, 5, 6, 7, 8, 9]);
}

{
    let outputs = []
    // 用例 1: 字符串、数字混合数组
    const mixedArray = [1, "two", 3, "four"];
    const newArray1 = mixedArray.toSpliced(1, 2, "new", 5);
    assert_equal(newArray1, [1, "new", 5, "four"]);

    // // 用例 2: 插入布尔值、对象和函数
    const complexArray = [true, { key: "value" }, () => "hello"];
    const newArray2 = complexArray.toSpliced(2, 0, false, { anotherKey: 42 }, () => "world");
    assert_equal(JSON.stringify(newArray2), JSON.stringify([true, { "key": "value" }, false, { "anotherKey": 42 }, null, null]));

    // 用例 3: 替换 undefined 和 null
    const arrayWithUndefinedNull = [undefined, null, 3, 4];
    const newArray3 = arrayWithUndefinedNull.toSpliced(0, 2, "replaced");
    assert_equal(newArray3, ["replaced", 3, 4]);

    // 用例 4: 处理稀疏数组
    const sparseArray = [1, , 3, , 5];
    const newArray4 = sparseArray.toSpliced(1, 1, "new");
    assert_equal(newArray4, [1, "new", 3, , 5]);
    if (newArray4[3] !== undefined) {
        print("fail: hole should be replaced by undefined");
    }

    // 用例 5: 在空数组中添加元素
    const emptyArray = [];
    const newArray5 = emptyArray.toSpliced(0, 0, "first", 2, "third");
    assert_equal(newArray5, ["first", 2, "third"]);

    // 用例 6: 带有符号类型和 Symbol 类型的数组
    const arrayWithSpecialTypes = [Symbol("id"), BigInt(123), "hello"];
    const newArray6 = arrayWithSpecialTypes.toSpliced(1, 1, "new value");
    assert_equal(JSON.stringify(newArray6), JSON.stringify([null, "new value", "hello"]));

    // 用例 7: 从数组中移除所有元素
    const fullArray = [1, 2, 3, 4];
    const emptyArray2 = fullArray.toSpliced(0, fullArray.length);
    assert_equal(emptyArray2, []);
}

{
    const arr = new Array(1, 2, 3, 4, 5);
    Object.defineProperty(arr, 1, {
        get() {
            arr.length = 0;
            arr[0] = -2147483634;
        }
    });
    let result = arr.toSpliced(arr, 10, 100, 101);
    assert_equal(JSON.stringify(result), JSON.stringify([100, 101]));
}

{
    const arr = new Array(1, 2, 3, 4, 5);
    Object.defineProperty(arr, 1, {
        get() {
            arr.length = 0;
            arr[0] = -2147483634;
        }
    });
    arr.toString = function () {
        arr.length = 0;
        arr[0] = -2147483634;
        return "2";
    };
    let result = arr.toSpliced(arr, 0, 100, 101);
    assert_equal(JSON.stringify(result), JSON.stringify([-2147483634, null, 100, 101, null, null, null]));
}

{
    const arr = new Array(1, 2, 3, 4, 5);
    Object.defineProperty(arr, 1, {
        get() {
            arr.length = 0;
            arr[0] = -2147483634;
        }
    });
    let result = arr.toSpliced(arr, 0, 100, 101);
    assert_equal(JSON.stringify(result), JSON.stringify([100, 101, -2147483634, null, null, null, null]));
}

test_end();