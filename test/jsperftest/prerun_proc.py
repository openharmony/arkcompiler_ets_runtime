#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Copyright (c) 2024 Huawei Device Co., Ltd.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Description: Use ark to execute workload test suite
"""

import sys
import re
import os

HELP = """
python prerun_proc.py __VAR__=value ... default_js_scr.js

lets assume that default_js_scr.js have next lines

```
    const __MULTIPLIER__ = 1
    let start1 = new Date().getTime();
    for (let i = 0; i < __MULTIPLIER__ * 10000; i++) {
        SetAddSmi();
    }
    print("__MULTIPLIER__ = " + __MULTIPLIER__)
```

After execution
```
python prerun_proc.py __MULTIPLIER__=3 ... default_js_scr.js
```

you will see in default_js_scr.js
```
    const __MULTIPLIER__ = 3
    let start1 = new Date().getTime();
    for (let i = 0; i < __MULTIPLIER__ * 10000; i++) {
        SetAddSmi();
    }
    print("__MULTIPLIER__ = " + __MULTIPLIER__)
```

So after executing you will see next line:
```
__MULTIPLIER__ = 3
```

"""


def get_args():
    vars_to_replace = {}
    script_name = ""

    for arg in sys.argv[1:]:
        if(re.search("^__.*__=.*$", arg)):
            (var, val) = arg.split('=')
            vars_to_replace[var] = val
        elif(re.search("^.*.js$", arg)):
            if(script_name != ""):
                raise RuntimeError("More then on script to preprocess")
            else:
                script_name = arg
        elif(arg == "-h" or arg == "--help"):
            print(HELP)
            exit(0)
        else:
            raise RuntimeError(f"{arg} is wrong argument, please look at help")

    if(script_name == ""):
        raise RuntimeError("There is no script to preprocess")

    if(len(vars_to_replace) == 0):
        raise RuntimeError("No vars to replace")

    return script_name, vars_to_replace


def main():
    script_name, vars_to_replace = get_args()
    data = []

    status = os.stat(script_name)
    fd = os.open(script_name, os.O_RDWR | os.O_CREAT, status.st_mode)
    with os.fdopen(fd, 'r') as script_file:
        data = script_file.readlines()

    for i, _ in enumerate(data):
        for var in vars_to_replace.items():
            if(re.search(f"const {var} =", data[i].strip())):
                val = data.get(i)
                data[i] = val[:val.find("=") + 1] + ' ' + vars_to_replace.get(var) + ';\n'

    fd = os.open(script_name, os.O_RDWR | os.O_CREAT, status.st_mode)
    with os.fdopen(fd, 'w') as script_file:
        script_file.seek(0)
        script_file.writelines(data)
        script_file.truncate()

if (__name__ == "__main__"):
    main()