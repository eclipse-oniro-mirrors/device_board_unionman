/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include <stdio.h>
#include "ipp_algo.h"
#include "securec.h"

#define MAX_BUFFER_COUNT 100

int Init(const IppAlgoMeta *meta)
{
    printf("ipp algo example Init ...\n");
    return 0;
}

int Start(void)
{
    printf("ipp algo example Start ...\n");
    return 0;
}

int Flush(void)
{
    printf("ipp algo example Flush ...\n");
    return 0;
}

int Process(IppAlgoBuffer *inBuffer[], int inBufferCount, IppAlgoBuffer *outBuffer, const IppAlgoMeta *meta)
{
    printf("ipp algo example Process ...\n");
    if (inBuffer == NULL || inBufferCount > MAX_BUFFER_COUNT || inBufferCount < 1 ||
        inBuffer[0] == NULL || inBuffer[0]->addr == NULL) {
        printf("ipp inBuffer is NULL\n");
        return -1;
    }

    if (outBuffer == NULL || outBuffer->addr == NULL) {
        printf("ipp outBuffer is NULL\n");
        return -1;
    }

    char *in = (char *)(inBuffer[0]->addr);
    char *out = (char *)(outBuffer->addr);
    if (memcpy_s(out, outBuffer->size, in, inBuffer[0]->size) != 0) {
        printf("ipp memcpy_s failed.");
        return -1;
    }

    return 0;
}

int Stop(void)
{
    printf("ipp algo example Stop ...\n");
    return 0;
}
