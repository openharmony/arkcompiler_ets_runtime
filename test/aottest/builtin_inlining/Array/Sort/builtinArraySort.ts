/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

let result = null;
function numberTest() {
  let a = new Array(1000);
  for (let i = 0; i < 1000; i++) {
    a[i] = i;
  }
  result = a.sort();

}
//aot: [trace] aot inline function name: #*#numberTest@builtinArraySort caller function name: func_main_0@builtinArraySort
//aot: [trace] aot call builtin: Array.Sort, caller function name:#*#numberTest@builtinArraySort
//pgo: false
//aot: true
numberTest();
print(ArkTools.isAOTCompiled(numberTest))

