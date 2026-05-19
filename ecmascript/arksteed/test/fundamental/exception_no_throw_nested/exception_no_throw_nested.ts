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

function exception_no_throw_nested(a: number, b: number, c: number): number {
  try {
    if (a < 0) {
      throw new Error("a is negative");
    }
    try {
      if (b < 0) {
        throw new Error("b is negative");
      }
      try {
        if (c < 0) {
          throw new Error("c is negative");
        }
        return a + b + c;
      } catch (e) {
        return -3;
      }
    } catch (e) {
      return -2;
    }
  } catch (e) {
    return -1;
  }
}

ArkTools.arkSteedCompileSync(exception_no_throw_nested);


print(exception_no_throw_nested(1, 2, 3));
print(exception_no_throw_nested(10, 20, 30));
print(exception_no_throw_nested(0, 0, 0));
