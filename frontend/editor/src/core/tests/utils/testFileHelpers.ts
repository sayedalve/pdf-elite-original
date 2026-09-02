/**
 * Test utilities for creating PDFEliteFile objects in tests
 */

import { PDFEliteFile, createPDFEliteFile } from "@app/types/fileContext";

/**
 * Create a PDFEliteFile object for testing purposes
 */
export function createTestPDFEliteFile(
  name: string,
  content: string = "test content",
  type: string = "application/pdf",
): PDFEliteFile {
  const file = new File([content], name, { type });
  return createPDFEliteFile(file);
}

/**
 * Create multiple PDFEliteFile objects for testing
 */
export function createTestFilesWithId(
  files: Array<{ name: string; content?: string; type?: string }>,
): PDFEliteFile[] {
  return files.map(
    ({ name, content = "test content", type = "application/pdf" }) =>
      createTestPDFEliteFile(name, content, type),
  );
}
