/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * @tc.name: array filter
 * @tc.desc: test array filter
 * @tc.type: FUNC
 */
{
    const words = ['spray', 'elite', 'exuberant', 'destruction', 'present'];

    const result = words.filter((word) => word.length > 6);

    assert_equal(result, ['exuberant', 'destruction', 'present']);
}

{
    let wordss = ["spray", "limit", "exuberant", "destruction", "elite", "present"];

    const modifiedWords = wordss.filter((word, index, arr) => {
        arr[index + 1] += " extra";
        return word.length < 6;
    });

    assert_equal(modifiedWords, ['spray']);
}

{
    const words1 = ["spray", "limit", "exuberant", "destruction", "elite", "present"];
    const appendedWords = words1.filter((word, index, arr) => {
        arr.push("new");
        return word.length < 6;
    });
    assert_equal(appendedWords, ["spray", "limit", "elite"]);

    const words2 = ["spray", "limit", "exuberant", "destruction", "elite", "present"];
    const deleteWords = words2.filter((word, index, arr) => {
        arr.pop();
            return word.length < 6;
    });

    assert_equal(deleteWords, ["spray", "limit"]);
}

{
    const arrayLike = {
        length: 3,
            0: "a",
            1: "b",
            2: "c",
    };
    assert_equal(Array.prototype.filter.call(arrayLike, (x) => x <= "b"), ['a', 'b']);
}

{
    const fruits = ["apple", "banana", "grapes", "mango", "orange"];
    function filterItems(arr, query) {
        return arr.filter((el) => el.toLowerCase().includes(query.toLowerCase()));
    }

    assert_equal(filterItems(fruits, "ap"), ['apple', 'grapes']);
    assert_equal(filterItems(fruits, "an"), ['banana', 'mango', 'orange']);
}

{
    const array = [-3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13];

    function isPrime(num) {
        for (let i = 2; num > i; i++) {
            if (num % i === 0) {
                return false;
            }
        }
        return num > 1;
    }

    assert_equal(array.filter(isPrime), [2, 3, 5, 7, 11, 13]);

    const emptyArr = [];
    assert_equal(emptyArr.filter(isPrime).length, 0);
}

{
    var array1 = [,]
    function filter() {
        return array1.filter(v => v > 0 );
    }
    array1.__proto__.push(6);
    var narr = filter();
    let res = Object.getOwnPropertyDescriptor(narr, 0);
    assert_equal(res.value, 6);
    assert_equal(res.writable, true);
    assert_equal(res.enumerable, true);
    assert_equal(res.configurable, true);
}

{
    var bPar = true;
    var bCalled = false;

    function callbackfn(val, idx, obj)
    {
        bCalled = true;
        if (obj[idx] !== val) {
            bPar = false;
        }
    }

    var srcArr = [0, 1, true, null, new Object(), "five"];
    srcArr[9999999] = -6.6;
    var resArr = srcArr.filter(callbackfn);

    assert_equal(bCalled, true);
    assert_equal(bPar, true);
}

{
    const arr1 = [1, null, 3, undefined, 5, null];
    const result1 = arr1.filter(item => item != null);
    assert_equal(result1, [1, 3, 5]);

    const arr2 = [1, 2, 3, 4, 5];
    const result2 = arr2.filter((num, index) => {
        if (index === 2) {
            return false;
        } else {
            return true;
        }
    });
    assert_equal(result2, [1, 2, 4, 5]);
}

{
    const arr3 = [0, 'hello', NaN, '', false, 42, undefined];
    const result3 = arr3.filter((item, index) => {
        if (!item) {
            arr3.splice(index, 1);
            return false;
        }
        return true;
    });
    assert_equal(result3, []);
    assert_equal(arr3, ['hello', '', 42])

    let arr4 = [1, 2, 3, 4, 5];
    let result4 = arr4.map((num, index, array) => {
        if (index === 3) {
            array.pop();
        } else {
            return num * 2;
        }
    }).filter(num => num !== 6);
    assert_equal(result4, [2, 4, undefined]);
}

test_end();
