/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
 * @tc.name:loadicarray
 * @tc.desc:test loadic on arrays
 * @tc.type: FUNC
 * @tc.require: issueI7UTOA
 */

// Test basic array element load IC
function testBasicArrayElementLoad() {
    const arr = [10, 20, 30, 40, 50];
    for (let i = 0; i < 100; i++) {
        let res0 = arr[0];
        let res1 = arr[1];
        let res2 = arr[2];
        let res3 = arr[3];
        let res4 = arr[4];
        assert_equal(res0, 10);
        assert_equal(res1, 20);
        assert_equal(res2, 30);
        assert_equal(res3, 40);
        assert_equal(res4, 50);
    }
}
testBasicArrayElementLoad();

// Test array element load with variable index
function testArrayElementLoadVariableIndex() {
    const arr = [1, 2, 3, 4, 5];
    for (let i = 0; i < 100; i++) {
        for (let j = 0; j < arr.length; j++) {
            let res = arr[j];
            assert_equal(res, j + 1);
        }
    }
}
testArrayElementLoadVariableIndex();

// Test array element load with negative index
function testArrayElementLoadNegativeIndex() {
    const arr = [1, 2, 3];
    for (let i = 0; i < 100; i++) {
        let res = arr[-1];
        assert_equal(res, undefined);
    }
}
testArrayElementLoadNegativeIndex();

// Test array element load with out of bounds index
function testArrayElementLoadOutOfBounds() {
    const arr = [1, 2, 3];
    for (let i = 0; i < 100; i++) {
        let res = arr[100];
        assert_equal(res, undefined);
    }
}
testArrayElementLoadOutOfBounds();

// Test array element load with string index
function testArrayElementLoadStringIndex() {
    const arr = ["a", "b", "c"];
    for (let i = 0; i < 100; i++) {
        let res1 = arr["0"];
        let res2 = arr["1"];
        let res3 = arr["2"];
        assert_equal(res1, "a");
        assert_equal(res2, "b");
        assert_equal(res3, "c");
    }
}
testArrayElementLoadStringIndex();

// Test array length property load IC
function testArrayLengthPropertyLoad() {
    const arr = [1, 2, 3, 4, 5];
    for (let i = 0; i < 100; i++) {
        let res = arr.length;
        assert_equal(res, 5);
    }
}
testArrayLengthPropertyLoad();

// Test array length property load with changing array
function testArrayLengthWithChangingArray() {
    const arr = [];
    for (let i = 0; i < 100; i++) {
        arr.push(i);
        let res = arr.length;
        assert_equal(res, i + 1);
    }
}
testArrayLengthWithChangingArray();

// Test array method load IC
function testArrayMethodLoad() {
    const arr = [1, 2, 3];
    for (let i = 0; i < 100; i++) {
        let res1 = typeof arr.push;
        let res2 = typeof arr.pop;
        let res3 = typeof arr.shift;
        let res4 = typeof arr.unshift;
        let res5 = typeof arr.slice;
        assert_equal(res1, "function");
        assert_equal(res2, "function");
        assert_equal(res3, "function");
        assert_equal(res4, "function");
        assert_equal(res5, "function");
    }
}
testArrayMethodLoad();

// Test array prototype method call IC
function testArrayPrototypeMethodCall() {
    const arr = [3, 1, 4, 1, 5];
    for (let i = 0; i < 100; i++) {
        let res1 = arr.indexOf(1);
        let res2 = arr.includes(4);
        assert_equal(res1, 1);
        assert_equal(res2, true);
    }
}
testArrayPrototypeMethodCall();

// Test array forEach IC
function testArrayForEach() {
    const arr = [1, 2, 3];
    let sum = 0;
    for (let i = 0; i < 100; i++) {
        sum = 0;
        arr.forEach(function(item) {
            sum += item;
        });
        assert_equal(sum, 6);
    }
}
testArrayForEach();

// Test array map IC
function testArrayMap() {
    const arr = [1, 2, 3];
    for (let i = 0; i < 100; i++) {
        let res = arr.map(function(item) {
            return item * 2;
        });
        assert_equal(res[0], 2);
        assert_equal(res[1], 4);
        assert_equal(res[2], 6);
    }
}
testArrayMap();

// Test array filter IC
function testArrayFilter() {
    const arr = [1, 2, 3, 4, 5];
    for (let i = 0; i < 100; i++) {
        let res = arr.filter(function(item) {
            return item > 2;
        });
        assert_equal(res.length, 3);
    }
}
testArrayFilter();

// Test array reduce IC
function testArrayReduce() {
    const arr = [1, 2, 3, 4, 5];
    for (let i = 0; i < 100; i++) {
        let res = arr.reduce(function(acc, item) {
            return acc + item;
        }, 0);
        assert_equal(res, 15);
    }
}
testArrayReduce();

