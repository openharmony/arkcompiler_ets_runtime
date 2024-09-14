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

// @ts-nocheck
declare function print(arg: any): string;

function func_1() {
    var ret = 0;
    var var_44 = (30);
    for (var sunzibo = 0; sunzibo < 1; sunzibo++) {
        var var_48 = (82);
        try {
            try {
                var_44 = (var_48--);
                var_48 = (var_44--);
                ret = var_48 + Number(0);
            } catch(e) {
                ret--;
            }
        } catch(e) {}
    }
}
func_1()
print(ArkTools.isAOTCompiled(func_1))
