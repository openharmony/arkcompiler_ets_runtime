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
 * @tc.name:setconstructor
 * @tc.desc:test setconstructor
 * @tc.type: FUNC
 * @tc.require: issue11699
 */

function testBasicNestedArrays() {
    let arr1 = [1, 2];
    let arr2 = [3, 4];
    let set = new Set([arr1, arr2]);
    
    assert_equal(set.has(arr1), true);
    assert_equal(set.has(arr2), true);
    assert_equal(set.size, 2);
}
testBasicNestedArrays();

function testMultiLevelNestedArrays() {
    let nestedArr1 = [1, [2, [3, 4]]];
    let nestedArr2 = [5, [6, [7, 8]]];
    let set = new Set([nestedArr1, nestedArr2]);
    
    assert_equal(set.has(nestedArr1), true);
    assert_equal(set.has(nestedArr2), true);
    assert_equal(set.size, 2);
}
testMultiLevelNestedArrays();

function testMixedTypeNestedArrays() {
    let mixedArr1 = [1, "hello", [true, false]];
    let mixedArr2 = [{name: "John"}, [1, 2, 3]];
    let set = new Set([mixedArr1, mixedArr2]);
    
    assert_equal(set.has(mixedArr1), true);
    assert_equal(set.has(mixedArr2), true);
    assert_equal(set.size, 2);
}
testMixedTypeNestedArrays();

function testEmptyAndSparseArrays() {
    let emptyArr = [];
    let sparseArr = [1, , 3];
    let set = new Set([emptyArr, sparseArr]);
    
    assert_equal(set.has(emptyArr), true);
    assert_equal(set.has(sparseArr), true);
    assert_equal(set.size, 2);
}
testEmptyAndSparseArrays();

function testObjectArrayNesting() {
    let objArr1 = [{id: 1}, [2, 3]];
    let objArr2 = [[4, 5], {id: 6}];
    let set = new Set([objArr1, objArr2]);
    
    assert_equal(set.has(objArr1), true);
    assert_equal(set.has(objArr2), true);
    assert_equal(set.size, 2);
}
testObjectArrayNesting();

function testSameContentDifferentReferences() {
    let nestedArr1 = [1, [2, 3]];
    let nestedArr2 = [1, [2, 3]];
    let set = new Set([nestedArr1, nestedArr2]);
    
    assert_equal(set.has(nestedArr1), true);
    assert_equal(set.has(nestedArr2), true);
    assert_equal(set.size, 2);
}
testSameContentDifferentReferences();

function testComplexMultiLevelStructures() {
    let complexArr1 = [1, [2, [3, [4, [5]]]]];
    let complexArr2 = [{a: 1}, [2, {b: 3}, [4, [5]]]];
    let set = new Set([complexArr1, complexArr2]);
    
    assert_equal(set.has(complexArr1), true);
    assert_equal(set.has(complexArr2), true);
    assert_equal(set.size, 2);
}
testComplexMultiLevelStructures();

test_end();