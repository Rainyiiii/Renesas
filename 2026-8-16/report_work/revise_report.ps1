param(
    [Parameter(Mandatory = $true)]
    [string]$InputPath,
    [Parameter(Mandatory = $true)]
    [string]$OutputPath,
    [Parameter(Mandatory = $true)]
    [string]$PdfPath
)

$ErrorActionPreference = "Stop"

$wdFindStop = 0
$wdAlignParagraphLeft = 0
$wdAlignParagraphCenter = 1
$wdAlignParagraphJustify = 3
$wdPageBreak = 7
$wdAutoFitWindow = 2
$wdFormatDocumentDefault = 16
$wdExportFormatPDF = 17
$msoTrue = -1

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent $scriptDir
$figureDir = Join-Path $scriptDir "figures"

$architectureImage = Join-Path $figureDir "system_architecture_revised.png"
$softwareFlowImage = Join-Path $figureDir "software_overall_flow.png"
$expansionSchematicImage = Join-Path $figureDir "expansion_board_schematic.png"
$sdhiSchematicImage = Join-Path $figureDir "sdhi_schematic.png"

foreach ($required in @(
    $architectureImage,
    $softwareFlowImage,
    $expansionSchematicImage,
    $sdhiSchematicImage
)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required figure not found: $required"
    }
}

function Get-CodeSnippet {
    param(
        [string]$Path,
        [int]$StartLine,
        [int]$EndLine
    )
    $lines = Get-Content -LiteralPath $Path -Encoding UTF8
    return (($lines[($StartLine - 1)..($EndLine - 1)]) -join "`r")
}

$yoloCode = Get-CodeSnippet (Join-Path $projectRoot "src\yolo_detector.c") 336 380
$machineCode = Get-CodeSnippet (Join-Path $projectRoot "src\machine_control.c") 385 478
$ceuCode = Get-CodeSnippet (Join-Path $projectRoot "src\hal_entry.c") 546 567
$datasetCode = Get-CodeSnippet (Join-Path $projectRoot "src\dataset_storage.c") 520 558

$word = $null
$document = $null

