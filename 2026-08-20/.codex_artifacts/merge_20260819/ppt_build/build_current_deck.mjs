import fs from "node:fs/promises";
import path from "node:path";
import { createRequire } from "node:module";

const require = createRequire(import.meta.url);
const { FileBlob, PresentationFile } = require("@oai/artifact-tool");

const SOURCE = "D:/RA/second/rainy/.codex_artifacts/merge_20260819/source_deck.pptx";
const OUT = "D:/RA/second/rainy/.codex_artifacts/merge_20260819/ppt_final_preview/export_payload.txt";
const PREVIEW = "D:/RA/second/rainy/.codex_artifacts/merge_20260819/ppt_final_preview";

async function saveBlob(filePath, blob) {
  await fs.mkdir(path.dirname(filePath), { recursive: true });
  await fs.writeFile(filePath, new Uint8Array(await blob.arrayBuffer()));
}

function replaceText(presentation, id, oldText, newText, style = null) {
  const shape = presentation.resolve(id);
  shape.text.replace(oldText, newText);
  if (style) shape.text.style = { ...shape.text.style, ...style };
  return shape;
}

function addNode(slide, name, position, text, fill, line, color = "#0f172a", fontSize = 15) {
  const node = slide.shapes.add({
    geometry: "roundRect",
    name,
    position,
    fill,
    line: { style: "solid", fill: line, width: 1.4 },
    borderRadius: 9,
  });
  node.text = text;
  node.text.style = {
    typeface: "Microsoft YaHei",
    fontSize,
    bold: true,
    color,
    alignment: "center",
    verticalAlignment: "middle",
    autoFit: "shrinkText",
    insets: { left: 8, right: 8, top: 6, bottom: 6 },
  };
  return node;
}

