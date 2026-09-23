/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// @ts-nocheck
declare function print(value: any): void;

function incrementElement(array, index) {
    let value = array[index] + 1;
    array[index] = value;
    return array[index];
}

let warm = [0, 0, 0, 0];
for (let i = 0; i < 20; i++) {
    incrementElement(warm, 1);
}
ArkTools.arkSteedCompileSync(incrementElement);

let fresh = [0, 0, 0, 0];
print(incrementElement(fresh, 1));
print(incrementElement(fresh, 1));