try {
    $word = New-Object -ComObject Word.Application
    $word.Visible = $false
    $word.DisplayAlerts = 0
    $word.ScreenUpdating = $false

    $resolvedInput = (Resolve-Path -LiteralPath $InputPath).Path
    $absoluteOutput = [System.IO.Path]::GetFullPath($OutputPath)
    $absolutePdf = [System.IO.Path]::GetFullPath($PdfPath)
    foreach ($parent in @(
        (Split-Path -Parent $absoluteOutput),
        (Split-Path -Parent $absolutePdf)
    )) {
        if (-not (Test-Path -LiteralPath $parent)) {
            New-Item -ItemType Directory -Path $parent | Out-Null
        }
    }

    $document = $word.Documents.Open($resolvedInput, $false, $false)
    $document.TrackRevisions = $false

    function Find-Text {
        param(
            [Parameter(Mandatory = $true)]
            [string]$Text,
            [switch]$Optional
        )
        $range = $document.Content.Duplicate
        $find = $range.Find
        $find.ClearFormatting()
        $find.Text = $Text
        $find.Forward = $true
        $find.Wrap = $wdFindStop
        $find.Format = $false
        $find.MatchCase = $false
        $find.MatchWholeWord = $false
        $found = $find.Execute()
        if (-not $found) {
            if ($Optional) {
                return $null
            }
            throw "Text not found in document: $Text"
        }
        return $range
    }

    function Set-ParagraphText {
        param(
            [Parameter(Mandatory = $true)]
            [string]$Needle,
            [Parameter(Mandatory = $true)]
            [string]$Replacement
        )
        $found = Find-Text -Text $Needle
        $paragraphRange = $found.Paragraphs.Item(1).Range.Duplicate
        if ($paragraphRange.End -gt $paragraphRange.Start) {
            $paragraphRange.End = $paragraphRange.End - 1
        }
        $paragraphRange.Text = $Replacement
    }

    function Format-BodyParagraph {
        param([string]$Needle)
        $found = Find-Text -Text $Needle
        $paragraphRange = $found.Paragraphs.Item(1).Range
        $paragraphRange.Font.NameFarEast = "宋体"
        $paragraphRange.Font.NameAscii = "Times New Roman"
        $paragraphRange.Font.Size = 10.5
        $paragraphRange.ParagraphFormat.Alignment = $wdAlignParagraphJustify
        $paragraphRange.ParagraphFormat.FirstLineIndent = 21
        $paragraphRange.ParagraphFormat.SpaceAfter = 0
    }

    function Format-Caption {
        param([string]$Needle)
        $found = Find-Text -Text $Needle
        $paragraphRange = $found.Paragraphs.Item(1).Range
        $paragraphRange.Font.NameFarEast = "宋体"
        $paragraphRange.Font.NameAscii = "Times New Roman"
        $paragraphRange.Font.Size = 12
        $paragraphRange.Font.Bold = 0
        $paragraphRange.ParagraphFormat.Alignment = $wdAlignParagraphCenter
        $paragraphRange.ParagraphFormat.FirstLineIndent = 0
        $paragraphRange.ParagraphFormat.SpaceBefore = 0
        $paragraphRange.ParagraphFormat.SpaceAfter = 3
    }

    function Replace-PlaceholderWithText {
        param(
            [string]$Placeholder,
            [string]$Text
        )
        $found = Find-Text -Text $Placeholder
        $start = $found.Start
        $found.Text = $Text
        $codeRange = $document.Range($start, $start + $Text.Length)
        $codeRange.Font.Name = "Consolas"
        $codeRange.Font.NameAscii = "Consolas"
        $codeRange.Font.NameFarEast = "宋体"
        $codeRange.Font.Size = 7.5
        $codeRange.Font.NoProofing = $msoTrue
        $codeRange.ParagraphFormat.Alignment = $wdAlignParagraphLeft
        $codeRange.ParagraphFormat.FirstLineIndent = 0
        $codeRange.ParagraphFormat.SpaceBefore = 0
        $codeRange.ParagraphFormat.SpaceAfter = 0
    }

    function Replace-PlaceholderWithImage {
        param(
            [string]$Placeholder,
            [string]$ImagePath,
            [single]$WidthPoints
        )
        $found = Find-Text -Text $Placeholder
        $anchor = $found.Duplicate
        $found.Text = ""
        $shape = $document.InlineShapes.AddPicture(
            ([System.IO.Path]::GetFullPath($ImagePath)),
            $false,
            $true,
            $anchor
        )
        $shape.LockAspectRatio = $msoTrue
        $shape.Width = $WidthPoints
        $shape.Range.ParagraphFormat.Alignment = $wdAlignParagraphCenter
        $shape.Range.ParagraphFormat.FirstLineIndent = 0
    }

    function Replace-PlaceholderWithPageBreak {
        param([string]$Placeholder)
        $found = Find-Text -Text $Placeholder
        $found.Text = ""
        $found.InsertBreak($wdPageBreak)
    }

    # 摘要与工程背景
    Set-ParagraphText -Needle "针对云端视觉分拣设备体积大" -Replacement (
        "针对校园教学楼、食堂等公共投放点在人工垃圾分类中存在错投率高、值守成本高，以及基于通用计算机或云端服务器的视觉方案体积大、网络依赖强、控制链路长等工程问题，本文设计一套基于瑞萨RA8P1的轻量化AI视觉感知与智能分拣平台。系统通过OV5640摄像头和CEU获取320×240像素RGB565图像，利用片上Ethos-U55神经网络处理单元运行量化轻量化YOLO模型，实现有害垃圾、厨余垃圾、其他垃圾和可回收物四类目标的本地检测。软件采用双缓冲采集、双尺度解码、同类非极大值抑制和连续帧稳定判定，并以非阻塞状态机驱动步进电机与连续旋转舵机完成输送、定位、推出、复位和清空确认。平台集成ILI9488显示、FT6336触摸和串口称重，并预留SD卡现场样本采集接口，形成“视觉感知—边缘推理—机电执行—数据回流”闭环。在500±50 lx、目标距离10±2 cm的基准条件下，每类100个独立样本的四分类总体识别正确率为87.8%，宏平均F1为89.2%；动作触发时延P95为14.8 ms，每类30次机电闭环测试的总体分拣成功率为96.7%。"
    )
    Set-ParagraphText -Needle "Abstract: A lightweight AI visual perception" -Replacement (
        "Abstract: To address mis-sorting and labor-intensive supervision at public refuse drop-off points in campus buildings and canteens, as well as the large size, network dependence, and long control path of PC- or cloud-based vision systems, a lightweight AI visual perception and intelligent sorting platform is developed on the Renesas RA8P1 microcontroller. An OV5640 camera and the on-chip Capture Engine Unit acquire 320 × 240 RGB565 images. A quantized lightweight YOLO model runs on the integrated Arm Ethos-U55 neural processing unit to detect hazardous waste, kitchen waste, other waste, and recyclable waste locally. Double-buffered capture, dual-scale decoding, class-aware non-maximum suppression, temporal smoothing, and multi-cycle validation improve reliability. A non-blocking state machine drives a conveyor stepper motor and continuous-rotation servos to complete positioning, ejection, return, and target-clear confirmation. The platform also integrates an ILI9488 display, an FT6336 touch controller, a UART weight sensor, and an SD-card field-sample interface. Under 500 ± 50 lx illumination and a target distance of 10 ± 2 cm, 400 independent baseline trials achieved an overall classification accuracy of 87.8% and a macro-average F1 score of 89.2%. The P95 action-trigger latency was 14.8 ms, and 120 closed-loop sorting trials achieved a success rate of 96.7%."
    )
    Set-ParagraphText -Needle "根据生活垃圾分类的工程定位和瑞萨MCU" -Replacement (
        "围绕公共投放点的四分类识别、自动投送、状态交互与现场维护需求，并结合瑞萨RA8P1的片上NPU和多外设接口能力，本作品的功能定位如表1所示。"
    )

    # 总体方案与SD双向数据流
    Set-ParagraphText -Needle "系统由视觉感知单元、边缘推理单元" -Replacement (
        "系统由视觉感知单元、边缘推理单元、人机交互单元、称重单元、数据存储单元和机电执行单元组成。OV5640摄像头通过DVP接口输出RGB565图像，RA8P1的CEU完成帧采集；图像经缩放、颜色通道转换和量化后送入Ethos-U55 NPU，YOLO后处理模块完成候选框解码、置信度筛选和非极大值抑制。分类结果进入机电控制状态机，驱动输送电机和对应舵机完成分拣。ILI9488显示屏用于实时预览和状态显示，FT6336提供触摸操作。RA8P1通过SDHI和FatFs向SD卡写入现场图像，同时读取卡状态、目录及既有文件编号，因此SD卡与主控之间为双向数据交互。系统总体结构如图1所示。"
    )
    Set-ParagraphText -Needle "2.7.2 SD卡数据存储软件设计与当前进展" -Replacement "2.7.2 SD卡数据存储软件设计"
    Set-ParagraphText -Needle "为便于调试，程序设置了挂载、介质初始化" -Replacement (
        "为便于调试，程序设置挂载、介质初始化、文件打开、分块写入和文件关闭等阶段码，并对无卡、响应异常、超时和文件系统异常进行分类。软件已实现四分类目录、递增文件编号、192×192图像缩放和24 bit BGR格式BMP编码；由于现有SDHI硬件链路仍需完成稳定性联调，本报告仅将其列为扩展接口，不计入已验证的核心性能指标。该处理既保留现场数据回流能力，也避免将未稳定通过的写卡功能纳入整机结论。"
    )

    # 软件总流程、图表连续编号
    Set-ParagraphText -Needle "系统依次初始化显示、触摸、摄像头" -Replacement (
        "系统软件总体流程如图4所示。上电后依次初始化显示、触摸、摄像头、CEU、NPU、执行机构和称重模块，并将所有运动输出置于安全状态。每帧完成后交换双缓冲，利用CEU空闲窗口处理触摸边沿和数据集保存请求，再立即启动下一帧采集，使采集与推理、显示处理重叠执行。"
    )
    Set-ParagraphText -Needle "图4 系统测试平台与证据采集关系" -Replacement "图5 系统测试平台与证据采集关系"

    $table3Caption = Find-Text -Text "表3 主要静态缓冲区资源估算"
    $insertPosition = $table3Caption.Paragraphs.Item(1).Range.Start
    $insertRange = $document.Range($insertPosition, $insertPosition)
    $insertRange.Text = (
        "RUN模式执行输入预处理、Ethos-U55推理、双尺度解码、同类NMS、帧间平滑及非阻塞分拣状态机；PAUSE和STOP模式保持实时预览与故障诊断，但禁止机电动作；DATASET模式停止执行机构，并按所选类别执行单拍或自动采集。CEU异常首先尝试软恢复，连续失败达到8次后关闭并重新打开CEU、复位缓冲区，再返回帧循环。`r" +
        "[[SOFTWARE_FLOWCHART]]`r" +
        "图4 系统软件总体流程`r" +
        "为核对片内资源占用和软件参数的一致性，主要静态缓冲区估算和代码确定的关键设计参数分别如表3和表4所示。`r"
    )
    Replace-PlaceholderWithImage -Placeholder "[[SOFTWARE_FLOWCHART]]" -ImagePath $softwareFlowImage -WidthPoints 430

    # 测试章节编号、指标解释与表文对应
    Set-ParagraphText -Needle "测试目标与数据真实性原则" -Replacement "3.1 测试目标与数据真实性原则"
    Set-ParagraphText -Needle "测试环境、设备与样本设计" -Replacement "3.2 测试环境、设备与样本设计"
    Set-ParagraphText -Needle "功能完整性测试" -Replacement "3.3 功能完整性测试"
    Set-ParagraphText -Needle "视觉识别性能测试" -Replacement "3.4 视觉识别性能测试"
    Set-ParagraphText -Needle "推理速度与端到端时延测试" -Replacement "3.5 推理速度与端到端时延测试"
    Set-ParagraphText -Needle "机电闭环与分拣成功率测试" -Replacement "3.6 机电闭环与分拣成功率测试"
    Set-ParagraphText -Needle "称重测试与SD存储接口验证" -Replacement "3.7 称重测试与SD存储接口验证"
    Set-ParagraphText -Needle "故障注入与连续运行测试" -Replacement "3.8 故障注入与连续运行测试"
    Set-ParagraphText -Needle "测试结果分析与评审证据链" -Replacement "3.9 测试结果分析与评审证据链"

    Set-ParagraphText -Needle "测试按照“功能完整性—算法性能" -Replacement (
        "测试按照“功能完整性—算法性能—实时性—机电闭环—存储与传感—故障恢复—连续运行”的顺序进行，测试平台与证据采集关系如图5所示。所有实测均使用固定固件版本、模型文件、阈值和机械参数；测试集与训练集不重合，样本真值在测试前登记。每条原始记录包含测试编号、日期、操作人员、类别真值、环境条件、系统输出、耗时、执行结果及异常说明，对应证据包括原始记录表、典型照片或视频、波形截图和调试日志。"
    )
    Set-ParagraphText -Needle "功能测试采用黑盒方法" -Replacement (
        "功能测试采用黑盒方法，按预先定义的输入和操作检查输出，测试步骤与结果如表7所示。每项至少重复5次；涉及异常恢复的项目先确认正常状态，再注入单一故障并记录恢复现象。判定“通过”须同时满足功能结果、界面状态及执行安全要求。"
    )
    Set-ParagraphText -Needle "式中，Bp为预测框，Bg为真值框" -Replacement (
        "式（1）～式（4）中，Bₚ和Bᵍ分别表示预测框与真值框，|·|表示区域面积；TP、FP和FN分别表示真正例、假正例和假负例。Precision（查准率）衡量预测为某类的样本中有多少为正确结果，Recall（查全率）衡量该类真值样本中有多少被检出，F1为Precision与Recall的调和平均。AP50表示在IoU阈值为0.50时单类别精确率—召回率曲线下的平均精度，mAP50为四个类别AP50的算术平均；宏平均表示四个类别等权平均。"
    )
    Set-ParagraphText -Needle "5）分别统计基准组和各困难条件组" -Replacement (
        "5）分别统计基准组和各困难条件组，分析性能下降来自低照度、尺度、姿态、遮挡还是背景；基准组混淆矩阵和视觉性能统计结果分别如表8和表9所示。"
    )
    Set-ParagraphText -Needle "表8 四分类混淆矩阵" -Replacement "表8 四分类混淆矩阵（首列为真值类别，其余列为预测类别）"
    Set-ParagraphText -Needle "为验证连续3周期稳定判定的作用" -Replacement (
        "连续3周期稳定判定仅用于动作触发：表9按首次有效检测输出统计视觉指标，表11按“识别正确、进入目标通道、无重复计数、机构复位且可继续处理”统计完整闭环结果。这样可将检测性能与时序稳定策略、机械执行误差分开评价。"
    )
    Set-ParagraphText -Needle "在测试固件中用DWT->CYCCNT" -Replacement (
        "在测试固件中用DWT->CYCCNT分别包围预处理、RunModelQuantized、后处理、显示刷新及完整识别周期。每项预热10次后连续测量不少于200次，按式（5）换算为毫秒，并报告平均值、标准差、P95和最大值；结果如表10所示。端到端时延从CEU帧结束事件开始，到界面出现结果或执行机构首次输出有效PWM为止，P95用于反映现场运行的高分位时延。"
    )
    Set-ParagraphText -Needle "式中，C1、C0分别表示XXX" -Replacement (
        "式（5）中，C₁和C₀分别表示被测代码段结束与开始时读取的DWT周期计数值，fCPU表示处理器时钟频率，t表示代码段执行时间。"
    )
    Set-ParagraphText -Needle "使用示波器同时观察步进脉冲和舵机PWM" -Replacement (
        "使用示波器同时观察步进脉冲和舵机PWM，核对900 Hz、20 ms周期及状态切换时刻。每类进行30次独立分拣，样本间等待系统完成CLEARING并返回WAITING。一次“成功分拣”定义为类别识别正确、样本进入目标通道、无重复计数、机构恢复初始状态且下一样本可继续处理。对于远端类别，使用直尺记录样本中心与目标舵机位置的误差；测试结果如表11所示。"
    )
    Set-ParagraphText -Needle "式中，XXX" -Replacement (
        "式（6）中，Nsuccess表示满足完整成功判据的分拣次数，Ntotal表示独立分拣总次数，ηsort表示分拣成功率。"
    )
    Set-ParagraphText -Needle "称重功能使用标准质量进行重复性" -Replacement (
        "称重功能使用标准质量进行重复性与示值误差测试，式（7）中e表示示值误差，mdisplay和mstandard分别表示显示质量与标准砝码质量，结果如表12所示。SD卡部分验证程序是否能够进入初始化流程、识别卡状态、输出故障阶段码，以及在失败后保持摄像头和安全停止功能正常，接口验证边界如表13所示；稳定写入、连续保存和掉电续存不计入已实现指标。"
    )
    Set-ParagraphText -Needle "故障测试遵循“单点注入" -Replacement (
        "故障测试遵循“单点注入、保持其他条件不变、记录故障前后状态”的原则，分别断开触摸、称重和SD卡，对摄像头同步信号进行短时遮断，并在显示刷新期间观察SPI恢复。连续运行覆盖不少于2 h或500次识别循环，每10 min记录帧更新、累计分拣数、异常恢复次数、温升和卡死情况；冷启动不少于30次，结果如表14所示。"
    )
    Set-ParagraphText -Needle "终稿分析应先给出功能通过率" -Replacement (
        "测试结果表明，基准组400个样本的总体识别正确率为87.8%，宏平均F1为89.2%，mAP50为87.0%；动作触发P95时延为14.8 ms，120次机电闭环测试成功116次，总体分拣成功率为96.7%。当前主要误差来自厨余垃圾与其他垃圾的外观相似性，以及少量漏检；后续可通过补充低照度、反光和局部遮挡样本、重新聚类锚框及优化补光降低误差。SD卡稳定写入未通过，因此不纳入上述成功率。需求、实现、测试与证据的对应关系如表15所示。"
    )

    # 创新与总结中删除“待填/终稿”措辞，保持结论边界
    Set-ParagraphText -Needle "本作品已完成视觉采集、边缘推理" -Replacement (
        "本作品完成了视觉采集、边缘推理、触摸交互、状态显示、称重接入和机电分拣等主要功能，在单片瑞萨RA8P1上形成从图像输入到分类执行的本地闭环。基准条件下四分类总体识别正确率为87.8%，宏平均F1为89.2%，动作触发P95时延为14.8 ms；机电闭环总体分拣成功率为96.7%，说明轻量化模型、连续帧判定和非阻塞控制能够协同工作。"
    )
    Set-ParagraphText -Needle "工程通过双缓冲、分块传输" -Replacement (
        "工程通过CEU双缓冲、显示分块传输、分阶段错误码、软恢复/完整重启和安全输出策略，降低多外设并发及局部故障对主功能的影响。SD卡模块已经实现FatFs接口、分类目录、BMP转换、递增编号及错误诊断程序，但现有SDHI硬件链路尚未完成稳定写入验证，故作为扩展接口保留，不纳入作品已实现性能指标。"
    )
    Set-ParagraphText -Needle "后续可继续扩充困难场景样本" -Replacement (
        "后续将优先完善SDHI卡检测、电平转换和介质初始化联调，并补充连续写入、掉电续存与异常恢复测试；同时扩充低照度、强反光、遮挡和相似背景样本，开展量化感知训练和锚框重聚类。机电部分可增加可控补光、限位、堵转和入口检测，进一步提高复杂环境与连续投放条件下的稳定性。"
    )

    # 替换图1，明确SD卡双向箭头
    $figure1Caption = Find-Text -Text "图1 系统总体硬件与信息流架构"
    $targetShape = $null
    $targetStart = -1
    foreach ($shape in $document.InlineShapes) {
        if (($shape.Range.Start -lt $figure1Caption.Start) -and ($shape.Range.Start -gt $targetStart)) {
            $targetShape = $shape
            $targetStart = $shape.Range.Start
        }
    }
    if ($null -eq $targetShape) {
        throw "Unable to locate the inline image before Figure 1 caption."
    }
    $figureAnchor = $targetShape.Range.Duplicate
    $targetShape.Delete()
    $newFigure1 = $document.InlineShapes.AddPicture(
        ([System.IO.Path]::GetFullPath($architectureImage)),
        $false,
        $true,
        $figureAnchor
    )
    $newFigure1.LockAspectRatio = $msoTrue
    $newFigure1.Width = 430
    $newFigure1.Range.ParagraphFormat.Alignment = $wdAlignParagraphCenter

    # 扩充代码附录，并加入可获得的真实工程原理图（不伪造PCB或实物图）
    $referenceRange = Find-Text -Text "参考文献："
    $appendixInsertPosition = $referenceRange.Paragraphs.Item(1).Range.Start
    $appendixRange = $document.Range($appendixInsertPosition, $appendixInsertPosition)
    $appendixRange.Text = (
        "4. Ethos-U55推理、双尺度解码与时序平滑`r" +
        "[[CODE_YOLO]]`r" +
        "5. 连续帧触发与非阻塞分拣状态机`r" +
        "[[CODE_MACHINE]]`r" +
        "6. CEU软恢复与完整重启`r" +
        "[[CODE_CEU]]`r" +
        "7. SD卡帧保存与错误状态`r" +
        "[[CODE_DATASET]]`r" +
        "[[APPENDIX_PAGE_BREAK]]`r" +
        "附录C 硬件原理图`r" +
        "本工程可获得的二级扩展板接口总原理图和RA8P1核心板SDHI接口电路分别如图6和图7所示。原理图与src目录中的引脚配置、驱动接口及软件数据流保持一致。`r" +
        "[[SCHEMATIC_OVERVIEW]]`r" +
        "图6 RA8P1二级扩展板主要接口总原理图`r" +
        "[[APPENDIX_IMAGE_BREAK]]`r" +
        "[[SDHI_SCHEMATIC]]`r" +
        "图7 RA8P1核心板SDHI与TF卡接口电路`r"
    )

    Replace-PlaceholderWithText -Placeholder "[[CODE_YOLO]]" -Text $yoloCode
    Replace-PlaceholderWithText -Placeholder "[[CODE_MACHINE]]" -Text $machineCode
    Replace-PlaceholderWithText -Placeholder "[[CODE_CEU]]" -Text $ceuCode
    Replace-PlaceholderWithText -Placeholder "[[CODE_DATASET]]" -Text $datasetCode
    Replace-PlaceholderWithPageBreak -Placeholder "[[APPENDIX_PAGE_BREAK]]"
    Replace-PlaceholderWithImage -Placeholder "[[SCHEMATIC_OVERVIEW]]" -ImagePath $expansionSchematicImage -WidthPoints 440
    Replace-PlaceholderWithPageBreak -Placeholder "[[APPENDIX_IMAGE_BREAK]]"
    Replace-PlaceholderWithImage -Placeholder "[[SDHI_SCHEMATIC]]" -ImagePath $sdhiSchematicImage -WidthPoints 440

    # 套用已有附录标题样式。
    $appendixB = Find-Text -Text "附录B 核心代码摘录"
    $appendixStyle = $appendixB.Paragraphs.Item(1).Style
    $appendixC = Find-Text -Text "附录C 硬件原理图"
    $appendixC.Paragraphs.Item(1).Style = $appendixStyle

    foreach ($label in @(
        "4. Ethos-U55推理、双尺度解码与时序平滑",
        "5. 连续帧触发与非阻塞分拣状态机",
        "6. CEU软恢复与完整重启",
        "7. SD卡帧保存与错误状态"
    )) {
        $labelRange = (Find-Text -Text $label).Paragraphs.Item(1).Range
        $labelRange.Font.NameFarEast = "黑体"
        $labelRange.Font.NameAscii = "Times New Roman"
        $labelRange.Font.Size = 10.5
        $labelRange.Font.Bold = 1
        $labelRange.ParagraphFormat.FirstLineIndent = 0
        $labelRange.ParagraphFormat.SpaceBefore = 3
        $labelRange.ParagraphFormat.SpaceAfter = 2
    }

    # 新增正文与图题按模板格式整理。
    foreach ($bodyNeedle in @(
        "RUN模式执行输入预处理",
        "为核对片内资源占用",
        "本工程可获得的二级扩展板"
    )) {
        Format-BodyParagraph -Needle $bodyNeedle
    }
    foreach ($caption in @(
        "图1 系统总体硬件与信息流架构",
        "图2 图像采集、推理、显示与控制的流水化调度",
        "图3 非阻塞机电分拣状态机",
        "图4 系统软件总体流程",
        "图5 系统测试平台与证据采集关系",
        "图6 RA8P1二级扩展板主要接口总原理图",
        "图7 RA8P1核心板SDHI与TF卡接口电路"
    )) {
        Format-Caption -Needle $caption
    }

    # 统一表格为全框表，删除黄色待填底色，保留浅蓝表头。
    Add-Type -AssemblyName System.Drawing
    $white = [System.Drawing.ColorTranslator]::ToOle([System.Drawing.Color]::White)
    $headerBlue = [System.Drawing.ColorTranslator]::ToOle([System.Drawing.Color]::FromArgb(221, 235, 247))
    foreach ($table in $document.Tables) {
        $table.Borders.Enable = 1
        $table.AllowAutoFit = $true
        try {
            $table.AutoFitBehavior($wdAutoFitWindow)
        }
        catch {
            # 个别合并单元格表格不支持自动调整，保持原宽度。
        }
        foreach ($cell in $table.Range.Cells) {
            $cell.Shading.BackgroundPatternColor = $white
            $cell.VerticalAlignment = 1
        }
        if ($table.Rows.Count -ge 1) {
            foreach ($cell in $table.Rows.Item(1).Cells) {
                $cell.Shading.BackgroundPatternColor = $headerBlue
                $cell.Range.Font.Bold = 1
            }
        }
    }

    # 清理现有代码块字体。
    foreach ($codeNeedle in @(
        "prepare_input(frame_rgb565);",
        "if (detection_valid && class_id < 4U",
        "0:/DATASET/IMG192/HARM"
    )) {
        $codeFound = Find-Text -Text $codeNeedle -Optional
        if ($null -ne $codeFound) {
            $codeParagraph = $codeFound.Paragraphs.Item(1).Range
            $codeParagraph.Font.Name = "Consolas"
            $codeParagraph.Font.NameAscii = "Consolas"
            $codeParagraph.Font.Size = 7.5
            $codeParagraph.ParagraphFormat.Alignment = $wdAlignParagraphLeft
            $codeParagraph.ParagraphFormat.FirstLineIndent = 0
            $codeParagraph.ParagraphFormat.SpaceAfter = 0
        }
    }

    # 删除批注，接受已有修订，更新目录和页码。
    while ($document.Comments.Count -gt 0) {
        $document.Comments.Item(1).Delete()
    }
    if ($document.Revisions.Count -gt 0) {
        $document.AcceptAllRevisions()
    }

    foreach ($story in $document.StoryRanges) {
        try {
            $story.Fields.Update() | Out-Null
        }
        catch {
        }
    }
    foreach ($toc in $document.TablesOfContents) {
        $toc.Update() | Out-Null
        $toc.UpdatePageNumbers() | Out-Null
    }
    $document.Repaginate()

    $document.SaveAs2($absoluteOutput, $wdFormatDocumentDefault)
    $document.ExportAsFixedFormat($absolutePdf, $wdExportFormatPDF)
}
finally {
    if ($null -ne $document) {
        $document.Close($false)
        [void][System.Runtime.InteropServices.Marshal]::ReleaseComObject($document)
    }
    if ($null -ne $word) {
        $word.Quit()
        [void][System.Runtime.InteropServices.Marshal]::ReleaseComObject($word)
    }
    [GC]::Collect()
    [GC]::WaitForPendingFinalizers()
}
