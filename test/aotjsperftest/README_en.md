# AOT JS Perf Test Temporary Execution README

## Environmental Dependencies
- ---------------------------
 ### IMPORTANT!!! -->All c_lang/ll_vm and v_8/d_8 are to avoid codecheck inspections, actually there are no underscores in them.
- --------------------------
- For the script's directory, the current user must have the permission to create folders. Please pay attention to the permissions settings for the current directory's user and group.

- openpyxl. No specific version requirement. If not installed, please execute pip3 install openpyxl.

- The environment requires downloading the full OpenHarmony code repository or standalone code repository, and compiling ark_js_vm and the frontend.
 ```shell
  #full OpenHarmony code repository
  ./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages --build-target ets_frontend_build

  #standalone code repository
  python3 ark.py x64.release
  ```

- Configuring the V_8 Engine Execution Environment

**Download：**

```shell
# you can download to any directory 
wget https://storage.googleapis.com/chromium-%76%38/official/canary/%76%38-linux64-rel-12.0.267.zip --no-check-certificate -O v_8-linux64-rel-12.0.267.zip
unzip v_8-linux64-rel-12.0.267.zip -d v_8
# don't forget to get permission to run d_8
```
**Check：**
```shell
#excute
    /path/to/your/d_8
#show
    V_8 version 12.0.267
    d_8>
```

**config.json**
```json
# Under normal circumstances, you do not need to change the configuration inside.
{
	"full_x86_64":{
		"ETS_RUNTIME_PATH": "out/rk3568/c_lang_x64/arkcompiler/ets_runtime/",
		"ICU_PATH": "out/rk3568/c_lang_x64/thirdparty/icu/",
		"ZLIB_PATH": "out/rk3568/c_lang_x64/thirdparty/zlib/", 
		"LIB_PATH": "prebuilts/c_lang/ohos/linux-x86_64/ll_vm/lib/",
		"ES2ABC": "out/rk3568/c_lang_x64/arkcompiler/ets_frontend/es2abc"
	},
	"independent_x86_64": {
		"ETS_RUNTIME_PATH": "out/x64.release/arkcompiler/ets_runtime",
		"ICU_PATH": "out/x64.release/thirdparty/icu/",
		"ZLIB_PATH": "", # you can let this value be empty,but dont delete this key
		"LIB_PATH": "", # you can let this value be empty,but dont delete this key
		"ES2ABC": "out/x64.release/arkcompiler/ets_frontend/es2abc"
	}	
}
```

## excute
1) How to run perftest
```shell
# openharmony_path is the code repository root directory (which has out/ after build)
bash run_js_perf_test.sh js_report_save_path openharmony_path d_8_binary_path ver_platform
#example full OpenHarmony code repository
bash run_js_perf_test.sh /home/report /home/codeOpenharmony /home/v_8/d_8 full_x86_64
#example standalone code repository
bash run_js_perf_test.sh /home/report /home/standalone /home/v_8/d_8 independent_x86_64
```
2) When aot perf test start to run,the perf test cases will be downloaded automatically .
But after run, perftest case will be deleted too.

u can check them in: 
[perf-test-case](https://gitee.com/dov1s/arkjs-perf-test/tree/builtins_test1110/)

if u dont want the script deletes test cases,please command out this line in run_js_test.py 
```python
# last line 
shutil.rmtree(Constants.TMP_PATH)
```

## stat
After running, a excel will be created in js_report_save_path(the directory u choose when bash run_js_perf_test.sh)



