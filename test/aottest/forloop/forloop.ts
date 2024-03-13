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
declare function assert_equal(a: Object, b: Object):void;

function foo(n: number) {
    let x: number = 0;
    for (let i: number = 0; i <= n; i++) {
        x = (i + 0.5) * (i + 0.5);
    }
    return x;
}

assert_equal(foo(300), 90300.25);

function forLoop(n: number): number {
  let sum = 0;
  for (let i = 0; i < n; i++) {
    sum++;
  }
  return sum;
}

function forLoopWithBreak(n: number): number {
  let sum = 0;
  for (let i = 0; i < n; i++) {
    if (sum >= 5) {
        sum++;
    } else {
        sum--;
        break;
    }
  }
  return sum;
}

function forLoopWithContinue(n: number): number {
  let sum = 0;
  for (let i = 0; i < n; i++) {
    if (sum >= 5) {
      sum++;
    } else {
      sum--;
      continue;  
    }
  }
  return sum;
}

function forNestedLoop0(n: number): number {
  let sum = 0;
  a: for (let i = 0; i < n; i++) {
    for (let j = 0; j < n; j++) {
        if (sum >= 5) {
            sum++;
            break a;
        }
        sum--;
    }
    sum--;
  }
  return sum;
}

function forNestedLoop1(n: number): number {
  let sum = 0;
  a: for (let i = 0; i < n; i++) {
    for (let j = 0; j < n; j++) {
        if (sum >= 5) {
            sum++;
            continue a;
        }
    }
    sum--;
  }
  return sum;
}

function forNestedLoop2(n: number): number {
  let sum = 0;
a:
  for (let i = 0; i < n; i++) {
    if (i % 2 == 0) {
        for (let j = 0; j < n; j++) {
            if (sum >= 5) {
                sum++;
                continue a;
            }
        }
        return sum;
    }
    sum--;
  }
  return sum;
}

function forNestedLoop3(n: number): number {
  let sum = 0;
  let j = 0;
  for (let i = 0; i < n; i++) {
    if (i % 2 == 0) {
        while (j < n) {
            j++;
            sum++;
        }
        return sum;
    }
    sum--;
  }
  return sum;
}

let n = 10;
let ret1 = forLoop(n);
assert_equal(ret1, 10);
let ret2 = forLoopWithBreak(n);
assert_equal(ret2, -1);
let ret3 = forLoopWithContinue(n);
assert_equal(ret3, -10);
let ret4 = forNestedLoop0(n);
assert_equal(ret4, -110);
let ret5 = forNestedLoop1(n);
assert_equal(ret5, -10);
let ret6 = forNestedLoop2(n);
assert_equal(ret6, 0);
let ret7 = forNestedLoop3(n);
assert_equal(ret7, 10);
let ret8 = foo(20);
