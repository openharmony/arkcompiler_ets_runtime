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

let shouldThrow = false;
const tagged = { value: 7 };

function leaf(
  throwNow: boolean,
  i1: number, i2: number, i3: number, i4: number,
  i5: number, i6: number, i7: number, i8: number,
  i9: number, i10: number, i11: number, i12: number,
  i13: number, i14: number, i15: number, i16: number,
  d1: number, d2: number, objectValue: number): number {
  if (throwNow) {
    throw new Error("lazy deopt stack");
  }
  return i1 + objectValue + Math.trunc(d1 + d2);
}

function withStack(
  i1: number, i2: number, i3: number, i4: number,
  i5: number, i6: number, i7: number, i8: number,
  i9: number, i10: number, i11: number, i12: number,
  i13: number, i14: number, i15: number, i16: number,
  d1: number, d2: number, taggedObject: { value: number }): number {
  const duplicate = i1;
  const objectValue = taggedObject.value;
  try {
    return leaf(shouldThrow, i1, i2, i3, i4, i5, i6, i7, i8,
      i9, i10, i11, i12, i13, i14, i15, i16, d1, d2, objectValue);
  } catch (e) {
    const stack = (e as Error).stack;
    if (typeof stack !== "string" || stack.length === 0) {
      return -1;
    }
    return duplicate + i1 + i2 + i3 + i4 + i5 + i6 + i7 + i8 +
      i9 + i10 + i11 + i12 + i13 + i14 + i15 + i16 + d1 + d2 + objectValue;
  }
}

function invokeWithStack(): number {
  return withStack(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 1.5, 2.5, tagged);
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
  invokeWithStack();
}

print(invokeWithStack());
ArkTools.arkSteedCompileSync(leaf);
ArkTools.arkSteedCompileSync(withStack);
shouldThrow = true;
print(invokeWithStack());
