import torch
import torch.nn as nn
import torch.nn.functional as F

class ShuffleV2Block(nn.Module):
    def __init__(self, inp, oup, mid_channels, *, ksize, stride, npu_friendly=False):
        super(ShuffleV2Block, self).__init__()
        self.stride = stride
        assert stride in [1, 2]

        self.mid_channels = mid_channels
        self.ksize = ksize
        pad = ksize // 2
        self.pad = pad
        self.inp = inp
        self.npu_friendly = npu_friendly

        if npu_friendly and stride == 1:
            channels = inp * 2
            even_then_odd = list(range(0, channels, 2)) + list(range(1, channels, 2))
            shuffle_weight = torch.zeros(channels, channels, 1, 1)
            for output_channel, input_channel in enumerate(even_then_odd):
                shuffle_weight[output_channel, input_channel, 0, 0] = 1.0
            # Do not add this deterministic deployment transform to checkpoints.
            self.register_buffer("shuffle_weight", shuffle_weight, persistent=False)

        outputs = oup - inp

        branch_main = [
            # pw
            nn.Conv2d(inp, mid_channels, 1, 1, 0, bias=False),
            nn.BatchNorm2d(mid_channels),
            nn.ReLU(inplace=True),
            # dw
            nn.Conv2d(mid_channels, mid_channels, ksize, stride, pad, groups=mid_channels, bias=False),
            nn.BatchNorm2d(mid_channels),
            # pw-linear
            nn.Conv2d(mid_channels, outputs, 1, 1, 0, bias=False),
            nn.BatchNorm2d(outputs),
            nn.ReLU(inplace=True),
        ]
        self.branch_main = nn.Sequential(*branch_main)

        if stride == 2:
            branch_proj = [
                # dw
                nn.Conv2d(inp, inp, ksize, stride, pad, groups=inp, bias=False),
                nn.BatchNorm2d(inp),
                # pw-linear
                nn.Conv2d(inp, inp, 1, 1, 0, bias=False),
                nn.BatchNorm2d(inp),
                nn.ReLU(inplace=True),
            ]
            self.branch_proj = nn.Sequential(*branch_proj)
        else:
            self.branch_proj = None

    def forward(self, old_x):
        if self.stride==1:
            x_proj, x = self.channel_shuffle(old_x)
            return torch.cat((x_proj, self.branch_main(x)), 1)
        elif self.stride==2:
            x_proj = old_x
            x = old_x
            return torch.cat((self.branch_proj(x_proj), self.branch_main(x)), 1)

    def channel_shuffle(self, x):
        if self.npu_friendly:
            # This fixed 1x1 convolution exactly reorders even channels before
            # odd channels.  Conv + Split is friendlier to Ethos-U than one
            # CPU StridedSlice region for every ShuffleNet block.
            shuffled = F.conv2d(x, self.shuffle_weight)
            half_channels = shuffled.shape[1] // 2
            return torch.split(shuffled, [half_channels, half_channels], dim=1)

        _, num_channels, _, _ = x.data.size()
        assert (num_channels % 4 == 0)
        return x[:, 0::2, :, :], x[:, 1::2, :, :]

class ShuffleNetV2(nn.Module):
    def __init__(self, stage_out_channels, load_param, npu_friendly=False):
        super(ShuffleNetV2, self).__init__()

        self.stage_repeats = [4, 8, 4]
        self.stage_out_channels = stage_out_channels

        # building first layer
        input_channel = self.stage_out_channels[1]
        self.first_conv = nn.Sequential(
            nn.Conv2d(3, input_channel, 3, 2, 1, bias=False),
            nn.BatchNorm2d(input_channel),
            nn.ReLU(inplace=True),
        )

        self.maxpool = nn.AvgPool2d(kernel_size=3, stride=2, padding=1)

        stage_names = ["stage2", "stage3", "stage4"]
        for idxstage in range(len(self.stage_repeats)):
            numrepeat = self.stage_repeats[idxstage]
            output_channel = self.stage_out_channels[idxstage+2]
            stageSeq = []
            for i in range(numrepeat):
                if i == 0:
                    stageSeq.append(ShuffleV2Block(input_channel, output_channel, 
                                                mid_channels=output_channel // 2, ksize=3, stride=2,
                                                npu_friendly=npu_friendly))
                else:
                    stageSeq.append(ShuffleV2Block(input_channel // 2, output_channel, 
                                                mid_channels=output_channel // 2, ksize=3, stride=1,
                                                npu_friendly=npu_friendly))
                input_channel = output_channel
            setattr(self, stage_names[idxstage], nn.Sequential(*stageSeq))
        
        if load_param == False:
            self._initialize_weights()
        else:
            print("load param...")

    def forward(self, x):
        x = self.first_conv(x)
        x = self.maxpool(x)
        C1 = self.stage2(x)
        C2 = self.stage3(C1)
        C3 = self.stage4(C2)

        return C2, C3

    def _initialize_weights(self):
        print("initialize_weights...")
        device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        self.load_state_dict(torch.load("./model/backbone/backbone.pth", map_location=device), strict = True)

if __name__ == "__main__":
    model = ShuffleNetV2()
    print(model)
    test_data = torch.rand(1, 3, 320, 320)
    test_outputs = model(test_data)
    for out in test_outputs:
        print(out.size())
