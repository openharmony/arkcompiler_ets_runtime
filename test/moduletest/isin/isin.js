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
 * @tc.name: isin
 * @tc.desc: test isin. Test whether the return value of IsIn is exception while input para is not a ECMA obj.
 * @tc.type: FUNC
 */
{
    try {
        1 in 0;
        let tmpArr = [0];
        assert_unreachable();
    } catch (e) {
        assert_equal(e instanceof TypeError, true);
    }
}

{
    const v5 = [-3];
    const a8 = new Uint8Array(2);
    v5.__proto__ = a8;
    assert_equal(-3 in v5, false);
}

{
    const obj = { a: 1 };
    assert_equal('a' in obj, true);
    assert_equal('b' in obj, false);
}

{
    const arr = ['x'];
    assert_equal(0 in arr, true);
    assert_equal(1 in arr, false);
    arr[100] = 'y';
    assert_equal(100 in arr, true);
    assert_equal('length' in arr, true);
}

{
    const base = { foo: 1 };
    const derived = Object.create(base);
    assert_equal('foo' in derived, true);
    derived.foo = 2;
    assert_equal('foo' in derived, true);
}

{
    try {
        'x' in undefined;
        assert_unreachable();
    } catch (e) {
        assert_equal(e instanceof TypeError, true);
    }

    try {
        'y' in null;
    } catch (e) {
        assert_equal(e instanceof TypeError, true);
    }
}

{
    const sym = Symbol('s');
    const obj = { [sym]: 42 };
    assert_equal(sym in obj, true);
    assert_equal('s' in obj, false);
}

{
    const target = {};
    const proxy = new Proxy(target, {
        has(target, prop) {
            return prop === 'custom';
        }
    });
    assert_equal('custom' in proxy, true);
    assert_equal('another' in proxy, false);
}

{
    try {
        'x' in 42;
        assert_unreachable();
    } catch (e) {
        assert_equal(e instanceof TypeError, true);
    }

    try {
        'length' in 'abc';
        assert_unreachable();
    } catch (e) {
        assert_equal(e instanceof TypeError, true);
    }
}

test_end();