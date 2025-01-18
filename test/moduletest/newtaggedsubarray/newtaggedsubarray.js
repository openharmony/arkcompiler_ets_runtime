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
 * @tc.name: newtaggedsubarray
 * @tc.desc: test NewObjectStubBuilder::NewTaggedSubArray
 * @tc.type: FUNC
 * @tc.require: issueIBFJWW
 */

{
    let uint16arr = new Uint16Array(new ArrayBuffer(0x200));
    let set = {set:undefined};
    Reflect.defineProperty(uint16arr, ('toString'), set);
    let arr = uint16arr.subarray(31);
    arr.toString();
    print("Test runs successfully!");
}
