/// <reference types="vite/client" />
/// <reference types="gapi" />
/// <reference types="gapi.client.drive-v3" />
/// <reference types="google.accounts" />
/// <reference types="google.picker" />

declare module "*.js";
declare module "*.module.css";

// Auto-generated icon set JSON import
declare module "assets/material-symbols-icons.json" {
  const value: {
    prefix: string;
    icons: Record<string, unknown>;
    width?: number;
    height?: number;
  };
  export default value;
}

declare global {
  interface Window {
    __PDFElite_PDF_BASE_URL__?: string;
    PDFElite_PDF_API_BASE_URL?: string;
    endpointAvailabilityService?: unknown;
  }
}

declare module "axios" {
  export interface AxiosRequestConfig<_D = unknown> {
    suppressErrorToast?: boolean;
    skipAuthRedirect?: boolean;
    skipBackendReadyCheck?: boolean;
  }

  export interface InternalAxiosRequestConfig<_D = unknown> {
    suppressErrorToast?: boolean;
    skipAuthRedirect?: boolean;
    skipBackendReadyCheck?: boolean;
  }
}

declare module "pdfjs-dist/build/pdf.mjs";
declare module "pdfjs-dist/build/pdf.worker.min.mjs?url";

export {};
