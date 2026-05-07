/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
'use strict';

export class File {
  fileName: string;
  data: string;
  static directory = new Map<string, File>();

  constructor(fileNames: string, datas: string) {
    this.fileName = fileNames;
    this.data = datas;
    if (File.directory.get(fileNames) !== undefined) {
      throw new Error('Creating file named ' + this.fileName + ' that already exists');
    } else {
      File.directory.set(fileNames,this);
    }
  }

  static open(fileName: string): File {
    return File.directory.get(fileName)!;
  }

  read(): string {
    return this.data;
  }
}