// Test array join IC
function testArrayJoin() {
    const arr = ["a", "b", "c"];
    for (let i = 0; i < 100; i++) {
        let res = arr.join("-");
        assert_equal(res, "a-b-c");
    }
}
testArrayJoin();

// Test array slice IC
function testArraySlice() {
    const arr = [1, 2, 3, 4, 5];
    for (let i = 0; i < 100; i++) {
        let res = arr.slice(1, 3);
        assert_equal(res[0], 2);
        assert_equal(res[1], 3);
    }
}
testArraySlice();

// Test array splice IC
function testArraySplice() {
    const arr = [1, 2, 3, 4, 5];
    for (let i = 0; i < 100; i++) {
        let temp = arr.slice();
        let res = temp.splice(1, 2);
        assert_equal(res[0], 2);
        assert_equal(res[1], 3);
    }
}
testArraySplice();

// Test array with hole
function testArrayWithHole() {
    const arr = [1, , 3];
    for (let i = 0; i < 100; i++) {
        let res = arr[1];
        assert_equal(res, undefined);
    }
}
testArrayWithHole();

// Test sparse array element load
function testSparseArrayElementLoad() {
    const arr = [];
    arr[0] = 1;
    arr[1000] = 2;
    for (let i = 0; i < 100; i++) {
        let res1 = arr[0];
        let res2 = arr[1000];
        let res3 = arr[500];
        assert_equal(res1, 1);
        assert_equal(res2, 2);
        assert_equal(res3, undefined);
    }
}
testSparseArrayElementLoad();

// Test array with different element kinds
function testArrayDifferentElementKinds() {
    const arr1 = [1, 2, 3];
    const arr2 = ["a", "b", "c"];
    const arr3 = [1, "a", 2, "b"];
    const arr4 = [{}, {}, {}];
    for (let i = 0; i < 50; i++) {
        assert_equal(arr1[0], 1);
        assert_equal(arr2[0], "a");
        assert_equal(arr3[0], 1);
        assert_equal(typeof arr4[0], "object");
    }
}
testArrayDifferentElementKinds();

// Test array element load after push
function testArrayElementLoadAfterPush() {
    const arr = [];
    for (let i = 0; i < 100; i++) {
        arr.push(i);
        let res = arr[arr.length - 1];
        assert_equal(res, i);
    }
}
testArrayElementLoadAfterPush();

// Test array element load after pop
function testArrayElementLoadAfterPop() {
    const arr = [1, 2, 3, 4, 5];
    for (let i = 0; i < 5; i++) {
        let len = arr.length;
        let res = arr[len - 1];
        assert_equal(res, 5 - i);
        arr.pop();
    }
}
testArrayElementLoadAfterPop();

// Test array element load with float index
function testArrayElementLoadFloatIndex() {
    const arr = ["a", "b", "c"];
    for (let i = 0; i < 100; i++) {
        let res1 = arr[0.0];
        let res2 = arr[1.5];
        assert_equal(res1, "a");
        assert_equal(res2, undefined);
    }
}
testArrayElementLoadFloatIndex();

// Test array element load with object as index
function testArrayElementLoadObjectIndex() {
    const arr = ["a", "b", "c"];
    for (let i = 0; i < 100; i++) {
        let res = arr["1"];
        assert_equal(res, "b");
    }
}
testArrayElementLoadObjectIndex();

// Test array constructor property load
function testArrayConstructorPropertyLoad() {
    const arr = [];
    for (let i = 0; i < 100; i++) {
        let res = arr.constructor;
        assert_equal(res, Array);
    }
}
testArrayConstructorPropertyLoad();

// Test Array.isArray IC
function testArrayIsArray() {
    const arr = [1, 2, 3];
    const obj = { length: 3 };
    for (let i = 0; i < 100; i++) {
        let res1 = Array.isArray(arr);
        let res2 = Array.isArray(obj);
        assert_equal(res1, true);
        assert_equal(res2, false);
    }
}
testArrayIsArray();

// Test array reverse IC
function testArrayReverse() {
    const arr = [1, 2, 3];
    for (let i = 0; i < 100; i++) {
        let temp = arr.slice();
        let res = temp.reverse();
        assert_equal(res[0], 3);
        assert_equal(res[1], 2);
        assert_equal(res[2], 1);
    }
}
testArrayReverse();

// Test array sort IC
function testArraySort() {
    const arr = [3, 1, 4, 1, 5];
    for (let i = 0; i < 100; i++) {
        let temp = arr.slice();
        let res = temp.sort();
        assert_equal(res[0], 1);
        assert_equal(res[1], 1);
        assert_equal(res[2], 3);
    }
}
testArraySort();

// Test array concat IC
function testArrayConcat() {
    const arr1 = [1, 2];
    const arr2 = [3, 4];
    for (let i = 0; i < 100; i++) {
        let res = arr1.concat(arr2);
        assert_equal(res.length, 4);
        assert_equal(res[0], 1);
        assert_equal(res[3], 4);
    }
}
testArrayConcat();

test_end();
