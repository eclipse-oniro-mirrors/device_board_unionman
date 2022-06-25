#!/bin/bash

# Copyright (c) 2022 Unionman Technology Co., Ltd.
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

echo "Build kernel..."
set -e
export PRODUCT_COMPANY=unionman
export DEVICE_NAME=unionpi_tiger
export KERNEL_OBJ_PATH=$2
export IMAGES_PATH=$3
export DEVICE_PATH=$4
export PRODUCT_PATH=$5
export RAMDISK_ENABLE=$6
mkdir -p ${2}
mkdir -p ${3}
make -f ${4}/kernel/build/kernel.mk
