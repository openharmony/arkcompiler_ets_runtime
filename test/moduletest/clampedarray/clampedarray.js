/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
var clampedArray = new Uint8ClampedArray(10);

function test() {
  clampedArray[0] = 0.499;
  print(0 == clampedArray[0]);
  clampedArray[0] = 0.5;
  print(0 == clampedArray[0]);
  clampedArray[0] = 0.501;
  print(1 == clampedArray[0]);
  clampedArray[0] = 1.499;
  print(1 == clampedArray[0]);
  clampedArray[0] = 1.5;
  print(2 == clampedArray[0]);
  clampedArray[0] = 1.501;
  print(2 == clampedArray[0]);
  clampedArray[0] = 2.5;
  print(2 == clampedArray[0]);
  clampedArray[0] = 3.5;
  print(4 == clampedArray[0]);
  clampedArray[0] = 252.5;
  print(252 == clampedArray[0]);
  clampedArray[0] = 253.5;
  print(254 == clampedArray[0]);
  clampedArray[0] = 254.5;
  print(254 == clampedArray[0]);
  clampedArray[0] = 256.5;
  print(255 == clampedArray[0]);
  clampedArray[0] = -0.5;
  print(0 == clampedArray[0]);
  clampedArray[0] = -1.5;
  print(0 == clampedArray[0]);
  clampedArray[0] = 1000000000000;
  print(255 == clampedArray[0]);
  clampedArray[0] = -1000000000000;
  print(0 == clampedArray[0]);
};
ArkTools.prepareFunctionForOptimization(test);
test();
test();
ArkTools.optimizeFunctionOnNextCall(test);
test();