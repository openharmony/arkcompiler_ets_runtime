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
 * @tc.name:obj_dictionarymode_legacy
 * @tc.desc:test dictionary elements with V70 optimization disabled
 * @tc.type: FUNC
 * @tc.require: issueICNDYR
 */

function assertDictionaryElements(obj, expected) {
    assert_equal(ArkTools.hasDictionaryElements(obj), expected);
}

{
    let obj = {};
    for (let i = 0; i < 8; i++) {
        obj[i] = i;
    }
    obj.named = 42;
    assert_equal(ArkTools.hasFastProperties(obj), true);
    delete obj[1];
    assert_equal(obj[1], undefined);
    assert_equal(obj[2], 2);
    assert_equal(obj.named, 42);
    assert_equal(ArkTools.hasFastProperties(obj), false);
    assertDictionaryElements(obj, true);
}

{
    let arr = new Array(8).fill(0);
    arr.named = 42;
    assert_equal(ArkTools.hasFastProperties(arr), true);
    delete arr[1];
    assert_equal(arr.length, 8);
    assert_equal(arr[1], undefined);
    assert_equal(arr[2], 0);
    assert_equal(arr.named, 42);
    assert_equal(ArkTools.hasFastProperties(arr), false);
    assertDictionaryElements(arr, true);
}

test_end();
