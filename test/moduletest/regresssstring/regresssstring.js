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
const str = "Umbridge has been reading your mail, Harry."

{
  let monkey_called = false;
  str.__proto__.__proto__[Symbol.replace] =
    () => { monkey_called = true; };
  str.replace(str);
  print(monkey_called);
}

{
  let monkey_called = false;
  str.__proto__.__proto__[Symbol.search] =
    () => { monkey_called = true; };
  str.search(str);
  print(monkey_called);
}

{
  let monkey_called = false;
  str.__proto__.__proto__[Symbol.match] =
    () => { monkey_called = true; };
  str.match(str);
  print(monkey_called);
}
