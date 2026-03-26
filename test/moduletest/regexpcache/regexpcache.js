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

const str = "SheSellsSeashellsByTheSeashoreTheShellsSheSellsAreSurelySeashells";
const regex = /She/;

// 1. RegExp.prototype.test()
let test_res1 = regex.test(str);
let test_res2 = regex.test(str);
assert_equal(test_res1, test_res2);

// 2. RegExp.prototype.exec()
let exec_res1 = regex.exec(str);
let exec_res2 = regex.exec(str);
assert_equal(exec_res1, exec_res2);

// 3. String.prototype.match()
const match_res1 = str.match(regex);
const match_res2 = str.match(regex);
assert_equal(match_res1, match_res2);

// 4. String.prototype.replace()
const replace_res1 = str.replace(regex, "He");
const replace_res2 = str.replace(regex, "He");
assert_equal(replace_res1, replace_res2);

// 5. String.prototype.search()
const search_res1 = str.search(regex);
const search_res2 = str.search(regex);
assert_equal(search_res1, search_res2);

// 6. String.prototype.split()
const split_res1 = str.split(regex);
const split_res2 = str.split(regex);
assert_equal(split_res1, split_res2);

{
    // String.prototype.split
    let count = 0;
    const originalSplit = RegExp.prototype[Symbol.split];
    RegExp.prototype[Symbol.split] = function (str) {
        count++;
        const original = originalSplit.call(this, str);
        return original.map(item => `PROTOTYPE_${item}`);
    }

    const reg = /,/g
    const result = "a,b".split(reg);
    assert_equal(JSON.stringify(result), "[\"PROTOTYPE_a\",\"PROTOTYPE_b\"]")
    const result2 = "a,b".split(reg);
    assert_equal(JSON.stringify(result2), "[\"PROTOTYPE_a\",\"PROTOTYPE_b\"]")
    assert_equal(count, 2);
    RegExp.prototype[Symbol.split] = originalSplit;
}

{
    // RegExp.prototype[Symbol.split]
    let count = 0;
    const originalSplit = RegExp.prototype[Symbol.split];
    RegExp.prototype[Symbol.split] = function (str) {
        count++;
        const original = originalSplit.call(this, str);
        return original.map(item => `PROTOTYPE_${item}`);
    }
    const result = RegExp.prototype[Symbol.split].call(/,/g, "a,b");
    assert_equal(JSON.stringify(result), "[\"PROTOTYPE_a\",\"PROTOTYPE_b\"]")
    const result2 = RegExp.prototype[Symbol.split].call(/,/g, "a,b");
    assert_equal(JSON.stringify(result2), "[\"PROTOTYPE_a\",\"PROTOTYPE_b\"]")
    assert_equal(count, 2);
    RegExp.prototype[Symbol.split] = originalSplit;
}

{
    // String.prototype.replace
    let count = 0;
    const originalReplace = RegExp.prototype[Symbol.replace];
    RegExp.prototype[Symbol.replace] = function (str, replacement) {
        count++;
        const original = originalReplace.call(this, str, replacement);
        return `PROTOTYPE_${original}`;
    }

    const reg = /,/g
    const result = "a,b".replace(reg, "_");
    assert_equal(JSON.stringify(result), "\"PROTOTYPE_a_b\"")
    const result2 = "a,b".replace(reg, "_");
    assert_equal(JSON.stringify(result2), "\"PROTOTYPE_a_b\"")
    assert_equal(count, 2);
    RegExp.prototype[Symbol.replace] = originalReplace;
}

