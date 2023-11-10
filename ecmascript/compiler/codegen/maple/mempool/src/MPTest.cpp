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

#include <iostream>
#include "mempool.h"
#include "mempool_allocator.h"
#include "maple_string.h"
#include "mpl_logging.h"
#include "securec.h"
using namespace maple;
struct Structure {
    int id;
    float height;
};

class MyClass {
public:
    MyClass(int currId, const std::string &currName)
    {
        id = currId;
        name = currName;
    };
    ~MyClass() {};
    int id;
    std::string name;
};

void TestLocalAllocater()
{
    MemPoolCtrler mpc;
    auto mp = std::unique_ptr<StackMemPool>(new StackMemPool(mpc, ""));
    LocalMapleAllocator alloc1(*mp);
    MapleVector<int> v1({1, 2, 3, 4, 5}, alloc1.Adapter());
    {
        LocalMapleAllocator alloc2(*mp);
        MapleVector<int> v2({1, 2, 3, 4, 5}, alloc2.Adapter());
        {
            LocalMapleAllocator alloc3(*mp);
            MapleVector<int> v3({1, 2, 3, 4, 5}, alloc3.Adapter());
        }
    }
}

int main()
{
    // 1. Create a memory pool controler instance;
    MemPoolCtrler mpc;
    // 2. Create two memory pools on mpc
    MemPool *mp1 = mpc.NewMemPool("Test Memory Pool 1", true /* isLocalPool */);
    MemPool *mp2 = mpc.NewMemPool("Test Memory Pool 2", true /* isLocalPool */);
    // 3. Usage of memory pool, Malloc/Call on primitive types
    // char string
    constexpr int lengthOfHelloWorld = 12;
    char *charP = static_cast<char *>(mp1->Malloc(lengthOfHelloWorld * sizeof(char)));
    errno_t cpyRes = strcpy_s(charP, lengthOfHelloWorld, "Hello world");
    if (cpyRes != 0) {
        LogInfo::MapleLogger() << "call strcpy_s failed" << std::endl;
        return 0;
    }
    LogInfo::MapleLogger() << charP << std::endl;
    // int, float, double
    constexpr int arrayLen = 10;
    int *intP = static_cast<int *>(mp1->Calloc(arrayLen * sizeof(int)));
    CHECK_FATAL(intP, "null ptr check  ");
    for (int i = 0; i < arrayLen; ++i) {
        intP[i] = i * i;
    }
    for (int i = 0; i < arrayLen - 1; ++i) {
        LogInfo::MapleLogger() << intP[i] << " ,";
    }
    LogInfo::MapleLogger() << intP[arrayLen - 1] << std::endl;
    float *floatP = static_cast<float *>(mp1->Malloc(arrayLen * sizeof(float)));
    for (int i = 0; i < arrayLen; ++i) {
        floatP[i] = 10.0 / (i + 1);  // test float num is 10.0
    }
    for (int i = 0; i < arrayLen - 1; ++i) {
        LogInfo::MapleLogger() << floatP[i] << " ,";
    }
    LogInfo::MapleLogger() << floatP[arrayLen - 1] << std::endl;
    // 4. Allocate memory on struct
    Structure *structP = mp1->New<Structure>();
    structP->height = 1024;  // test num is 1024.
    structP->id = 0;
    LogInfo::MapleLogger() << "Structure: my_struct height=" << structP->height << " feet,"
                           << "id=    x" << structP->id << std::endl;
    // 5. Allocate memory on class constructor
    MyClass *myClass = mp1->New<MyClass>(1, "class name");
    LogInfo::MapleLogger() << "Class: my_class id=" << myClass->id << " , name=" << myClass->name << std::endl;
    // 7. Memory Pool supports std library such list, vector, string, map, set.
    // using mempool
    MapleAllocator mpAllocator(mp2);
    // vector
    MapleVector<int> myVector(mpAllocator.Adapter());
    for (int i = 0; i < arrayLen; ++i) {
        myVector.push_back(i * i * i);
    }
    MapleVector<int>::iterator itr;
    for (itr = myVector.begin(); itr != myVector.end(); ++itr) {
        LogInfo::MapleLogger() << *itr << " ,";
    }
    LogInfo::MapleLogger() << std::endl;
    // stack
    MapleQueue<int> myQueue(mpAllocator.Adapter());
    myQueue.push_back(1);
    myQueue.push_back(2);
    myQueue.push_back(3);
    LogInfo::MapleLogger() << "Queue front is" << myQueue.front() << std::endl;
    LogInfo::MapleLogger() << "Queue back is" << myQueue.back() << std::endl;
    myQueue.pop_back();
    LogInfo::MapleLogger() << "after pop() vector top is" << myQueue.back() << std::endl;
    // String
    MapleString myString(mp2);
    myString = "Using my mempool";
    myString += " example";
    LogInfo::MapleLogger() << myString << std::endl;
    // list
    MapleList<float> myList(mpAllocator.Adapter());
    for (int i = 0; i < arrayLen; ++i) {
        myList.push_back(1000.0 / (i + 1));  // test float num is 1000.0
    }
    MapleList<float>::iterator listItr;
    for (listItr = myList.begin(); listItr != myList.end(); ++listItr) {
        LogInfo::MapleLogger() << *listItr << " ,";
    }
    LogInfo::MapleLogger() << std::endl;
    // Map
    MapleMap<int, MapleString> myMap(std::less<int>(), mpAllocator.Adapter());
    for (int i = 0; i < arrayLen; ++i) {
        MapleString temp(mp2);
        temp += std::to_string(i);
        temp += " value";
        myMap.insert(std::pair<int, MapleString>(i, temp));
    }
    MapleMap<int, MapleString>::iterator mapItr;
    for (mapItr = myMap.begin(); mapItr != myMap.end(); ++mapItr) {
        LogInfo::MapleLogger() << "key= " << mapItr->first << ", value=" << mapItr->second << std::endl;
    }
    // Set
    MapleSet<MapleString> mySet(std::less<MapleString>(), mpAllocator.Adapter());
    for (int i = 0; i < arrayLen; ++i) {
        MapleString temp(mp2);
        temp += std::to_string(i * i);
        temp.append(" set values");
        mySet.insert(temp);
    }
    MapleSet<MapleString>::iterator setItr;
    for (setItr = mySet.begin(); setItr != mySet.end(); ++setItr) {
        LogInfo::MapleLogger() << "set value =" << *setItr << std::endl;
    }
    // Delete memory pool
    delete mp1;
    delete mp2;

    TestLocalAllocater();
    return 1;
}
