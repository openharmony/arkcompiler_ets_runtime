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

function iteratorCase(value: any, pick: boolean): number {
  let live = 40;
  let sum = 1;
  let seen = 0;
  try {
    live = live + 2;
    if (pick) {
      for (let item of value) {
        sum = sum + item;
        seen = seen + 1;
        if (seen > 1) {
          break;
        }
      }
    } else {
      sum = sum + 3;
    }
    return live + sum + seen;
  } catch (e) {
    return 1400 + live + sum + seen;
  }
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  iteratorCase([4, 5, 6], (i & 1) === 0);
}

print(iteratorCase([4, 5, 6], true));
ArkTools.arkSteedCompileSync(iteratorCase);
print(iteratorCase(7, true));
