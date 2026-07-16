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

declare function print(arg: number): string;

declare class ArkTools {
  static arkSteedCompileSync<T extends Function>(func: T): T;
}

let shouldThrow = false;

function maybeThrow(flag: boolean, tag: number): number {
  if (flag) {
    throw { branch: tag, code: tag * 10 };
  }
  return tag + 1;
}

function exception_no_throw_catch_phi_2(mode: number): number {
  const liveBeforeTry = 24;
  let x = 0;
  let y = 6;
  try {
    if (mode === 0) {
      x = liveBeforeTry;
      maybeThrow(shouldThrow, 11);
      y = x + 10;
    } else {
      x = liveBeforeTry + 1;
      maybeThrow(shouldThrow, 12);
      y = x + 20;
    }
    return liveBeforeTry + x + y;
  } catch (e) {
    const obj = e as any;
    return liveBeforeTry * 100 + x * 10 + y + obj.branch + obj.code;
  }
}

print(exception_no_throw_catch_phi_2(0));
print(exception_no_throw_catch_phi_2(1));
ArkTools.arkSteedCompileSync(exception_no_throw_catch_phi_2);
shouldThrow = true;
print(exception_no_throw_catch_phi_2(0));
print(exception_no_throw_catch_phi_2(1));
