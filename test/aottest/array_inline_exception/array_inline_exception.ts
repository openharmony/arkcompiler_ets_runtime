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

declare interface ArkTools {
    isAOTCompiled(args: any): boolean;
}
declare function print(arg:any):string;

let arr = [1, 2, 3]

function arrayInlineException() {
    try {
        arr.forEach(() => {throw new Error('exception')})
    } catch(e){
        print("arrayForEach exception catched success")
    }
    try {
        arr.find(() => { throw new Error('exception')})
    } catch(e){
        print("arrayFind exception catched success")
    }
    try {
        arr.findIndex(() => { throw new Error('exception')})
    } catch(e){
        print("arrayFindIndex exception catched success")
    }
    
    try {
        arr.filter(() => { throw new Error('exception')})
    } catch(e){
        print("arrayFilter exception catched success")
    }
    
    try {
        arr.map(() => { throw new Error('exception')})
    } catch(e){
        print("arrayMap exception catched success")
    }
    
    try {
        arr.some(() => { throw new Error('exception')})
    } catch(e){
        print("arraySome exception catched success")
    }
    
    try {
        arr.every(() => { throw new Error('exception')})
    } catch(e){
        print("arrayEvery exception catched success")
    }
}



arrayInlineException()
print("isAOTCompiled: " + ArkTools.isAOTCompiled(arrayInlineException))