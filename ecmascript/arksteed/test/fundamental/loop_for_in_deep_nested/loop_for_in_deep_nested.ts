/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

declare function print(str: number):string;

function loop_for_in_deep_nested(data: Record<string, Record<string, Record<string, number>>>): number {
    let sum = 0;
    for (const k1 in data) {
        let level1 = data[k1];
        for (const k2 in level1) {
            let level2 = level1[k2];
            for (const k3 in level2) {
                sum += level2[k3];
            }
        }
    }
    return sum;
}

ArkTools.arkSteedCompileSync(loop_for_in_deep_nested);


print(loop_for_in_deep_nested({a: {x: {i: 1, j: 2}, y: {k: 3}}}));
print(loop_for_in_deep_nested({a: {x: {i: 10}}}));
print(loop_for_in_deep_nested({}));
print(loop_for_in_deep_nested({a: {}}));
