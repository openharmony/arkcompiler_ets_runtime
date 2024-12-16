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

function TestArrayConcat() {
    let arr1 = [];
    let arr2 = [];
    let arr3 = [1, 2, 3, 4];
    let arr4 = arr1.concat(arr2);
    let arr5 = arr3.concat(arr2);
    let arr6 = arr4.concat(arr5);
    let arr7 = arr1.concat();
    let arr8 = arr3.concat();
    print(arr5);
    print(arr6);
    print(arr4.length);
    print(arr7.length);
    print(arr8);
}

TestArrayConcat();
print(ArkTools.isAOTCompiled(TestArrayConcat));
ArkTools.printTypedOpProfiler("TYPED_CALL_BUILTIN_SIDE_EFFECT");
ArkTools.clearTypedOpProfiler();