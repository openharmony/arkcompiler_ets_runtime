/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an 'AS IS' BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

const SEEDINIT: number = 49734321;
const TIME_CONVERSION: number = 1000;
const HEX_FFFFFFFF: number = 0xffffffff;
const HEX_FFFFFFF: number = 0xfffffff;
const HEX_7ED55D16: number = 0x7ed55d16;
const HEX_C761C23C: number = 0xc761c23c;
const HEX_165667B1: number = 0x165667b1;
const HEX_D3A2646C: number = 0xd3a2646c;
const HEX_FD7046C5: number = 0xfd7046c5;
const HEX_B55A4F09: number = 0xb55a4f09;
const HEX_10000000: number = 0x10000000;
const INT_12: number = 12;
const INT_19: number = 19;
const INT_5: number = 5;
const INT_9: number = 9;
const INT_3: number = 3;
const INT_16: number = 16;
const RANDOM_MULTIPLY: number = 128;
const RANDOM_ADD: number = 2056;
const RESULT_MULTIPLY: number = 255;
const ITEM_INDEX: number = 2;
const DIRS_ALL: number = 250;
const DIRS_SON: number = 8;
const FILES_COUNT: number = 5;
const DIR_RANDOM: number = 0.3;
const FILE_RANDOM: number = 0.6;
const FILE_DELETE_COUNT: number = 3;
const TEST_TIMES: number = 10;

let seedNum = SEEDINIT;
function resetSeed(): void {
  seedNum = SEEDINIT;
}
const random = ((): (() => number) => {
  return () => {
    // Robert Jenkins' 32 bit integer hash function.
    seedNum = (seedNum + HEX_7ED55D16 + (seedNum << INT_12)) & HEX_FFFFFFFF;
    seedNum = (seedNum ^ HEX_C761C23C ^ (seedNum >>> INT_19)) & HEX_FFFFFFFF;
    seedNum = (seedNum + HEX_165667B1 + (seedNum << INT_5)) & HEX_FFFFFFFF;
    seedNum = ((seedNum + HEX_D3A2646C) ^ (seedNum << INT_9)) & HEX_FFFFFFFF;
    seedNum = (seedNum + HEX_FD7046C5 + (seedNum << INT_3)) & HEX_FFFFFFFF;
    seedNum = (seedNum ^ HEX_B55A4F09 ^ (seedNum >>> INT_16)) & HEX_FFFFFFFF;
    return (seedNum & HEX_FFFFFFF) / HEX_10000000;
  };
})();

function randomFileContents(bytes: number = ((random() * RANDOM_MULTIPLY) >>> 0) + RANDOM_ADD): Uint8Array {
  let result = new Uint8Array(bytes);
  for (let i = 0; i < bytes; i++) {
    result[i] = (random() * RESULT_MULTIPLY) >>> 0;
  }
  return result;
}

class File {
  private _data: Uint8Array;
  constructor(dataView: Uint8Array) {
    this._data = dataView;
  }
  get data(): Uint8Array {
    return this._data;
  }

  set data(dataView: Uint8Array) {
    this._data = dataView;
  }

  swapByteOrder(): void {
    for (let i = 0; i < this.data.length; i++) {
      this.data[i] = this.data[this.data.byteLength - 1 - i];
    }
  }
}

class Directory {
  structure: Map<string, Directory | File>;
  constructor() {
    this.structure = new Map<string, Directory | File>();
  }

  async addFile(name: string, file: File): Promise<File> {
    let entryFile = this.structure.get(name);
    if (entryFile !== undefined) {
      if (entryFile instanceof File) {
        throw new Error('Can\'t replace file with file;');
      }
      if (entryFile instanceof Directory) {
        throw new Error('Can\'t replace a file with a new directory;');
      }
      throw new Error('Should not reach this code;');
    }

    this.structure.set(name, file);
    return file;
  }

  async addDirectory(name: string, directory: Directory = new Directory()): Promise<Directory> {
    let entryFile = this.structure.get(name);
    if (entryFile !== undefined) {
      if (entryFile instanceof File) {
        throw new Error('Can\'t replace file with directory;');
      }
      if (entryFile instanceof Directory) {
        throw new Error('Can\'t replace directory with new directory;');
      }
      throw new Error('Should not reach this code;');
    }

    this.structure.set(name, directory);
    return directory;
  }

