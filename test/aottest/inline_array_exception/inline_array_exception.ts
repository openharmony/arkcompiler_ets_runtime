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

const arr = [1,2,3,4];
try{
    arr.map(-123456);
    arr.some(-123456);
    arr.every(-123456);
    arr.forEach(-123456);
    arr.find(-123456);
    arr.findIndex(-123456);
}catch(e){}

const v0 = []
for(let i25 =  0;i25<1;i25++){
    for(let i = 0;i<1;i++){
        i25 = v0.some(()=>{});
    }
}
for(let i25 =  0;i25<1;i25++){
    for(let i = 0;i<1;i++){
        i25 = v0.pop();
    }
}
for(let i25 =  0;i25<1;i25++){
    for(let i = 0;i<1;i++){
        i25 = v0.forEach(()=>{});
    }
}
for(let i25 =  0;i25<1;i25++){
    for(let i = 0;i<1;i++){
        i25 = v0.find(()=>{});
    }
}
for(let i25 =  -1;i25<0;i25++){
    for(let i = 0;i<1;i++){
        i25 = v0.findIndex(()=>{});
    }
}
for(let i25 =  0;i25<1;i25++){
    for(let i = 0;i<1;i++){
        i25 = v0.every(()=>{});
    }
}

for(let i25 =  0;i25<1;i25++){
    for(let i = 0;i<1;i++){
        i25 = v0.sort();
    }
}
const buffer = new ArrayBuffer(16);
for(let i25 =  0;i25<1;i25++){
    for(let i = 0;i<1;i++){
        i25 = ArrayBuffer.isView(buffer);
    }
}
const map = new Map([[1, 'one'], [2, 'two']]);
for(let i25 =  0;i25<1;i25++){
    for(let i = 0;i<1;i++){
        i25 = map.keys();
    }
}
for(let i25 =  0;i25<1;i25++){
    for(let i = 0;i<1;i++){
        i25 = map.values();
    }
}

for(let i25 =  0;i25<1;i25++){
    for(let i = 0;i<1;i++){
        i25 = map.entries();
    }
}

const set = new Set([1, 2, 2, 3]);
for(let i25 =  0;i25<1;i25++){
    for(let i = 0;i<1;i++){
        i25 = set.keys();
    }
}
for(let i25 =  0;i25<1;i25++){
    for(let i = 0;i<1;i++){
        i25 = set.values();
    }
}

for(let i25 =  0;i25<1;i25++){
    for(let i = 0;i<1;i++){
        i25 = set.entries();
    }
}

print(arr.findIndex((num)=>{return num>2}));
print(arr.find((num)=>{return num>2}))