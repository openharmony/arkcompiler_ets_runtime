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
declare interface ArkTools{
  timeInUs(arg:any):number
}
function picColorFromBuffer(e: EData){
  let data = e.data
  if (!data || !data.data) {
    console.log('dingwen input error')
    return
  }
  let msgType: string = data.type
  let readBuffer: ArrayBuffer = data.data
  let len: number = data.len / 4
  switch (msgType) {
    case 'pickColor':
      let intBuffer: Uint8Array = new Uint8Array(readBuffer)
      let r: number = 0
      let g: number = 0
      let b: number = 0
      let a: number = 0
      for (var i = 0; i < len; i++) {
        r += intBuffer[i * 4]
        g += intBuffer[i * 4 + 1]
        b += intBuffer[i * 4 + 2]
        a += intBuffer[i * 4 + 3]
      }
      r = Math.round(r / len)
      g = Math.round(g / len)
      b = Math.round(b / len)
      a = Math.round(a / len)
    // 	  print("Takecolor pinColorFormBuffer color[ARGB] :\t" + a + "\t" + r + "\t" + g + "\t" + b);
      break
    default:
      break
  }
}

class Data {
  type: string;
  data: ArrayBuffer;
  len: number;

  constructor(mType: string, mData: ArrayBuffer, mLen: number) {
    this.type = mType;
    this.data = mData;
    this.len = mLen;
  }
}

class EData {
  data: Data;

  constructor(mData: Data) {
    this.data = mData;
  }
}

function main() {
  let readBuffer: ArrayBuffer = new ArrayBuffer(500*500);
  let uint8Array: Uint8Array = new Uint8Array(readBuffer);
  for (let i:number = 0; i < uint8Array.length; i++) {
    uint8Array[i] = i % 256;
  }
  let data: Data = new Data('pickColor', readBuffer, 500*500);
  let eData: EData = new EData(data);
  const start: number = ArkTools.timeInUs();
  for (let i:number=0;i<10;i++){
    picColorFromBuffer(eData);
  }
  const end: number = ArkTools.timeInUs();
  print("Takecolor_Obj: " +( (end - start)/1000) + "\tms");
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  main()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

main()
