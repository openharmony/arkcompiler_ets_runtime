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
 * @tc.name: arrayjoin
 * @tc.desc: test Array.join
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

// do not modify prototype before this
{
    let arr = new Array(10).fill(1);
    arr[0] = {
        toString() {
            arr.length = 10000;
            return 2;
        }
    }
    let res = arr.join();
    print(res);
    assert_equal(res, '2,1,1,1,1,1,1,1,1,1');
}

{
    let arr = new Array(10).fill(1);
    let res = arr.join({
        toString() {
            arr.length = 10000;
            return "-";
        }
    });
    print(res);
    assert_equal(res, '1-1-1-1-1-1-1-1-1-1');
}

{
    var a = new Array(1).join("  ");
    print(a.length);
    assert_equal(a.length, 0);
    var str1 = Array(3).join("0");
    print(str1);
    assert_equal(str1, '00');
    var str2 = new Array(3).join("0");
    print(str2);
    assert_equal(str2, '00');
    const arr = []
    arr.length = 3
    var str3 = arr.join("0");
    assert_equal(str2, '00');
}

// test circular reference
{
    var arr1 = [1];
    arr1.push(arr1);
    arr1.push(arr1);
    assert_equal(arr1.toString(), '1,,');
}

{
    var arr2 = [1];
    var arr3 = [2];
    arr2[10] = arr3;
    arr3[10] = arr2;
    assert_equal(arr2.toString(), '1,,,,,,,,,,2,,,,,,,,,,');
}

{
    var arr4 = [1];
    var arr5 = [2];
    var arr6 = [3];
    arr4.push(arr5);
    arr5.push(arr6);
    arr6.push(arr4);
    assert_equal(arr4.toString(), '1,2,3,');
}

{
    var arr7 = [
        {
            toLocaleString() {
                return [1, arr7];
            }
        }
    ];
    assert_equal(arr7.toLocaleString(), '1,');
}
{
    var aa = this;
    var bb = {};
    aa.length = 4294967296; // 2 ^ 32 (max array length + 1)
    try {
        Array.prototype.join.call(aa,bb)
    } catch (e) {
        assert_equal(e instanceof TypeError, true);
    }

    try {
        Object.getOwnPropertyDescriptors(Array(1e9).join('c'))
    } catch (e) {
        assert_equal(e instanceof RangeError, true);
    }
}

{
    ([11])["join"]('쏄');
    let proxy1 = new Proxy([123], {});
    proxy1.pop();
    proxy1.toString();
    proxy1.push(456);
    assert_equal(proxy1.toString(), '456')

    let proxy2 = new Proxy([123, 456], {});
    proxy2.pop();
    proxy2.toString();
    proxy2.push(456);
    assert_equal(proxy2.toString(),'123,456');
}

{
    const v5 = new Float32Array(1);
    v5[0] = NaN;
    let res1 = v5.join(String.fromCodePoint(0));
    assert_equal(res1, 'NaN');

    const v6 = new Float32Array(1);
    v6[0] = NaN;
    v6[1] = NaN;
    let res2 = v6.join(String.fromCodePoint(0));
    assert_equal(res2, 'NaN');

    const v7 = new Float32Array(2);
    v7[0] = NaN;
    let res3 = v7.join(String.fromCodePoint(0));
    assert_equal(res3, 'NaN\x000');
}

{
    const element = {
        toString() {
            Array.prototype[1] = 'b';
            return 'a';
        }
    };
    const arr_join = [element, ,'c'];
    let res = "abc" == arr_join.join('');
    assert_equal(res, true);

    const v9 = new Int16Array(128);
    const v10 = new Int8Array(128);
    v9[11] = [v9, v10];
    assert_equal(v9[11], 0);
}

test_end();