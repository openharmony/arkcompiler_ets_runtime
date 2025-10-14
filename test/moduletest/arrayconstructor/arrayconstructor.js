/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
 * @tc.name: arrayconstauctor
 * @tc.desc: test Array constructor
 * @tc.type: FUNC
 */
class CustomArray extends Array {}

function testSpeciesEffect() {
    let custom = [1, 2, 3, 4, 5, 6, 7, 8, 9]
    custom.constructor = CustomArray;

    // Array.prototype.concat
    let result = custom.concat([4, 5]);
    assert_equal(true, result instanceof CustomArray); // true

    // Array.prototype.map
    result = custom.map(x => x * 2);
    assert_equal(true, result instanceof CustomArray); // true

    // Array.prototype.filter
    result = custom.filter(x => x > 1);
    assert_equal(true, result instanceof CustomArray); // true

    // Array.prototype.slice
    result = custom.slice(1);
    assert_equal(true, result instanceof CustomArray); // true

    // Array.prototype.flatMap
    result = custom.flatMap(x => [x, x * 2]);
    assert_equal(true, result instanceof CustomArray); // true

    // Array.prototype.flat
    result = custom.flat();
    assert_equal(true, result instanceof CustomArray); // true

    // Array.prototype.splice
    result = custom.splice(1, 2, 3);
    assert_equal(true, result instanceof CustomArray); // true

    // Array.prototype.toReversed
    result = custom.toReversed();
    assert_equal(true, result instanceof Array); // true

    // Array.prototype.toSorted
    result = custom.toSorted();
    assert_equal(true, result instanceof Array); // true

    // Array.prototype.toSpliced
    result = custom.toSpliced(0, 1);
    assert_equal(true, result instanceof Array); // true

    // Array.prototype.toSpliced
    result = custom.with(0, 1);
    assert_equal(true, result instanceof Array); // true
}

testSpeciesEffect()

test_end();
