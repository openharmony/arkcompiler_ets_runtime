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

declare function print(arg:any):string;
declare interface ArkTools {
    isAOTCompiled(args: any): boolean;
}

function testKeysRetype() {
    const v0 = []
    const v8 = [1]
    let v19 = v8.keys()
    for (let i22 = 0; i22 < 1; i22++) {
        v19 = i22
    }
    v0[0] = v19
}

//aot: [trace] aot inline function name: #*#testKeysRetype@builtinArrayKeys caller function name: func_main_0@builtinArrayKeys
//aot: [trace] aot inline builtin: Array.prototype.keys, caller function name:#*#testKeysRetype@builtinArrayKeys
testKeysRetype()
//aot: true
//pgo: false
print(ArkTools.isAOTCompiled(testKeysRetype))