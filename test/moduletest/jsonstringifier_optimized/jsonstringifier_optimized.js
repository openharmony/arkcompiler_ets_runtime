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


// Disable ENABLE_NEXT_OPTIMIZATION cannot run the following tests

{
    // chainese and english space with length 9
    let obj = { a: 1, b: { c: 2 } };
    let expected = `{
中文-En9+"a": 1,
中文-En9+"b": {
中文-En9+中文-En9+"c": 2
中文-En9+}
}`;

    let space = "中文-En9+";
    assert_equal(JSON.stringify(obj, null, space), expected);
}

{
    // chainese and english space with length 10
    let obj = { a: 1, b: { c: 2 } };
    let expected = `{
混合Mixin-12"a": 1,
混合Mixin-12"b": {
混合Mixin-12混合Mixin-12"c": 2
混合Mixin-12}
}`;
    let space = "混合Mixin-12";
    assert_equal(JSON.stringify(obj, null, space), expected);
}

{
    // chainese and english space with length 11
    let obj = { a: 1, b: { c: 2 } };
    let expected = `{
测试Test-567"a": 1,
测试Test-567"b": {
测试Test-567测试Test-567"c": 2
测试Test-567}
}`;
    let space = "测试Test-5678";
    assert_equal(JSON.stringify(obj, null, space), expected);
}

{
    // chainese characters and space
    let obj = { a: 1, b: { c: 2 } };
    let expected = `{
九空格　　　"a": 1,
九空格　　　"b": {
九空格　　　九空格　　　"c": 2
九空格　　　}
}`;
    let space = "九空格　　　";
    assert_equal(JSON.stringify(obj, null, space), expected);
}

test_end();