/* eslint-disable */
import {
  defineCustomTool,
  useToolOperation,
} from "@app/hooks/tools/shared/useToolOperation";
import { objectToFormData } from "@app/hooks/tools/shared/toolApiMapping";
import type { ConvertParameters } from "./useConvertParameters";
import { createStandardErrorHandler } from "@app/utils/toolErrorHandler";
import { useTranslation } from "react-i18next";
import apiClient from "@app/services/apiClient";
import { detectFileExtension } from "@app/utils/fileUtils";

export const convertOperationConfig = defineCustomTool<ConvertParameters>({
  operationType: "convert",
  customProcessor: async (params, files) => {
    if (!files.length) {
      throw new Error("convert.error.noFileSelected");
    }

    const firstExt = detectFileExtension(files[0].name);
    const isImageToPdf =
      ["jpg", "jpeg", "png", "gif", "webp", "bmp"].includes(firstExt) &&
      params.toExtension === "pdf";

    const isPdfToCsv = firstExt === "pdf" && params.toExtension === "csv";
    const isPdfToImg =
      firstExt === "pdf" &&
      ["jpg", "png", "gif", "webp", "bmp", "tiff", "svg"].includes(
        params.toExtension || "",
      );
    const isEmlToPdf = firstExt === "eml" && params.toExtension === "pdf";
    const isPdfToPdfa = firstExt === "pdf" && params.toExtension === "pdfa";
    const isPdfToPdfx = firstExt === "pdf" && params.toExtension === "pdfx";
    const isHtmlToPdf =
      ["html", "htm", "zip"].includes(firstExt) && params.toExtension === "pdf";

    let endpoint = "";
    let optionsToSpread: any = {};

    if (isImageToPdf) {
      endpoint = "/api/v1/convert/img/pdf";
      optionsToSpread = params.imageOptions || {};
    } else if (isPdfToCsv) {
      endpoint = "/api/v1/convert/pdf/csv";
      optionsToSpread = { pageNumbers: "all" };
    } else if (isPdfToImg) {
      endpoint = "/api/v1/convert/pdf/img";
      optionsToSpread = {
        imageFormat: params.toExtension,
        ...(params.imageOptions || {}),
      };
    } else if (isEmlToPdf) {
      endpoint = "/api/v1/convert/eml/pdf";
      optionsToSpread = params.emailOptions || {};
    } else if (isPdfToPdfa) {
      endpoint = "/api/v1/convert/pdf/pdfa";
      optionsToSpread = params.pdfaOptions || {};
    } else if (isPdfToPdfx) {
      endpoint = "/api/v1/convert/pdf/pdfx";
      optionsToSpread = params.pdfxOptions || {};
    } else if (isHtmlToPdf) {
      endpoint = "/api/v1/convert/html/pdf";
      optionsToSpread = { zoom: params.htmlOptions?.zoomLevel };
    } else if (params.toExtension === "pdf") {
      endpoint = "/api/v1/convert/file/pdf";
    } else {
      throw new Error("Unsupported conversion format");
    }

    const combineImages = params.imageOptions?.combineImages !== false;
    const batches: File[][] =
      isImageToPdf && combineImages ? [files] : files.map((f) => [f]);

    const generatedFiles: File[] = [];
    const errors: Error[] = [];

    for (const batch of batches) {
      const firstFile = batch[0];
      const filename = firstFile.name || "";

      try {
        const formData = objectToFormData(optionsToSpread, {
          fileInput: batch,
        });

        const response: any = await apiClient.post(endpoint, formData, {
          responseType: "blob",
        });

        if (!response) {
          throw new Error("Empty response");
        }

        const contentType =
          (response.headers && response.headers["content-type"]) ||
          "application/octet-stream";

        let outName = "";
        const disposition =
          response.headers && response.headers["content-disposition"];
        if (disposition && disposition.includes('filename="')) {
          const match = disposition.match(/filename="([^"]+)"/);
          if (match) {
            outName = match[1];
          }
        }

        if (!outName) {
          outName =
            filename.replace(/\.[^/.]+$/, "") +
            "." +
            (params.toExtension || "pdf");
        }

        const blob =
          response.data instanceof Blob
            ? response.data
            : new Blob([response.data || ""], { type: contentType });
        generatedFiles.push(new File([blob], outName, { type: contentType }));
      } catch (err) {
        if (batches.length > 1) {
          console.warn(`Failed to convert file ${filename}`, err);
        }
        errors.push(err as Error);
      }
    }

    if (generatedFiles.length === 0 && errors.length > 0) {
      throw errors[0];
    }

    return { files: generatedFiles };
  },
});

export const useConvertOperation = () => {
  const { t } = useTranslation();
  return useToolOperation<ConvertParameters>({
    ...convertOperationConfig,
    getErrorMessage: createStandardErrorHandler(
      t("convert.error.failed", "Operation failed."),
    ),
  });
};
