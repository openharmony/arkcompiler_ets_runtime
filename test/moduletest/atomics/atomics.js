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
 * @tc.name: Atomics.wait
 * @tc.desc: test Atomics.wait
 * @tc.type: FUNC
 * @tc.require: issue#11914
 */

{
    const v21 = new SharedArrayBuffer(32);
    const v22 = new BigInt64Array(v21);
    Atomics.or(v22, Int16Array, false);
    assert_equal(v22.toString(), "0,0,0,0");
    assert_equal(Atomics.wait(v22, false, true), "not-equal");
}

{
    let buf1 = new SharedArrayBuffer(8);
    let view1 = new BigInt64Array(buf1);
    try {
        Atomics.wait(view1, 0, 0x41414141);
    } catch (e) {
        assert_equal(e instanceof TypeError, true);
    }
}

{
    let buf2 = new SharedArrayBuffer(8);
    let view2 = new BigInt64Array(buf2);
    try {
        Atomics.wait(view2, 0, {});
    } catch(e) {
        assert_equal(e instanceof SyntaxError, true);
    }
}

{
    let buf = new SharedArrayBuffer(8);
    let view = new BigInt64Array(buf);
    assert_equal(Atomics.wait(view, 0, true), "not-equal");
}
test_end();