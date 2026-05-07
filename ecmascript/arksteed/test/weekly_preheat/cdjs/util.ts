/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

const PERCENTILE_DIVISOR: number = 100;
const NUMBER_THOUSAND: number = 1000;

export function compareNumbers(a: number, b: number): number {
  if (a === b) {
    return 0;
  }
  if (a < b) {
    return -1;
  }
  if (a > b) {
    return 1;
  }

  // We say that NaN is smaller than non-NaN.
  if (a === a) {
    return 1;
  }

  return -1;
}

export function averageAbovePercentile(numbers: number[], percentile: number): number {
  // Don't change the original array.

  // Sort in ascending order.
  numbers.sort((a, b) => {
    return a - b;
  });

  // Now the elements we want are at the end. Keep removing them until the array size shrinks too much.
  // Examples assuming percentile = 99:
  //
  // - numbers.length starts at 100: we will remove just the worst entry and then not remove anymore,
  //   since then numbers.length / originalLength = 0.99.
  //
  // - numbers.length starts at 1000: we will remove the ten worst.
  //
  // - numbers.length starts at 10: we will remove just the worst.
  let numbersWeWant: number[] = [];
  const originalLength = numbers.length;
  while (numbers.length / originalLength > percentile / PERCENTILE_DIVISOR) {
    numbersWeWant.push(numbers.pop()!);
  }

  let sum = 0.0;
  for (const number of numbersWeWant) {
    sum += number;
  }

  let result = sum / numbersWeWant.length;

  // Do a sanity check.
  if (numbers.length > 0 && result < numbers[numbers.length - 1]) {
    throw new Error('Sanity check fail: the worst case result is ' + result + " but we didn't take into account " + numbers);
  }

  return result;
}

export const currentTime = (): number => {
  return 0 + ArkTools.timeInUs() / NUMBER_THOUSAND;
};
