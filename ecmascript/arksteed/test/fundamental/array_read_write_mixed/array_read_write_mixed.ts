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
function array_read_write_mixed(arr: number[]): number
{
    let a = arr[0];
    arr[0] = a * 10;
    let b = arr[0];

    let c = arr[2];
    arr[2] = c + 100;
    let d = arr[2];

    return arr[0] + arr[1] + arr[2] + arr[3] + arr[4];
}

ArkTools.arkSteedCompileSync(array_read_write_mixed);


print(array_read_write_mixed([1, 2, 3, 4, 5]));
