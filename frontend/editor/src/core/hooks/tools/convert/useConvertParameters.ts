/* eslint-disable */
import { BaseParameters } from "@app/types/parameters";
import {
  BaseParametersHook,
  useBaseParameters,
} from "@app/hooks/tools/shared/useBaseParameters";
import { FIT_OPTIONS } from "@app/constants/convertConstants";
import { detectFileExtension } from "@app/utils/fileUtils";

export interface ImageOptions {
  colorType?: string;
  dpi?: number;
  singleOrMultiple?: string;
  fitOption?: string;
  autoRotate?: boolean;
  combineImages?: boolean;
}

export interface HtmlOptions {
  zoomLevel?: number;
}

export interface EmailOptions {
  includeAttachments?: boolean;
  maxAttachmentSizeMB?: number;
  downloadHtml?: boolean;
  includeAllRecipients?: boolean;
}

export interface PdfaOptions {
  outputFormat?: string;
  strict?: boolean;
}

export interface PdfxOptions {
  outputFormat?: string;
}

export interface CbrOptions {
  optimizeForEbook?: boolean;
}

export interface PdfToCbrOptions {
  dpi?: number;
}

export interface CbzOptions {
  optimizeForEbook?: boolean;
}

export interface CbzOutputOptions {
  dpi?: number;
}

export interface EpubOptions {
  author?: string;
  title?: string;
  detectChapters?: boolean;
  targetDevice?: string;
  outputFormat?: string;
}

export interface EbookOptions {
  embedAllFonts?: boolean;
  includeTableOfContents?: boolean;
  includePageNumbers?: boolean;
  optimizeForEbook?: boolean;
}

export interface ConvertParameters extends BaseParameters {
  fromExtension?: string;
  toExtension?: string;
  isSmartDetection?: boolean;
  smartDetectionType?: string;
  imageOptions?: ImageOptions;
  htmlOptions?: HtmlOptions;
  emailOptions?: EmailOptions;
  pdfaOptions?: PdfaOptions;
  pdfxOptions?: PdfxOptions;
  cbrOptions?: CbrOptions;
  pdfToCbrOptions?: PdfToCbrOptions;
  cbzOptions?: CbzOptions;
  cbzOutputOptions?: CbzOutputOptions;
  epubOptions?: EpubOptions;
  ebookOptions?: EbookOptions;
}

export const defaultParameters: ConvertParameters = {
  fromExtension: "",
  toExtension: "",
  isSmartDetection: true,
  smartDetectionType: "none",
  imageOptions: {
    colorType: "color",
    dpi: 300,
    singleOrMultiple: "multiple",
    fitOption: FIT_OPTIONS.MAINTAIN_ASPECT,
    autoRotate: true,
    combineImages: true,
  },
  htmlOptions: {
    zoomLevel: 1.0,
  },
  emailOptions: {
    includeAttachments: true,
    maxAttachmentSizeMB: 10,
    downloadHtml: false,
    includeAllRecipients: true,
  },
  pdfaOptions: {
    outputFormat: "pdfa2b",
    strict: false,
  },
  pdfxOptions: {
    outputFormat: "pdfx4",
  },
  cbrOptions: {
    optimizeForEbook: false,
  },
  pdfToCbrOptions: {
    dpi: 150,
  },
  cbzOptions: {
    optimizeForEbook: false,
  },
  cbzOutputOptions: {
    dpi: 150,
  },
};

export interface ConvertParametersHook extends BaseParametersHook<ConvertParameters> {
  analyzeFileTypes: (
    files: { name?: string; file?: { name: string } }[],
  ) => void;
}

export const useConvertParameters = (): ConvertParametersHook => {
  const base = useBaseParameters({
    defaultParameters,
    endpointName: "convert",
  });

  const analyzeFileTypes = (
    files: { name?: string; file?: { name: string } }[],
  ) => {
    if (!files.length) return;

    let allSame = true;
    let firstExt = "";

    const exts = files.map((f) => {
      const filename = f.name || (f.file && f.file.name) || "";
      return detectFileExtension(filename);
    });

    firstExt = exts[0];
    const isWeb = exts.every((ext) => ["html", "htm", "zip"].includes(ext));

    for (let i = 1; i < exts.length; i++) {
      if (exts[i] !== firstExt) {
        allSame = false;
        break;
      }
    }

    const isImage = exts.every((ext) =>
      ["jpg", "jpeg", "png", "gif", "webp", "bmp"].includes(ext),
    );

    if (isWeb) {
      base.updateParameter("fromExtension", "html");
      base.updateParameter("toExtension", "pdf");
      base.updateParameter("isSmartDetection", true);
      base.updateParameter("smartDetectionType", "web");
    } else if (isImage && !allSame) {
      base.updateParameter("fromExtension", "image");
      base.updateParameter("toExtension", "pdf");
      base.updateParameter("isSmartDetection", true);
      base.updateParameter("smartDetectionType", "images");
    } else if (allSame && firstExt) {
      if (["docx", "doc", "rtf", "odt", "txt"].includes(firstExt)) {
        base.updateParameter("fromExtension", firstExt);
        base.updateParameter("toExtension", "pdf");
        base.updateParameter("isSmartDetection", false);
      } else if (
        ["jpg", "jpeg", "png", "gif", "webp", "bmp"].includes(firstExt)
      ) {
        base.updateParameter("fromExtension", "img");
        base.updateParameter("toExtension", "pdf");
        base.updateParameter("isSmartDetection", false);
      } else {
        base.updateParameter("fromExtension", "file-" + firstExt);
        base.updateParameter("toExtension", "pdf");
        base.updateParameter("isSmartDetection", false);
      }
    } else {
      // Mixed or unknown
      base.updateParameter("fromExtension", "any");
      base.updateParameter("toExtension", "pdf");
      base.updateParameter("isSmartDetection", true);
      base.updateParameter("smartDetectionType", "mixed");
    }
  };

  return {
    ...base,
    analyzeFileTypes,
  };
};
