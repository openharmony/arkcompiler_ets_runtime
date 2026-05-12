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
function array_set_element(arr: number[], index: number, value: number): void
{
    arr[index] = value;
}

ArkTools.arkSteedCompileAsync(array_set_element);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let arr: number[] = [10, 20, 30, 40, 50];
array_set_element(arr, 1, 200);
print(arr[1]);
