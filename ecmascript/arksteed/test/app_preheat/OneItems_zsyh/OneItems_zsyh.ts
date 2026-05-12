/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

const target_log = ",workload,"
class OneItem {
  public type1: number = 0;
  public value: Int8Array = new Int8Array(0);
}

class Items {
  public static test(){
    const items = new Items();
    for (let i = 1; i < 20; i++) {
      const item = new OneItem();
      item.type1 = i;
      item.value = new Int8Array(i * 6);
      items.addItem(item);
    }
    let time1 = new Date().getTime();
    let originBytes = new Int8Array();
    for(let i=0;i< 10000;i++) {
      originBytes = items.toBytes();
    }

    print(`oneItem_zsyh_toBytes: ${new Date().getTime() - time1} ms`);
    time1 = new Date().getTime();
    for(let i=0;i< 10000;i++) {
      const items2 = new Items();
      items2.parseFromBytes(originBytes);
    }
    print(`oneItem_zsyh_parseFromBytes: ${new Date().getTime() - time1} ms`);

  }
  public items: OneItem[] = [];
  public itemMap: Map<number, OneItem> = new Map();

  public addItem(item: OneItem) {
    this.items.push(item);
    this.itemMap.set(item.type1, item);
  }

  public toBytes(): Int8Array {
    let msgSize = 0;
    this.itemMap.forEach((value) => {
      msgSize = msgSize + 2 + value.value.length;
    });
    let startIndex = 0;
    const buffer = new Int8Array(msgSize);
    this.itemMap.forEach((item) => {
      const len = item.value.length;
      buffer[startIndex++] = item.type1;
      buffer[startIndex++] = len;
      if (len > 0) {
        buffer.set(item.value, startIndex);
        startIndex += len;
      }
    });
    return buffer;
  }

  public parseFromBytes(values: Int8Array): void {
    const length = values.length;
    let pos = 0;
    let oneItem: OneItem = new OneItem();
    while(pos < length) {
      oneItem = new OneItem();
      oneItem.type1 = values[pos++];
      const len = values[pos++];
      oneItem.value = values.slice(pos, pos + len);
      pos = pos + len;
      this.items.push(oneItem);
      this.itemMap.set(oneItem.type1, oneItem);
    }
  }
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  Items.test()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

Items.test();
