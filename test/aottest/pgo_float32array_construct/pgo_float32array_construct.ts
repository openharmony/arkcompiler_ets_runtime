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

function test() {
    let len1 = 2;
    let len2 = 3000;
    
    let array1 = new Float32Array(len1);
    let array2 = new Float32Array(len2);
    
    array1[0] = 1.1;
    array2[2000] = 2.2;
}

test()
print(ArkTools.isAOTCompiled(test))  // pgo:false, aot:true
