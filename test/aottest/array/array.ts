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

declare function print(arg:any):string;
{
  let a = []
  let l = a.push(1)
  print(l)
  l = a.push(1, 2, 3, 4, 5)
  print(l)

  for (let i = 0; i < 100; i++) {
    a.push(i)
  }

  let c = [1, 2, 3, 4]
  a.push(...c)

  print(a.length)

  let b = []
  b.push(1, 2, 3, 4)
  b.push(1, 2, 3)
  b.push(1, 2)
  b.push(1)
  b.push()
  print(Object.values(b))
  print(b.length)
}

{
  let result;
  let array = new Array();
  let array_size = 100;

  for (let i = 0; i < array_size; i++) {
    array[i] = i;
  }

  result = array.sort((a, b) => {
    return a - b
  });

  print(result);

  result = array.sort();
  print(result);
}

{
  const o24 = {
    "mmaxByteLength": 268435456,
  };
  const v25 = new SharedArrayBuffer(4, o24);
  const v27 = new Uint32Array(v25);
  v27[0] = 2051996997;
  const v30 = v27.fill(o24, 4, 4);
  v30[0] = -36814;
  const v32 = new Float32Array(v25);
  let v33 = v32[0];
  print(v33);
}

{
  const BASE_NAN_VALUE = NaN;
  function simulateFloatConvert(value, isFloat32) {
    const float64Value = isFloat32 ?
      new Float64Array(Float64Array.from(Float32Array.of(value)).buffer)[0] : value;
    return isNaN(float64Value) ? BASE_NAN_VALUE : float64Value;
  }
  print(simulateFloatConvert(NaN, false));
}
