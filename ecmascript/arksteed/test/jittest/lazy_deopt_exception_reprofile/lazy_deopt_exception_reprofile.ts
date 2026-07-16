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

declare function print(value: number): void;

declare class ArkTools {
  static arkSteedCompileSync<T extends Function>(func: T): T;
}

let throwValue = 0;

function compileBeforeProfile(): number {
  try {
    throw 1;
  } catch (e) {
    return 200 + (e as number);
  }
}

function callee(): number {
  if (throwValue !== 0) {
    throw throwValue;
  }
  return 7;
}

function caller(): number {
  try {
    return callee();
  } catch (e) {
    return 100 + (e as number);
  }
}

ArkTools.arkSteedCompileSync(compileBeforeProfile);
print(compileBeforeProfile());

const REP = 60000;
for (let i = 0; i < REP; i++) {
  caller();  // Triggers PGO collection
}

print(caller());
ArkTools.arkSteedCompileSync(caller);
throwValue = 1;
print(caller());

throwValue = 0;
for (let i = 0; i < REP; i++) {
  caller();  // Rebuilds the profile after the first lazy deopt
}
print(caller());
ArkTools.arkSteedCompileSync(caller);
throwValue = 2;
print(caller());
