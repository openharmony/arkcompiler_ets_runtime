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
 * @tc.name:getvaluefrombuffer
 * @tc.desc:test BuiltinsTypedArrayStubBuilder::GetValueFromBuffer
 * @tc.type: FUNC
 * @tc.require: issueIBDK44
 */

// This case aims to check overflow of double in BuiltinsTypedArrayStubBuilder::GetValueFromBuffer
{
    let v0 = new ArrayBuffer(8);
    let v1 = new Int32Array(v0);
    v1[0] = 0xcafe0000;
    v1[1] = 0xffff0000;
    let v2 = new Float64Array(v0);
    Array.prototype.push.apply(v0, v2);
    assert_equal(Number.isNaN(v2[0]), true);
}

test_end();
