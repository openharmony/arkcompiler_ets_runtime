/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

"use strict";

function foo1() {
  let ans = 0;
  for(let i=0;i<10;i=i+1)
  {
    ans = ans + 1;
  }
  print(ans)
}
function foo2() {
  let ans = 0;
  for(let i=0;i<=10;i=i+1)
  {
    ans = ans - 1;
  }
  print(ans)
}
function foo3() {
  let ans = 0;
  for(let i=10;i>=0;i=i-2)
  {
    ans = ans + 1;
  }
  print(ans)
}
function foo4() {
  let ans = 0;
  for(let i=10;i>0;i=i-2)
  {
    ans = ans + 1;
  }
  print(ans)
}
function foo5() {
  let ans = 0;
  for(let i=10;i>0;i=i-2)
  {
    ans = ans + 1.1;
  }
  print(ans)
}
foo1();
foo2();
foo3();
foo4();
foo5();
