/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

{
  let s = new Set([NaN]);
  assert_true(s.has(NaN));
  assert_true(s.has(-NaN));
  s.add(-NaN);
  assert_equal(s.size, 1);
  assert_true(s.delete(NaN));
  assert_equal(s.size, 0);
}

{
  let m = new Map([[NaN, "first"]]);
  assert_true(m.has(-NaN));
  assert_equal(m.get(-NaN), "first");
  m.set(-NaN, "updated");
  assert_equal(m.size, 1);
  assert_equal(m.get(NaN), "updated");
  assert_true(m.delete(-NaN));
  assert_equal(m.size, 0);
}

test_end();