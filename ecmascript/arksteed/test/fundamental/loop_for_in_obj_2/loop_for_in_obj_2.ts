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

function loop_for_in_obj_2(obj: Record<string, number>): number {
    let sumLength = 0;
    let sumValue = 0;
    for (const key in obj) {
        sumLength += key.length;
        sumValue += obj[key];
    }
    return sumLength + sumValue * 100;
}

ArkTools.arkSteedCompileSync(loop_for_in_obj_2);


print(loop_for_in_obj_2({}));
print(loop_for_in_obj_2({"first": 1, "second": 2}));
print(loop_for_in_obj_2({"one": 1, "two": 2, "three": 3, "four": 4}));
