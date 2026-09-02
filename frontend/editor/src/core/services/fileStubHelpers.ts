import { PDFEliteFile, PDFEliteFileStub } from "@app/types/fileContext";
import {
  createChildStub,
  generateProcessedFileMetadata,
} from "@app/contexts/file/fileActions";
import { createPDFEliteFile } from "@app/types/fileContext";
import { ToolId } from "@app/types/toolId";

/**
 * Create PDFEliteFiles and PDFEliteFileStubs from exported files
 * Used when saving page editor changes to create version history
 */
export async function createPDFEliteFilesAndStubs(
  files: File[],
  parentStub: PDFEliteFileStub,
  toolId: ToolId,
): Promise<{ PDFEliteFiles: PDFEliteFile[]; stubs: PDFEliteFileStub[] }> {
  const PDFEliteFiles: PDFEliteFile[] = [];
  const stubs: PDFEliteFileStub[] = [];

  for (const file of files) {
    const processedFileMetadata = await generateProcessedFileMetadata(file);
    const childStub = createChildStub(
      parentStub,
      { toolId, timestamp: Date.now() },
      file,
      processedFileMetadata?.thumbnailUrl,
      processedFileMetadata,
    );

    const PDFEliteFile = createPDFEliteFile(file, childStub.id);
    PDFEliteFiles.push(PDFEliteFile);
    stubs.push(childStub);
  }

  return { PDFEliteFiles, stubs };
}
