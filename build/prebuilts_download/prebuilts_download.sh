#!/bin/bash
# Copyright (c) 2022 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
set -e
while [ $# -gt 0 ]; do
  case "$1" in
    -skip-ssl|--skip-ssl) # wget、npm skip ssl check, which will allow
                          # hacker to get and modify data stream between server and client!
    SKIP_SSL=YES
    ;;
    -h|--help)
    HELP=YES
    ;;
    --tool-repo)
    TOOL_REPO="$2"
    shift
    ;;
    --tool-repo=*)
    TOOL_REPO="${1#--tool-repo=}"
    ;;
    --npm-registry)
    NPM_REGISTRY="$2"
    shift
    ;;
    --npm-registry=*)
    NPM_REGISTRY="${1#--npm-registry=}"
    ;;
    --trusted-host)
    TRUSTED_HOST="$2"
    shift
    ;;
    --trusted-host=*)
    TRUSTED_HOST="${1#--trusted-host=}"
    ;;
    --pypi-url)           # python package index url
    PYPI_URL="$2"
    shift
    ;;
    --pypi-url=*)
    PYPI_URL="${1#--pypi-url=}"
    ;;
    *)
    echo "$0: Warning: unsupported parameter: $1" >&2
    ;;
  esac
  shift
done

case $(uname -s) in
    Linux)

        host_platform=linux
        ;;
    Darwin)
        host_platform=darwin
        ;;
    *)
        echo "Unsupported host platform: $(uname -s)"
        exit 1
esac

case $(uname -m) in
    arm64)

        host_cpu=arm64
        ;;
    *)
        host_cpu=x86_64
esac

if [ "X${SKIP_SSL}" == "XYES" ];then
    wget_ssl_check="--skip-ssl"
else
    wget_ssl_check=''
fi

if [ "X${HELP}" == "XYES" ];then
    help="-h"
else
    help=''
fi

if [ ! -z "$TOOL_REPO" ];then
    tool_repo="--tool-repo $TOOL_REPO"
else
    tool_repo=''
fi

if [ ! -z "$NPM_REGISTRY" ];then
    npm_registry="--npm-registry $NPM_REGISTRY"
else
    npm_registry=''
fi

if [ ! -z "$TRUSTED_HOST" ];then
    trusted_host=$TRUSTED_HOST
elif [ ! -z "$PYPI_URL" ];then
    trusted_host=${PYPI_URL/#*:\/\//}       # remove prefix part such as http:// https:// etc.
    trusted_host=${trusted_host/%[:\/]*/}   # remove suffix part including the port number
else
    trusted_host='repo.huaweicloud.com'
fi

if [ ! -z "$PYPI_URL" ];then
    pypi_url=$PYPI_URL
else
    pypi_url='http://repo.huaweicloud.com/repository/pypi/simple'
fi

if [ $UID -ne 0 ]; then
    npm_para=''
else
    npm_para='--unsafe-perm'
fi

cpu="--host-cpu $host_cpu"
platform="--host-platform $host_platform"

script_path=$(cd $(dirname $0);pwd)
code_dir=$(dirname ${script_path})
pip3 install --trusted-host $trusted_host -i $pypi_url rich
echo "prebuilts_download start"
python3 "${code_dir}/prebuilts_download/prebuilts_download.py" $wget_ssl_check $tool_repo $npm_registry $help $cpu $platform $npm_para
echo "prebuilts_download end"

echo -e "\n"
