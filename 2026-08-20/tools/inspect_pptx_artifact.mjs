import fs from "node:fs/promises";
import path from "node:path";
import { createRequire } from "node:module";

const require = createRequire(import.meta.url);
const { FileBlob, PresentationFile } = require("@oai/artifact-tool");

async function saveBlob(filePath, blob) {
  await fs.mkdir(path.dirname(filePath), { recursive: true });
  const bytes = new Uint8Array(await blob.arrayBuffer());
  await fs.writeFile(filePath, bytes);
}

async function main() {
  const [sourcePath, outputDir] = process.argv.slice(2);
  if (!sourcePath || !outputDir) throw new Error("usage: source.pptx output-dir");
  await fs.rm(outputDir, { recursive: true, force: true });
  await fs.mkdir(outputDir, { recursive: true });

  const presentation = await PresentationFile.importPptx(await FileBlob.load(sourcePath));
  const snapshot = await presentation.inspect({
    kind: "deck,slide,textbox,shape,image,table,chart,notes,thread,layout",
    include: "id,slide,name,title,text,textPreview,textChars,textLines,bbox,bboxUnit,alt,isPlaceholder,placeholders",
    maxChars: 200000,
  });
  await fs.writeFile(path.join(outputDir, "template-inspect.ndjson"), snapshot.ndjson, "utf8");

  const slides = presentation.slides.items;
  for (let index = 0; index < slides.length; index += 1) {
    const stem = `slide-${String(index + 1).padStart(2, "0")}`;
    await saveBlob(path.join(outputDir, `${stem}.png`), await presentation.export({ slide: slides[index], format: "png", scale: 1.5 }));
    const layout = await slides[index].export({ format: "layout" });
    await fs.writeFile(path.join(outputDir, `${stem}.layout.json`), await layout.text(), "utf8");
  }
  await saveBlob(path.join(outputDir, "deck-montage.webp"), await presentation.export({ format: "webp", montage: true, scale: 1 }));
  await fs.writeFile(path.join(outputDir, "manifest.json"), JSON.stringify({ slideCount: slides.length }, null, 2), "utf8");
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
