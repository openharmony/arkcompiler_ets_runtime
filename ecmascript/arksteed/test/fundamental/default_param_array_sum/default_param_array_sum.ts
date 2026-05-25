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
function default_param_array_sum(values: number[] = [1, 2, 3], scale: number = 2) {
    let sum = 0;
    for (let i = 0; i < values.length; i++) {
        sum += values[i] * scale;
    }
    print(sum);
}

ArkTools.arkSteedCompileSync(default_param_array_sum);


default_param_array_sum();
default_param_array_sum([4, 5], 3);
