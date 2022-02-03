// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// TargetApp.cpp : Example application which will be debuggeed

#include <iostream>
#include <windows.h>
#include "entity.h"

class MyClass
{
    const entity m_fileTime;
    const int m_anotherField;

public:
    MyClass(const entity& ft, int anotherField) :
        m_fileTime(ft),
        m_anotherField(anotherField)
    {
    }
};

int wmain(int argc, WCHAR* argv[])
{
    entity creationTime = { 1, 2, 3 };

    entity* pPointerTest1 = &creationTime;
    entity* pPointerTest2 = nullptr;
    MyClass c(creationTime, 12);

    entity FTZero = { 0, 0, 0 };

    __debugbreak(); // program will stop here. Evaluate 'creationTime' and 'pPointerTest' in the locals or watch window.
    std::cout << "Test complete\n";

    return 0;
}
