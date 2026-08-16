from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT = Path(__file__).resolve().parent / "figures"
OUT.mkdir(parents=True, exist_ok=True)


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont:
    candidates = [
        Path("C:/Windows/Fonts/msyhbd.ttc" if bold else "C:/Windows/Fonts/msyh.ttc"),
        Path("C:/Windows/Fonts/simhei.ttf" if bold else "C:/Windows/Fonts/simsun.ttc"),
    ]
    for candidate in candidates:
        if candidate.exists():
            return ImageFont.truetype(str(candidate), size=size)
    return ImageFont.load_default()


TITLE = font(42, bold=True)
BODY = font(25)
SMALL = font(21)
SMALL_BOLD = font(22, bold=True)


def centered_multiline(
    draw: ImageDraw.ImageDraw,
    xy: tuple[int, int],
    text: str,
    font_obj: ImageFont.ImageFont,
    fill: str = "#111111",
    spacing: int = 8,
) -> None:
    bbox = draw.multiline_textbbox((0, 0), text, font=font_obj, spacing=spacing, align="center")
    width = bbox[2] - bbox[0]
    height = bbox[3] - bbox[1]
    draw.multiline_text(
        (xy[0] - width / 2, xy[1] - height / 2),
        text,
        font=font_obj,
        fill=fill,
        spacing=spacing,
        align="center",
    )


