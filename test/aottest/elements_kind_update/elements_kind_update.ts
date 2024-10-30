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

interface ArkTools{
  getElementsKind(obj:any);
}
class EventBusClass {
  public result: Object[] = [];
  public holeIntArray: Object[] = [0, 1, 2, , 4, 5, 6];
  public addEventListener(): void {
      this.result.push({});
  }

  public removeEventListener(): void {
      this.result = [];
  }
}

let EventBus = new EventBusClass();
function main(): void {
  EventBus.addEventListener();
  for (let index = 0; index < 500000; index++) {
      EventBus.removeEventListener();
  }

  for (let index = 0; index < 50; index++) {
      EventBus.addEventListener();
      EventBus.removeEventListener();
      print(ArkTools.getElementsKind(EventBus.result))
  }
}

main();

function testToReversed() {
    let c = new EventBusClass();
    let b = c.holeIntArray.toReversed();
    // when pgo profiler is enabled, but elementsKind are not enabled, the array created in builtins are generic.
    if (b.length == 7 && ArkTools.getElementsKind(b) == 31) {
        print("testToReversed - success");
    } else {
        print("testToReversed - failed, expected: " + 31 + " , but get: " + ArkTools.getElementsKind(b));
    }
}

testToReversed()