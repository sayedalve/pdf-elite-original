import React from "react";
import { PDFEliteFileStub } from "@app/types/fileContext";
import { PrivateContent } from "@app/components/shared/PrivateContent";
import { truncateCenter } from "@app/utils/textUtils";

interface FileEditorFileNameProps {
  file: PDFEliteFileStub;
  maxLength?: number;
}

const FileEditorFileName = ({
  file,
  maxLength = 40,
}: FileEditorFileNameProps) => (
  <PrivateContent>{truncateCenter(file.name, maxLength)}</PrivateContent>
);

export default FileEditorFileName;
