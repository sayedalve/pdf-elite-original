/* eslint-disable */
import { open } from "@tauri-apps/plugin-dialog";

export async function openPdfViaTauri(): Promise<string | null> {
  try {
    const selected = await open({
      multiple: false,
      filters: [{ name: "PDF", extensions: ["pdf"] }],
    });
    if (!selected) return null;
    return typeof selected === "string"
      ? selected
      : Array.isArray(selected)
        ? selected[0]
        : null;
  } catch {
    return null;
  }
}

export function openPdfViaWeb(onFile: (f: File) => void) {
  const input = document.createElement("input");
  input.type = "file";
  input.accept = ".pdf,application/pdf";
  input.onchange = () => {
    const file = input.files?.[0];
    if (file) onFile(file);
  };
  input.click();
}

export async function openPdfUniversal(
  onFile: (f: File) => void,
  onPath?: (path: string) => void,
) {
  // Try Tauri first for true local path
  const path = await openPdfViaTauri();
  if (path && onPath) {
    onPath(path);
    // Create fake File with path for recent docs
    const name = path.split("/").pop()?.split("\\").pop() || "document.pdf";
    const fake = new File([], name, { type: "application/pdf" });
    (fake as any).path = path;
    onFile(fake);
    return;
  }
  if (path) return; // handled
  openPdfViaWeb(onFile);
}
