/*
 * Copyright (c) 2021 Unionman Technology Co., Ltd.
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

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/syscall.h>

#define REBOOT_MAGIC1 0xfee1dead
#define REBOOT_MAGIC2 672274793
#define REBOOT_CMD_RESTART2 0xA1B2C3D4

#define REBOOT_FASTBOOT_MODE "fastboot"
#define REBOOT_UPDATE_MODE "update"

static int DoRebootCmd(const char *cmd)
{
    return syscall(__NR_reboot, REBOOT_MAGIC1, REBOOT_MAGIC2, REBOOT_CMD_RESTART2, cmd);
}

int main(int argc, const char *argv[])
{
    int ret = 0;

    if (argc <= 1L) {
        printf("DoRebootCmd : update (usb burn) mode.\n");
        ret = DoRebootCmd(REBOOT_UPDATE_MODE);
    } else {
        const char *cmd = argv[1];
        if (strncmp(cmd, REBOOT_FASTBOOT_MODE, strlen(REBOOT_FASTBOOT_MODE)) == 0) {
            printf("DoRebootCmd : fastboot mode.\n");
            ret = DoRebootCmd(REBOOT_FASTBOOT_MODE);
        } else {
            ret = DoRebootCmd(cmd);
        }
    }

    if (ret != 0) {
        printf("DoRebootCmd failed! (%d) %s\n", errno, strerror(errno));
    } else {
        printf("DoRebootCmd : OK\n");
    }

    return ret;
}
