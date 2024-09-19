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

function func1() {
  if (decodeURIComponent("") !== "") {
    return false;
  }
  ArkTools.printTypedOpProfiler("TYPED_CALL_BUILTIN");
  ArkTools.clearTypedOpProfiler();
  return true;
}

function func2() {
  if (decodeURIComponent("http:%2f%2Fwww.runoob.ru/support/jobs/bin/static.py%3Fpage%3dwhy-ru.html%26sid%3Dliveandwork")
    !== "http://www.runoob.ru/support/jobs/bin/static.py?page=why-ru.html&sid=liveandwork") {
      return false;
    }
    ArkTools.printTypedOpProfiler("TYPED_CALL_BUILTIN");
    ArkTools.clearTypedOpProfiler();
    return true;
}

// tree string
function func3() {
  let s1 = "http:%2f%2Fwww.runoob.ru/support/jobs/bin/static.py";
  let s2 = "%3Fpage%3dwhy-ru.html%26sid%3Dliveandwork";
  let uri = s1.concat(s2);
  if (decodeURIComponent(uri) !== "http://www.runoob.ru/support/jobs/bin/static.py?page=why-ru.html&sid=liveandwork") {
    return false;
  }
  ArkTools.printTypedOpProfiler("TYPED_CALL_BUILTIN");
  ArkTools.clearTypedOpProfiler();
  return true;
}

// sliced string
function func4() {
  let string = "http:%2f%2Fwww.runoob.ru/support/jobs/bin/static.py%3Fpage%3dwhy-ru.html%26sid%3Dliveandwork";
  let uri = string.substring(0, 24);
  if (decodeURIComponent(uri) !== "http://www.runoob.ru") {
    return false;
  }
  ArkTools.printTypedOpProfiler("TYPED_CALL_BUILTIN");
  ArkTools.clearTypedOpProfiler();
  return true;
}

// utf16
function func5() {
  let uri = "http:%2f%2Fwww.runoob好.ru/support/jobs/bin/static.py%3Fpage%3dwhy-ru.html%26sid%3Dliveandwork";
  if (decodeURIComponent(uri) !==
    "http://www.runoob好.ru/support/jobs/bin/static.py?page=why-ru.html&sid=liveandwork") {
    return false;
  }
  ArkTools.printTypedOpProfiler("TYPED_CALL_BUILTIN");
  ArkTools.clearTypedOpProfiler();
  return true;
}

print(func1())
print(func2())
print(func3())
print(func4())
print(func5())