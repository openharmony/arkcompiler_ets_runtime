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
declare function print(...args: any[]): void;

function loop_while_6(n: number): number {
    let x0 = 1;
    let x1 = 2;
    let x2 = 3;
    let x3 = 4;
    let i = 0;
    while (i < n) {
        x0 = x0 + x1;
        x1 = x0 ^ x2;
        x2 = x2 + x3;
        i++;
    }
    return x0 + x1 + x2 + x3 + i;
}

ArkTools.arkSteedCompileSync(loop_while_6);
print(loop_while_6(3));
