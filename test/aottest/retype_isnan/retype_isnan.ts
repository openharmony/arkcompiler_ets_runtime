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

declare interface ArkTools {
    isAOTCompiled(args: any): boolean;
}

declare function print(arg:any):string;
function test(): void {
    for (let v1 = 0; v1 < 1; v1++) {
        const v2 = isNaN(v1);
        for (let v3 = 0; v3 < 3; v3++) {
            BigInt.asUintN(v2, v2);
        }
    }
}

test();
print(ArkTools.isAOTCompiled(test));