def box(
    draw: ImageDraw.ImageDraw,
    rect: tuple[int, int, int, int],
    text: str,
    *,
    width: int = 3,
    fill: str = "#ffffff",
    font_obj: ImageFont.ImageFont = BODY,
    radius: int = 18,
) -> None:
    draw.rounded_rectangle(rect, radius=radius, fill=fill, outline="#111111", width=width)
    centered_multiline(
        draw,
        ((rect[0] + rect[2]) // 2, (rect[1] + rect[3]) // 2),
        text,
        font_obj,
    )


def diamond(
    draw: ImageDraw.ImageDraw,
    center: tuple[int, int],
    size: tuple[int, int],
    text: str,
) -> tuple[int, int, int, int]:
    cx, cy = center
    w, h = size
    points = [(cx, cy - h // 2), (cx + w // 2, cy), (cx, cy + h // 2), (cx - w // 2, cy)]
    draw.polygon(points, fill="#ffffff", outline="#111111")
    draw.line(points + [points[0]], fill="#111111", width=3, joint="curve")
    centered_multiline(draw, center, text, SMALL_BOLD, spacing=5)
    return (cx - w // 2, cy - h // 2, cx + w // 2, cy + h // 2)


def arrow(
    draw: ImageDraw.ImageDraw,
    start: tuple[int, int],
    end: tuple[int, int],
    *,
    both: bool = False,
    width: int = 4,
) -> None:
    draw.line([start, end], fill="#111111", width=width)

    def head(tip: tuple[int, int], tail: tuple[int, int]) -> None:
        import math

        angle = math.atan2(tip[1] - tail[1], tip[0] - tail[0])
        length = 18
        spread = 0.55
        p1 = (
            tip[0] - length * math.cos(angle - spread),
            tip[1] - length * math.sin(angle - spread),
        )
        p2 = (
            tip[0] - length * math.cos(angle + spread),
            tip[1] - length * math.sin(angle + spread),
        )
        draw.polygon([tip, p1, p2], fill="#111111")

    head(end, start)
    if both:
        head(start, end)


def architecture() -> None:
    image = Image.new("RGB", (1800, 1080), "white")
    draw = ImageDraw.Draw(image)
    centered_multiline(draw, (900, 70), "系统总体硬件与信息流架构", TITLE)

    center = (575, 310, 1225, 770)
    box(
        draw,
        center,
        "瑞萨 RA8P1\nCortex-M85 + Ethos-U55\nCEU 图像采集 / YOLO 推理\n人机交互与机电状态机调度",
        width=4,
        fill="#f5f7f9",
        font_obj=font(30),
    )

    left = [
        ((60, 170, 430, 350), "OV5640 摄像头\nDVP，QVGA RGB565"),
        ((60, 450, 430, 630), "FT6336 触摸屏\n模式选择与控制输入"),
        ((60, 730, 430, 910), "称重模块\nUART 质量数据"),
    ]
    right = [
        ((1370, 170, 1740, 350), "ILI9488 显示屏\n实时预览与状态面板"),
        ((1370, 450, 1740, 630), "SD 卡 / FatFs\n现场数据集与文件状态"),
        ((1370, 730, 1740, 910), "步进电机 + 舵机\n四分类执行机构"),
    ]
    for rect, text in left + right:
        box(draw, rect, text)

    for rect, _ in left:
        arrow(draw, (rect[2], (rect[1] + rect[3]) // 2), (center[0], (rect[1] + rect[3]) // 2))
    arrow(draw, (center[2], 260), (right[0][0][0], 260))
    arrow(draw, (center[2], 540), (right[1][0][0], 540), both=True)
    arrow(draw, (center[2], 820), (right[2][0][0], 820))
    centered_multiline(draw, (1300, 495), "SDHI 双向读/写", font(18, bold=True))
    centered_multiline(
        draw,
        (900, 1010),
        "单片 MCU 内形成“感知—推理—决策—执行—数据回流”闭环",
        font(27, bold=True),
    )
    image.save(OUT / "system_architecture_revised.png", dpi=(300, 300))


def software_flow() -> None:
    image = Image.new("RGB", (1800, 1500), "white")
    draw = ImageDraw.Draw(image)
    centered_multiline(draw, (900, 62), "系统软件总体流程", TITLE)

    x_center = 900
    box(draw, (650, 115, 1150, 205), "上电 / 复位", fill="#f5f7f9", font_obj=SMALL_BOLD)
    arrow(draw, (x_center, 205), (x_center, 245))
    box(
        draw,
        (520, 245, 1280, 365),
        "初始化显示、触摸、OV5640、CEU、Ethos-U55\n执行机构与称重模块；输出安全初始状态",
        fill="#f5f7f9",
        font_obj=SMALL,
    )
    arrow(draw, (x_center, 365), (x_center, 405))
    box(draw, (650, 405, 1150, 495), "启动 CEU 帧采集（缓冲区 A/B）", font_obj=SMALL_BOLD)
    arrow(draw, (x_center, 495), (x_center, 535))
    frame_ok = diamond(draw, (900, 610), (430, 150), "帧结束事件正常？")

    arrow(draw, (frame_ok[2], 610), (1350, 610))
    centered_multiline(draw, (1285, 580), "否", SMALL_BOLD)
    box(
        draw,
        (1350, 535, 1740, 685),
        "软恢复：重新启动采集\n连续失败达到 8 次：\n关闭并重开 CEU、复位缓冲区",
        fill="#fff8f1",
        font_obj=SMALL,
    )
    arrow(draw, (1545, 535), (1545, 450))
    arrow(draw, (1545, 450), (1150, 450))

    arrow(draw, (x_center, frame_ok[3]), (x_center, 725))
    centered_multiline(draw, (940, 704), "是", SMALL_BOLD)
    box(
        draw,
        (500, 725, 1300, 845),
        "交换完整帧缓冲；触摸边沿轮询与模式更新\n随后立即启动下一帧采集，实现采集/处理重叠",
        fill="#f5f7f9",
        font_obj=SMALL,
    )
    arrow(draw, (x_center, 845), (x_center, 885))
    mode = diamond(draw, (900, 960), (430, 150), "当前运行模式？")

    arrow(draw, (mode[0], 960), (450, 960))
    centered_multiline(draw, (530, 925), "DATASET", SMALL_BOLD)
    box(
        draw,
        (55, 885, 450, 1035),
        "停止执行机构\n按类别选择单拍/自动采集\nRGB565 → 192×192 BGR BMP",
        fill="#f5f7f9",
        font_obj=SMALL,
    )

    arrow(draw, (mode[2], 960), (1350, 960))
    centered_multiline(draw, (1270, 925), "PAUSE / STOP", SMALL_BOLD)
    box(
        draw,
        (1350, 885, 1740, 1035),
        "保持实时预览与故障诊断\n禁止步进与舵机动作\n输出安全状态",
        fill="#f5f7f9",
        font_obj=SMALL,
    )

    arrow(draw, (x_center, mode[3]), (x_center, 1075))
    centered_multiline(draw, (945, 1058), "RUN", SMALL_BOLD)
    box(
        draw,
        (490, 1075, 1310, 1195),
        "输入预处理 → Ethos-U55 推理 → 双尺度解码\n置信度筛选 → 同类 NMS → 帧间平滑",
        fill="#f5f7f9",
        font_obj=SMALL,
    )
    arrow(draw, (x_center, 1195), (x_center, 1235))
    box(
        draw,
        (490, 1235, 1310, 1355),
        "连续 3 周期同类且置信度 ≥ 0.45 时触发\n非阻塞分拣状态机；同时叠加检测框并刷新界面",
        fill="#f5f7f9",
        font_obj=SMALL,
    )

    for x, start_y in [(250, 1035), (1545, 1035), (900, 1355)]:
        draw.line([(x, start_y), (x, 1430)], fill="#111111", width=4)
    draw.line([(250, 1430), (1545, 1430)], fill="#111111", width=4)
    draw.line([(250, 1430), (25, 1430), (25, 450)], fill="#111111", width=4)
    arrow(draw, (25, 450), (650, 450))
    centered_multiline(draw, (900, 1465), "完成本周期，返回帧采集循环", SMALL_BOLD)
    image.save(OUT / "software_overall_flow.png", dpi=(300, 300))


def appendix_images() -> None:
    schematic = Image.open(ROOT / "schematic_review_latest-1.png").convert("RGB")
    cropped = schematic.crop((0, 0, schematic.width, min(1500, schematic.height)))
    cropped.save(OUT / "expansion_board_schematic.png", dpi=(300, 300))

    sd = Image.open(ROOT / "cpkhmi_ra8p1_page6_sd.png").convert("RGB")
    sd.save(OUT / "sdhi_schematic.png", dpi=(300, 300))


if __name__ == "__main__":
    architecture()
    software_flow()
    appendix_images()
    for path in sorted(OUT.glob("*.png")):
        with Image.open(path) as image:
            print(f"{path.name}: {image.width}x{image.height}")
