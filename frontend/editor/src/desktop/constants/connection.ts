/**
 * Connection-related constants for desktop app
 */

// SaaS authentication server URL
export const PDFElite_SAAS_URL: string = import.meta.env.VITE_SAAS_SERVER_URL;

// PDFElite SaaS backend API server (for team endpoints, etc.)
export const PDFElite_SAAS_BACKEND_API_URL: string = import.meta.env
  .VITE_SAAS_BACKEND_API_URL;

// Supabase publishable key — used for SaaS authentication
export const SUPABASE_KEY: string = import.meta.env
  .VITE_SUPABASE_PUBLISHABLE_DEFAULT_KEY;

// Desktop deep link callback for Supabase email confirmations
export const DESKTOP_DEEP_LINK_CALLBACK = "PDFElitepdf://auth/callback";