async function main() {
  await fs.rm(PREVIEW, { recursive: true, force: true });
  await fs.mkdir(PREVIEW, { recursive: true });
  await fs.mkdir(path.dirname(OUT), { recursive: true });

  const presentation = await PresentationFile.importPptx(await FileBlob.load(SOURCE));

  replaceText(presentation, "sh/547294r6", "智能垃圾分类分拣系统", "RA8P1 异构边缘智能与智能分拣平台", {
    fontSize: 42,
    autoFit: "shrinkText",
  });
  replaceText(presentation, "sh/k3yl0zql", "基于瑞萨 RA8P1 的单芯片视觉控制方案", "M85/M33 双核协同 · Ethos-U55 NPU · SDRAM", {
    fontSize: 22,
    autoFit: "shrinkText",
  });
  replaceText(presentation, "sh/7qp4be9c", "感知 · 推理 · 交互 · 执行", "感知 · 推理 · 服务 · 执行 · 数据回流");

  replaceText(presentation, "sh/9072xkry", "RA8P1 完成视觉、交互与机电控制", "RA8P1 双核协同完成视觉、服务与机电控制", { autoFit: "shrinkText" });
  replaceText(presentation, "sh/x4r21kru", "瑞萨 RA8P1", "Cortex-M85 / CPU0");
  replaceText(presentation, "sh/w3i1sfa9", "单芯片构建完整智能分拣控制核心", "视觉采集、NPU调度、显示、存储与分拣状态机");
  replaceText(presentation, "sh/ove9o7yd", "Cortex-M85", "Cortex-M33 / CPU1");
  replaceText(presentation, "sh/9wnqhczy", "实时任务调度与机电状态机", "触摸、称重、健康监测与掉线降级");
  replaceText(presentation, "sh/wjy9sry9", "Ethos-U55 NPU", "Ethos-U55 + SDRAM");
  replaceText(presentation, "sh/xk7qlczu", "192×192 int8 模型离线推理", "192×192 int8 推理；128 MB 外部内存已可用");
  replaceText(presentation, "sh/cnupgny5", "丰富外设接口", "32 位 IPC 通信");
  replaceText(presentation, "sh/do3q9szq", "摄像头、屏幕、触摸、称重与电机", "跨核只传触摸、称重、健康与心跳小数据");
  replaceText(presentation, "sh/s72xofmh", "视觉识别结果直接进入分拣控制状态机", "图像留在 M85 侧，M33 独立服务，避免跨核搬运大帧");

  replaceText(presentation, "sh/cza94vmx", "一颗芯片串起感知、推理、执行与交互", "M85、M33 与 Ethos-U55 构成异构边缘智能闭环", { autoFit: "shrinkText" });
  replaceText(presentation, "sh/pcjqtg36", "OV5640 / CEU", "OV5640 / CEU");
  replaceText(presentation, "sh/m5cra54z", "Ethos-U55 / YOLO", "Ethos-U55 / YOLO");
  replaceText(presentation, "sh/n6ls3alk", "状态机 / PWM / UART", "IPC / M33 服务");
  replaceText(presentation, "sh/n2l4fq98", "屏幕 · 触摸 · TF卡", "SDRAM · TF卡 · 执行器");

  const slide3 = presentation.slides.items[2];
  const cover = slide3.shapes.add({
    geometry: "rect",
    name: "dual-core-diagram-cover",
    position: { left: 293, top: 215, width: 702, height: 350 },
    fill: "#ffffff",
    line: { style: "solid", fill: "#ffffff", width: 0 },
  });

  const camera = addNode(slide3, "camera-node", { left: 320, top: 258, width: 132, height: 62 }, "OV5640\nCEU 采集", "#eff6ff", "#60a5fa", "#1e3a8a", 14);
  const display = addNode(slide3, "display-node", { left: 320, top: 404, width: 132, height: 62 }, "ILI9488\n屏幕显示", "#ecfdf5", "#34d399", "#065f46", 14);
  const m85 = addNode(slide3, "m85-node", { left: 500, top: 245, width: 180, height: 98 }, "Cortex-M85 / CPU0\n图像 · NPU · 状态机\nSDHI / FatFs", "#e0f2fe", "#0284c7", "#0c4a6e", 15);
  const ethos = addNode(slide3, "ethosu-node", { left: 731, top: 253, width: 145, height: 82 }, "Ethos-U55 NPU\n192×192 int8", "#fff7ed", "#fb923c", "#9a3412", 15);
  const ipc = addNode(slide3, "ipc-node", { left: 535, top: 361, width: 110, height: 48 }, "IPC CH0\n32 bit", "#f8fafc", "#64748b", "#334155", 13);
  const m33 = addNode(slide3, "m33-node", { left: 500, top: 432, width: 180, height: 88 }, "Cortex-M33 / CPU1\n触摸 · 称重\n健康监测", "#ecfdf5", "#10b981", "#065f46", 15);
  const storage = addNode(slide3, "storage-node", { left: 731, top: 404, width: 145, height: 62 }, "128 MB SDRAM\nTF 卡拍照", "#f5f3ff", "#8b5cf6", "#5b21b6", 14);
  const act = addNode(slide3, "actuator-node", { left: 883, top: 474, width: 92, height: 52 }, "步进 / 舵机\n执行机构", "#fff7ed", "#fb923c", "#9a3412", 13);

  const conn = (from, to, fromSide, toSide, fill = "#475569", dashed = false) => {
    const connector = slide3.shapes.connect(from, to, {
      kind: "straight",
      fromSide,
      toSide,
      line: { style: dashed ? "dashed" : "solid", fill, width: 2 },
      tail: { type: "triangle", width: "sm", length: "sm" },
    });
    connector.bringToFront();
    return connector;
  };
  conn(camera, m85, "right", "left", "#2563eb");
  conn(m85, display, "left", "right", "#10b981");
  conn(m85, ethos, "right", "left", "#f97316");
  conn(m85, ipc, "bottom", "top", "#64748b", true);
  conn(ipc, m33, "bottom", "top", "#64748b", true);
  conn(m85, storage, "right", "left", "#7c3aed");
  conn(m33, act, "right", "left", "#10b981");
  [camera, display, m85, ethos, ipc, m33, storage, act].forEach((node) => node.bringToFront());
  const dualCoreCaption = slide3.shapes.add({
    geometry: "textbox",
    name: "dual-core-caption",
    position: { left: 392, top: 535, width: 510, height: 24 },
    fill: "none",
    line: { style: "solid", fill: "none", width: 0 },
  });
  dualCoreCaption.text = "视觉主链路与服务链路解耦，IPC 只传小数据";
  dualCoreCaption.text.style = {
    typeface: "Microsoft YaHei",
    fontSize: 13,
    bold: true,
    color: "#334155",
    alignment: "center",
    verticalAlignment: "middle",
  };

  replaceText(presentation, "sh/1cj2d8b6", "192×192 量化模型完成四类离线识别", "192×192 量化模型只锁定最高置信度目标");
  replaceText(presentation, "sh/sna103ap", "减少多目标干扰，让控制对象唯一", "显示阈值 0.35，执行阈值 0.45，连续 3 周期确认");
  replaceText(presentation, "sh/oz29krqh", "本地推理 · 无需云端 · 结果直接驱动执行", "本地推理 · 最高目标唯一 · 识别结果直接进入控制闭环");

  replaceText(presentation, "sh/dgbulwnm", "识别结果被转化为可重复的机械动作", "识别结果经过对齐、锁定和清场才驱动机构");
  replaceText(presentation, "sh/032tgr6d", "连续帧确认类别与置信度", "最高目标连续 3 周期确认");
  replaceText(presentation, "sh/x4vedgvm", "根据目标位置修正传送带", "判定线 160 ± 12 px 双向对齐");
  replaceText(presentation, "sh/58vehgvy", "步进定位，舵机完成分类", "前进、推料、回位与清场闭环");
  replaceText(presentation, "sh/tcfel0vu", "防重复：锁定一次目标，完成 RETURN / CLEAR 后再重新识别", "非阻塞状态机持续刷新相机、触摸与健康监测；RETURN / CLEAR 后才重新识别");

  replaceText(presentation, "sh/0fy5k3id", "RA8P1拓展板与电源供电板PCB设计", "自制扩展板统一引出视觉、交互、存储与执行器接口");

  replaceText(presentation, "sh/cb2tkvap", "让 RA8P1 同时成为眼睛、大脑和控制器", "垃圾分拣是载体，RA8P1 系统能力才是核心", { autoFit: "shrinkText" });
  replaceText(presentation, "sh/dcbud0ra", "从识别到执行，再到现场数据回流，形成完整可演示闭环。", "从像素到动作，再到现场数据回流，形成完整可演示闭环。");
  replaceText(presentation, "sh/kzmdova1", "1", "2");
  replaceText(presentation, "sh/l0vuh0rm", "颗 RA8P1", "核协同");
  replaceText(presentation, "sh/eh0ba1sr", "统一视觉推理与整机控制", "M85 视觉主核 + M33 服务核");
  const sdramMetric = replaceText(presentation, "sh/a9ojq5kz", "4", "128", {
    fontSize: 48,
    autoFit: "shrinkText",
  });
  sdramMetric.position = { left: 476, top: 280, width: 112, height: 78 };
  replaceText(presentation, "sh/xcz2l03q", "类垃圾", "MB SDRAM");
  replaceText(presentation, "sh/cbq1sv25", "最高置信度目标离线识别", "已完成初始化与读写验证");
  replaceText(presentation, "sh/f2d0b6lc", "触摸、称重、执行与 TF 采集", "触摸、称重、执行与 TF 数据闭环");
  replaceText(presentation, "sh/jedkn654", "谢谢各位评委老师", "可迁移到零件、农产品与缺陷检测等边缘智能场景");
  replaceText(presentation, "sh/4fm1wb6p", "06", "07");

  const notes = [
    "我们没有把作品只定义为智能垃圾桶，而是把垃圾分拣作为 RA8P1 异构边缘智能能力的验证载体。\n[Sources]\n- 当前本地 RA8P1 双核工程与构建记录。",
    "CPU0 的 M85 负责视觉主链路，CPU1 的 M33 负责低带宽服务；Ethos-U55 执行量化模型，SDRAM 与 TF 卡支撑数据闭环。\n[Sources]\n- 当前 FSP 配置、源代码和链接分区。",
    "图像始终留在 M85 一侧，IPC 通道 0 只传 32 位触摸、称重、健康和心跳消息，这样避免跨核搬运整帧图像。\n[Sources]\n- platform_services.c、m33_services.c 与双核 FSP 配置。",
    "模型输入为 192×192，只保留最高置信度目标。显示阈值 0.35，执行阈值 0.45，并要求连续 3 周期稳定。\n[Sources]\n- 当前视觉后处理和机器状态机参数。",
    "执行前先对齐判定线，再锁定目标，完成输送、推料、回位和清场。目标短暂丢失不会立即重新触发，避免重复分拣。\n[Sources]\n- 当前非阻塞 machine_control 状态机。",
    "扩展板按设备独立端子引出摄像头、屏幕、触摸、称重、舵机和步进接口，并采用统一信号地。\n[Sources]\n- 当前扩展板原理图、PCB 与现场接线文档。",
    "当前 CPU0 和 CPU1 均已编译通过，SDRAM 已完成初始化和读写，TF 卡能够按类别保存 192×192 图片。下一步重点是现场标定与长时间稳定性验证。\n[Sources]\n- 2026-08-20 双核构建记录与当前工程实现。",
  ];
  presentation.slides.items.forEach((slide, index) => {
    slide.speakerNotes.textFrame.setText(notes[index]);
    slide.speakerNotes.setVisible(true);
  });

  for (let i = 0; i < presentation.slides.items.length; i += 1) {
    const slide = presentation.slides.items[i];
    const stem = `slide-${String(i + 1).padStart(2, "0")}`;
    await saveBlob(path.join(PREVIEW, `${stem}.png`), await presentation.export({ slide, format: "png", scale: 1.5 }));
    const layout = await slide.export({ format: "layout" });
    await fs.writeFile(path.join(PREVIEW, `${stem}.layout.json`), await layout.text(), "utf8");
  }

  const inspect = await presentation.inspect({
    kind: "deck,slide,textbox,shape,image,notes,layout",
    include: "id,slide,name,title,text,textPreview,textChars,textLines,bbox,bboxUnit,alt",
    maxChars: 200000,
  });
  await fs.writeFile(path.join(PREVIEW, "final-inspect.ndjson"), inspect.ndjson, "utf8");
  const pptx = await PresentationFile.exportPptx(presentation);
  await fs.writeFile(OUT, Buffer.from(pptx.data).toString("base64"), "ascii");
  console.log(OUT);
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
