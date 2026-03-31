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
 * @tc.name:storeicarray
 * @tc.desc:test storeic on arrays
 * @tc.type: FUNC
 * @tc.require: issueI7UTOA
 */

// Test basic array element store
function testBasicArrayElementStore() {
    const arr = new Array(100);
    for (let i = 0; i < 100; i++) {
        arr[i] = i;
        assert_equal(arr[i], i);
    }
}
testBasicArrayElementStore();

// Test array element store with initialization
function testArrayElementStoreWithInit() {
    const arr = [];
    for (let i = 0; i < 100; i++) {
        arr[i] = i * 2;
        assert_equal(arr[i], i * 2);
    }
}
testArrayElementStoreWithInit();

// Test array push method
function testArrayPush() {
    const arr = [];
    for (let i = 0; i < 100; i++) {
        arr.push(i);
        assert_equal(arr[arr.length - 1], i);
        assert_equal(arr.length, i + 1);
    }
}
testArrayPush();

// Test array unshift method
function testArrayUnshift() {
    const arr = [];
    for (let i = 0; i < 50; i++) {
        arr.unshift(i);
        assert_equal(arr[0], i);
    }
}
testArrayUnshift();

// Test array element overwrite
function testArrayElementOverwrite() {
    const arr = [0, 0, 0, 0, 0];
    for (let i = 0; i < 100; i++) {
        arr[0] = i;
        arr[1] = i + 1;
        arr[2] = i + 2;
        assert_equal(arr[0], i);
        assert_equal(arr[1], i + 1);
        assert_equal(arr[2], i + 2);
    }
}
testArrayElementOverwrite();

// Test array element store with different types
function testArrayElementStoreDifferentTypes() {
    const arr = [];
    const values = [1, "string", true, null, undefined, {}, []];
    for (let i = 0; i < 100; i++) {
        arr[i] = values[i % values.length];
        assert_equal(arr[i], values[i % values.length]);
    }
}
testArrayElementStoreDifferentTypes();

// Test array length modification
function testArrayLengthModification() {
    const arr = [1, 2, 3, 4, 5];
    for (let i = 0; i < 100; i++) {
        arr.length = 10;
        assert_equal(arr.length, 10);
    }
}
testArrayLengthModification();

// Test array element store after length change
function testArrayStoreAfterLengthChange() {
    const arr = [1, 2, 3];
    arr.length = 10;
    for (let i = 0; i < 100; i++) {
        arr[5] = i;
        assert_equal(arr[5], i);
    }
}
testArrayStoreAfterLengthChange();

// Test sparse array element store
function testSparseArrayElementStore() {
    const arr = [];
    for (let i = 0; i < 100; i++) {
        arr[i * 10] = i;
        assert_equal(arr[i * 10], i);
    }
}
testSparseArrayElementStore();

// Test array pop and re-store
function testArrayPopAndRestore() {
    const arr = [];
    for (let i = 0; i < 50; i++) {
        arr.push(i);
    }
    for (let i = 0; i < 50; i++) {
        arr.pop();
        arr.push(i + 100);
        assert_equal(arr[arr.length - 1], i + 100);
    }
}
testArrayPopAndRestore();

// Test array splice and store
function testArraySpliceAndStore() {
    const arr = [1, 2, 3, 4, 5];
    for (let i = 0; i < 100; i++) {
        arr.splice(2, 0, i);
        assert_equal(arr[2], i);
        arr.splice(2, 1);
    }
}
testArraySpliceAndStore();

// Test array fill method
function testArrayFill() {
    const arr = new Array(10);
    for (let i = 0; i < 100; i++) {
        arr.fill(i);
        assert_equal(arr[0], i);
        assert_equal(arr[9], i);
    }
}
testArrayFill();

// Test array copyWithin method
function testArrayCopyWithin() {
    const arr = [1, 2, 3, 4, 5];
    for (let i = 0; i < 100; i++) {
        arr[0] = i;
        arr.copyWithin(0, 3);
        assert_equal(arr[0], 4);
    }
}
testArrayCopyWithin();

// Test array element store with negative index
function testArrayNegativeIndexStore() {
    const arr = [];
    for (let i = 0; i < 100; i++) {
        arr[-1] = i;
        assert_equal(arr[-1], i);
    }
}
testArrayNegativeIndexStore();

// Test array element store with string index
function testArrayStringIndexStore() {
    const arr = [];
    for (let i = 0; i < 100; i++) {
        arr["key" + i] = i;
        assert_equal(arr["key" + i], i);
    }
}
testArrayStringIndexStore();

// Test array property store
function testArrayPropertyStore() {
    const arr = [];
    for (let i = 0; i < 100; i++) {
        arr.custom = i;
        assert_equal(arr.custom, i);
    }
}
testArrayPropertyStore();

// Test array concat and store
function testArrayConcatAndStore() {
    const arr1 = [1, 2];
    const arr2 = [3, 4];
    for (let i = 0; i < 100; i++) {
        let result = arr1.concat(arr2);
        result.push(i);
        assert_equal(result[result.length - 1], i);
    }
}
testArrayConcatAndStore();

// Test array reverse and store
function testArrayReverseAndStore() {
    const arr = [1, 2, 3, 4, 5];
    for (let i = 0; i < 100; i++) {
        arr.reverse();
        arr[0] = i;
        assert_equal(arr[0], i);
    }
}
testArrayReverseAndStore();

// Test array sort and store
function testArraySortAndStore() {
    const arr = [5, 3, 1, 4, 2];
    for (let i = 0; i < 100; i++) {
        arr.sort();
        arr.push(i);
        assert_equal(arr[arr.length - 1], i);
        arr.pop();
    }
}
testArraySortAndStore();

// Test large array element store
function testLargeArrayElementStore() {
    const arr = new Array(1000);
    for (let i = 0; i < 100; i++) {
        arr[i * 10] = i;
        assert_equal(arr[i * 10], i);
    }
}
testLargeArrayElementStore();

// Test array element store consistency
function testArrayStoreConsistency() {
    const arr = [0];
    for (let i = 0; i < 100; i++) {
        arr[0] = i;
        arr[0] = i + 1;
        arr[0] = i + 2;
        assert_equal(arr[0], i + 2);
    }
}
testArrayStoreConsistency();

test_end();
