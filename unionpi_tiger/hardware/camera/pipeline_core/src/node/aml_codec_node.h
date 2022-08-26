/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HOS_CAMERA_AMLCODEC_NODE_H
#define HOS_CAMERA_AMLCODEC_NODE_H

#include <vector>
#include <condition_variable>
#include <ctime>
#include <jpeglib.h>
#include "device_manager_adapter.h"
#include "utils.h"
#include "camera.h"
#include "source_node.h"


namespace OHOS::Camera {
class AMLCodecNode : public NodeBase {
public:
    AMLCodecNode(const std::string& name, const std::string& type);
    ~AMLCodecNode() override;
    RetCode Start(const int32_t streamId) override;
    RetCode Stop(const int32_t streamId) override;
    void DeliverBuffer(std::shared_ptr<IBuffer>& buffer) override;
    virtual RetCode Capture(const int32_t streamId, const int32_t captureId) override;
    RetCode CancelCapture(const int32_t streamId) override;
    RetCode Flush(const int32_t streamId);
private:
    void encodeJpegToMemory(unsigned char* image, int width, int height,
            unsigned long* jpegSize, unsigned char** jpegBuf);
    void EncodeForPreview(std::shared_ptr<IBuffer>& buffer);
    void EncodeForJpeg(std::shared_ptr<IBuffer>& buffer);
    void EncodeForVideo(std::shared_ptr<IBuffer>& buffer);

    static uint32_t                       previewWidth_;
    static uint32_t                       previewHeight_;
    std::vector<std::shared_ptr<IPort>>   outPutPorts_;
    
    uint32_t    vencFrameCnt_ = 0;
    void*       ge2d_ = nullptr;
    long		h264Enc_ = -1;
};
} // namespace OHOS::Camera
#endif