  async ls(): Promise<[[name: string, entry: Directory | File, isDirectory: boolean]]> {
    let result: [[name: string, entry: Directory | File, isDirectory: boolean]] = [['', new Directory(), false]];
    result.shift();
    for (let item of this.structure) {
      result.push([item[0], item[1], item[1] instanceof Directory]);
    }
    return result;
  }

  async forEachFile(): Promise<[[name: string, entry: Directory | File, isDirectory: boolean]]> {
    let result: [[name: string, entry: Directory | File, isDirectory: boolean]] = [['', new Directory(), false]];
    result.shift();
    for (let item of await this.ls()) {
      if (!item[ITEM_INDEX]) {
        result.push(item);
      }
    }
    return result;
  }

  async forEachFileRecursively(): Promise<[[name: string, entry: Directory | File, isDirectory: boolean]]> {
    let result: [[name: string, entry: Directory | File, isDirectory: boolean]] = [['', new Directory(), false]];
    result.shift();
    for (let item of await this.ls()) {
      if (item[ITEM_INDEX]) {
        for (let file of await (item[1] as Directory).forEachFileRecursively()) {
          result.push(file);
        }
      } else {
        result.push(item);
      }
    }
    return result;
  }

  async forEachDirectoryRecursively(): Promise<[[name: string, entry: Directory | File, isDirectory: boolean]]> {
    let result: [[name: string, entry: Directory | File, isDirectory: boolean]] = [['', new Directory(), false]];
    result.shift();
    for (let item of await this.ls()) {
      if (!item[ITEM_INDEX]) {
        continue;
      }
      for (let dirItem of await (item[1] as Directory).forEachDirectoryRecursively()) {
        result.push(dirItem);
      }
      result.push(item);
    }
    return result;
  }

  async fileCount(): Promise<number> {
    let count = 0;
    for (let item of await this.ls()) {
      if (!item[ITEM_INDEX]) {
        count += 1;
      }
    }
    return count;
  }

  async rm(name: string): Promise<boolean> {
    return this.structure.delete(name);
  }
}

async function setupDirectory(): Promise<Directory> {
  const fs = new Directory();
  let dirs = [fs];

  for (let index = 0; index < DIRS_ALL; index++) {
    let dir = dirs[index];
    for (let i = 0; i < DIRS_SON; ++i) {
      if (dirs.length < DIRS_ALL && random() >= DIR_RANDOM) {
        dirs.push(await dir.addDirectory(`dir-${i}`));
      }
    }
  }

  for (let dir of dirs) {
    for (let i = 0; i < FILES_COUNT; ++i) {
      if (random() >= FILE_RANDOM) {
        await dir.addFile(`file-${i}`, new File(randomFileContents()));
      }
    }
  }

  return fs;
}
/*
 * @State
 * @Tags Jetstream2
 */
class Benchmark {
  /*
   *@Benchmark
   */
  async runIteration(): Promise<void> {
    resetSeed();
    const fs = await setupDirectory();
    if (isDebug) {
      let dirs = await fs.forEachDirectoryRecursively();
      let files = await fs.forEachFileRecursively();
      //printLog('根节点fs下所有dir的数量====='+dirs.length)
      //printLog('根节点fs下所有dir下的file数量====='+files.length)
    }
    for (let file of await fs.forEachFileRecursively()) {
      (file[1] as File).swapByteOrder();
    }

    for (let item of await fs.forEachDirectoryRecursively()) {
      let dir = item[1] as Directory;
      if ((await dir.fileCount()) > FILE_DELETE_COUNT) {
        if (isDebug) {
          let deles = await dir.fileCount();
          //printLog('')
          //printLog('dir对象删除的file数量====='+deles)
        }
        for (let name of await dir.forEachFile()) {
          let result = await dir.rm(name[0]);
          if (!result) {
            throw new Error('rm should have returned true');
          }
        }
      }
    }
  }
}

//以下是测试打印日志相关代码
const isDebug = false;
function printLog(str: string): void {
  print(str);
}

async function addLoop(times: number, ben: Benchmark = new Benchmark()): Promise<void> {
  let start = ArkTools.timeInUs() / TIME_CONVERSION;
  for (let i = 0; i < times; i++) {
    await ben.runIteration();
  }
  let end = ArkTools.timeInUs() / TIME_CONVERSION;
  print('file-system: ms = ' + (end - start).toString());
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  addLoop(TEST_TIMES);
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

addLoop(TEST_TIMES);
