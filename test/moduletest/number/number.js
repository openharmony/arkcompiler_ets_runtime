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
 * @tc.name:Number
 * @tc.desc:test Number
 * @tc.type: FUNC
 * @tc.require: issueI8S2AX
 */

print(1.25.toFixed(1))
print(1.25.toPrecision(2))
print(0.000112356.toExponential())
print((-(true)).toPrecision(0x30, 'lib1-f1'))