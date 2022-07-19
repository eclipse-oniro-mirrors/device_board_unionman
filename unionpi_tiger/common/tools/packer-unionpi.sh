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

#!/usr/bin/env sh

root_src_dir=$(pwd)

pushd ${root_src_dir}
cp ${root_src_dir}/device/board/unionman/unionpi_tiger/bootloader/images/* ${root_src_dir}/out/unionpi_tiger/packages/phone/images -rvf
${root_src_dir}/device/board/unionman/unionpi_tiger/common/tools/linux/img2simg ${root_src_dir}/out/unionpi_tiger/packages/phone/images/system.img ${root_src_dir}/out/unionpi_tiger/packages/phone/images/system.img2simg
mv ${root_src_dir}/out/unionpi_tiger/packages/phone/images/system.img2simg ${root_src_dir}/out/unionpi_tiger/packages/phone/images/system.img
${root_src_dir}/device/board/unionman/unionpi_tiger/common/tools/linux/img2simg ${root_src_dir}/out/unionpi_tiger/packages/phone/images/vendor.img ${root_src_dir}/out/unionpi_tiger/packages/phone/images/vendor.img2simg
mv ${root_src_dir}/out/unionpi_tiger/packages/phone/images/vendor.img2simg ${root_src_dir}/out/unionpi_tiger/packages/phone/images/vendor.img
${root_src_dir}/device/board/unionman/unionpi_tiger/common/tools/linux/img2simg ${root_src_dir}/out/unionpi_tiger/packages/phone/images/userdata.img ${root_src_dir}/out/unionpi_tiger/packages/phone/images/userdata.img2simg
mv ${root_src_dir}/out/unionpi_tiger/packages/phone/images/userdata.img2simg ${root_src_dir}/out/unionpi_tiger/packages/phone/images/userdata.img
${root_src_dir}/device/board/unionman/unionpi_tiger/common/tools/linux/mkbootimg --kernel ${root_src_dir}/out/kernel/src_tmp/linux-5.10/unionpi_tiger/Image.gz --base 0x0 --kernel_offset 0x1080000 --cmdline "" --ramdisk  ${root_src_dir}/out/unionpi_tiger/packages/phone/images/ramdisk.img --second ${root_src_dir}/out/unionpi_tiger/packages/phone/images/dtb.img --output ${root_src_dir}/out/unionpi_tiger/packages/phone/images/boot.img > /dev/null
${root_src_dir}/device/board/unionman/unionpi_tiger/common/tools/linux/aml_image_v2_packer -r ${root_src_dir}/out/unionpi_tiger/packages/phone/images/openharmony.conf ${root_src_dir}/out/unionpi_tiger/packages/phone/images/ ${root_src_dir}/out/unionpi_tiger/packages/phone/images/OpenHarmony.img
rm -rf ${root_src_dir}/out/unionpi_tiger/packages/phone/images/openharmony.conf
rm -rf ${root_src_dir}/out/unionpi_tiger/packages/phone/images/platform.conf
rm -rf ${root_src_dir}/out/unionpi_tiger/packages/phone/images/aml_sdc_burn.ini
rm -rf ${root_src_dir}/out/unionpi_tiger/packages/phone/images/LICENSE
popd
