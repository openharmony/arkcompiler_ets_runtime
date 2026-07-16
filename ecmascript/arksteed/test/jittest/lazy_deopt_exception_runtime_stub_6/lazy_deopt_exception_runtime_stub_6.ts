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

function copyDataCase(src: any, cond: boolean): number {
  let live = 90;
  let marker = 7;
  try {
    if (cond) {
      live = live + 2;
    } else {
      live = live + 3;
    }
    const dst = { head: marker, ...src };
    marker = dst.a + dst.head;
    return live + marker;
  } catch (e) {
    return 1900 + live + marker;
  }
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  copyDataCase({ a: 43 }, true);
}

print(copyDataCase({ a: 43 }, true));
ArkTools.arkSteedCompileSync(copyDataCase);
print(copyDataCase(null, true));
