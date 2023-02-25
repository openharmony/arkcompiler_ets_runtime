/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

declare function AssertType(value:any, type:string):void;
{
    function bs(array:number[], value:number, ep:number = 1e-6) : number {
        let low = 0;
        let high = array.length - 1;
        let middle = high >>> 1;
        for(;low <= high; middle = (low + high) >>> 1){
            const test = array[middle];
            AssertType(low, "int");
            AssertType(high, "int");
            AssertType(middle, "int");
            if (test > (value + ep)) {
                high = middle - 1;
            } else if (test < (value - ep)) {
                low = middle + 1;
            } else {
                return middle;
            }
            AssertType(high, "int");
            AssertType(low, "int");
        }
        return  ~low;
    }    
}