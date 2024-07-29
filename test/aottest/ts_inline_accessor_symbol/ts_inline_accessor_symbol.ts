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

declare var ArkTools:any;
declare function print(arg:any):string;
class Code {
    _bases: Base[] = new Array();
  
    addBase(): Base {
      let base = new Base();
      let result = base;
      this._bases.push(result);
      return result;
    }
  
    hash() {
      this._bases[0].successorBases()
    }
  }
  
class Base {
  _successors = []
    
  successorBases()  {
    return this.successors;
  }

  get successors(){
    return this._successors;
  }
  
  [Symbol.iterator]() {
    return 1
  }
}
  
function createTest(): Code {
  let code =  new Code();
  let bb0 = code.addBase();
  let bb1 = code.addBase();
  return code
}
  
function runIteration(): void {
  let code = createTest()
  code.hash()
}
  
let i;
for (i = 0; i < 10; i++) {
  runIteration();
}
print(i)

print(ArkTools.isAOTCompiled(createTest))
print(ArkTools.isAOTCompiled(runIteration))