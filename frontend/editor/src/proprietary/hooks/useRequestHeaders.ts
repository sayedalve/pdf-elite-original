export function useRequestHeaders(): HeadersInit {
  const token = localStorage.getItem("PDFElite_jwt");
  return token ? { Authorization: `Bearer ${token}` } : {};
}
