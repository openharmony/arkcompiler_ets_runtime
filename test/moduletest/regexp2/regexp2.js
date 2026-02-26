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

/*
 * @tc.name:regexp
 * @tc.desc:test Regexp
 * @tc.type: FUNC
 * @tc.require: issue11945
 */
{
  // Case 1: Global regex alternating between cache hits and misses
  const regex = /\d+/g;
  const results = [];
  
  // First use - should hit cache (fresh state)
  results.push(regex.test("abc123def"));
  
  // Second use - should miss cache (lastIndex changed)
  results.push(regex.test("abc123def"));
  
  // Third use - reset and hit cache again
  regex.lastIndex = 0;
  results.push(regex.test("abc123def"));
  
  assert_equal(results[0], true);
  assert_equal(results[1], false);
  assert_equal(results[2], true);
}

{
  // Case 2: Sticky flag with alternating positions
  const regex = /hello/y;
  const str = "hello world hello";
  const results = [];
  
  // First match at position 0
  regex.lastIndex = 0;
  results.push(regex.test(str));
  
  // Should fail at position 5
  results.push(regex.test(str));
  
  // Should succeed at position 12
  regex.lastIndex = 12;
  results.push(regex.test(str));
  
  assert_equal(results[0], true);
  assert_equal(results[1], false);
  assert_equal(results[2], true);
}

{
  // Case 3: Multiple regex objects with same pattern but different state
  const regex1 = /\d+/g;
  const regex2 = /\d+/g;
  const str = "123 456";
  
  const results = [];
  results.push(regex1.test(str)); // true, lastIndex=3
  results.push(regex2.test(str)); // true, lastIndex=3 (fresh regex)
  results.push(regex1.test(str)); // true, lastIndex=7 (continues from 3)
  results.push(regex2.test(str)); // true, lastIndex=7 (continues from 3)
  
  assert_equal(results[0], true);
  assert_equal(results[1], true);
  assert_equal(results[2], true);
  assert_equal(results[3], true);
}

test_end();