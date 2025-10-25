/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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
 * @tc.name: arrayfindlast
 * @tc.desc: test Array.findLast and Array.findLastIndex
 * @tc.type: FUNC
 * @tc.require: issueI7LP2E
 */
{
    var arr = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
    var result = -1;
    result = arr.findLast((element, index, array) => {
        array.length = 5;
        return (element == 5);
    });
    assert_equal(result, undefined);

    result = arr.findLast((element) => {
        return element > 2;
    });
    assert_equal(result, 4);

    result = arr.findLastIndex((element, index, array) => {
        if (array.length == 5) {
            array.push(100);
        }
        return (element == 100);
    });
    assert_equal(result, -1);

    result = arr.findLastIndex((element, index, array) => {
        return (element == 100);
    });
    assert_equal(result, 5);
}

{
    var arr2 = new Array(1025);
    result = arr2.findLast((element, index, array) => {
        return (element == undefined);
    });
    assert_equal(result, undefined);

    result = arr2.findLastIndex((element, index, array) => {
        return (element == undefined);
    });
    assert_equal(result, 1024);

    result = arr2.findLastIndex((element, index, array) => {
        array[5] = 100;
        return (element == 100);
    });
    assert_equal(result, 5);
}

{
    var arr3 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
    var arr4 = new Array();
    function testFunction(element, index, array) {
        if (index == 9) {
            array.length = 6;
        }
        return element < 1;
    }
    for (let i = 0; i < 10; i++) {
        arr4[i] = i;
    }

    result = arr3.findLast(testFunction);
    assert_equal(result, 0);
    result = arr4.findLast(testFunction);
    assert_equal(result, 0);
}

{
    let arr1 = [1, 2, , 7, , undefined];
    arr1.__proto__.push(9);
    arr1.__proto__.push(9);
    arr1.__proto__.push(9);
    function fun1(element){
        return element === 1;
    }
    assert_equal(arr1.findLast(fun1), 1);
    arr1.__proto__.pop(9);
    arr1.__proto__.pop(9);
    arr1.__proto__.pop(9);
}

{
    // slowPath
    let arr5 = [1, 2, 3, 4, 5, 6, 7, 8];
    function func2(element, index, arr) {
        return element === 6;
    }
    assert_equal(arr5.findLast(func2), 6);
}

{
    // not Found
    let arr6 = new Array(5, 2, 7);
    function func3(element, index, arr) {
        arr.length = 3;
        return element === 100;
    }
    assert_equal(arr6.findLast(func3), undefined);
}

{
    function call(obj, ...args) {
        obj.findLast(...args);
    }
    var v1 = new Array(5);
    function f1() {
        return v1.pop();
    }
    call(v1, f1);
}

test_end()