{
    // RegExp.prototype[Symbol.replace]
    let count = 0;
    const originalReplace = RegExp.prototype[Symbol.replace];
    RegExp.prototype[Symbol.replace] = function (str, replacement) {
        count++;
        const original = originalReplace.call(this, str, replacement);
        return `PROTOTYPE_${original}`;
    }

    const reg = /,/g
    const result = RegExp.prototype[Symbol.replace].call(reg, "a,b", "_");
    assert_equal(JSON.stringify(result), "\"PROTOTYPE_a_b\"")
    const result2 = RegExp.prototype[Symbol.replace].call(reg, "a,b", "_");
    assert_equal(JSON.stringify(result2), "\"PROTOTYPE_a_b\"")
    assert_equal(count, 2);
    RegExp.prototype[Symbol.replace] = originalReplace;
}

{
    // String.prototype.matchAll
    let count = 0;
    const originalMatchAll = RegExp.prototype[Symbol.matchAll];
    RegExp.prototype[Symbol.matchAll] = function (str) {
        count++;
        const original = [...originalMatchAll.call(this, str)];
        return original.map(item => `PROTOTYPE_${item}`);
    }

    const reg = /[0-9]+/g
    const str = "2025-11-11";
    const result = [...str.matchAll(reg)];
    assert_equal(JSON.stringify(result), "[\"PROTOTYPE_2025\",\"PROTOTYPE_11\",\"PROTOTYPE_11\"]")
    const result2 = [...str.matchAll(reg)];
    assert_equal(JSON.stringify(result2), "[\"PROTOTYPE_2025\",\"PROTOTYPE_11\",\"PROTOTYPE_11\"]")
    assert_equal(count, 2);
    RegExp.prototype[Symbol.matchAll] = originalMatchAll;
}

{
    // String.prototype.matchAll
    let count = 0;
    const originalMatchAll = RegExp.prototype[Symbol.matchAll];
    RegExp.prototype[Symbol.matchAll] = function (str) {
        count++;
        const original = [...originalMatchAll.call(this, str)];
        return original.map(item => `PROTOTYPE_${item}`);
    }

    const reg = /[0-9]+/g
    const str = "2025-11-11";
    const result = [...RegExp.prototype[Symbol.matchAll].call(reg, str)];
    assert_equal(JSON.stringify(result), "[\"PROTOTYPE_2025\",\"PROTOTYPE_11\",\"PROTOTYPE_11\"]")
    const result2 = [...RegExp.prototype[Symbol.matchAll].call(reg, str)];
    assert_equal(JSON.stringify(result2), "[\"PROTOTYPE_2025\",\"PROTOTYPE_11\",\"PROTOTYPE_11\"]")
    assert_equal(count, 2);
    RegExp.prototype[Symbol.matchAll] = originalMatchAll;
}

// Test large string to trigger caching
{
    let largeString = "";
    for (let i = 0; i < 100; i++) {
        largeString += "word" + i + " ";
    }

    const pattern = /word\d+/g;
    const results = [];

    for (let i = 0; i < 5; i++) {
        pattern.lastIndex = 0;
        let count = 0;
        while (pattern.exec(largeString) !== null && count < 200) {
            count++;
        }
        results.push(count);
    }

    const allCorrect = results.every(r => r === 100);
    assert_equal(allCorrect, true);
    print("Lines 1716 & 2071 - Large string caching: " + allCorrect);
}

// Test mixed operations
{
    const execResults = [];
    const splitResults = [];

    for (let i = 0; i < 10; i++) {
        const p1 = /\d+/g;
        const s1 = "123 456 789";
        p1.lastIndex = 0;
        let c1 = 0;
        while (p1.exec(s1) !== null && c1 < 10) {
            c1++;
        }
        execResults.push(c1);

        const s2 = "a1b2c3d4e5f6";
        const parts = s2.split(/\d/);
        splitResults.push(parts.length);
    }

    const execCorrect = execResults.every(r => r === 3);
    const splitCorrect = splitResults.every(r => r === 7);
    const mixedCorrect = execCorrect && splitCorrect;

    assert_equal(mixedCorrect, true);
    print("Lines 1716 & 2071 - Mixed operations: " + mixedCorrect);
}

test_end();
