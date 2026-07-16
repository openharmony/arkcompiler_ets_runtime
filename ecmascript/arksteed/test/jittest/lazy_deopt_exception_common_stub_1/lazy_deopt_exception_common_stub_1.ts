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

const hotArray = [35];

function readElement(obj: any): number {
  let x = 7;
  try {
    x = obj[0];
    return x + 7;
  } catch (e) {
    return 700 + x;
  }
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  readElement(hotArray);
}

print(readElement(hotArray));
ArkTools.arkSteedCompileSync(readElement);
print(readElement(null));
