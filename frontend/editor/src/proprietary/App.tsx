import { Suspense } from "react";
import { Routes, Route, useParams } from "react-router-dom";
import { AppProviders } from "@app/components/AppProviders";
import { AppLayout } from "@app/components/AppLayout";
import { LoadingFallback } from "@app/components/shared/LoadingFallback";
import { PreferencesProvider } from "@app/contexts/PreferencesContext";
import { ThemeProvider } from "@app/components/shared/ThemeProvider";
import ParticipantView from "@app/components/workflow/ParticipantView";
import Workbench from "@app/components/layout/Workbench";

// Import global styles
import "@app/styles/tailwind.css";
import "@app/styles/cookieconsent.css";
import "@app/styles/index.css";
import "@app/styles/design-tokens.css";
import "@app/styles/globals.css";

// Import file ID debugging helpers (development only)
import "@app/utils/fileIdSafety";

function PublicRouteProviders({ children }: { children: React.ReactNode }) {
  return (
    <PreferencesProvider>
      <ThemeProvider>{children}</ThemeProvider>
    </PreferencesProvider>
  );
}

function ParticipantViewPage() {
  const { token } = useParams<{ token: string }>();
  if (!token) return null;
  return <ParticipantView token={token} />;
}

export default function App() {
  return (
    <Suspense fallback={<LoadingFallback />}>
      <Routes>
        <Route
          path="/workflow/sign/:token"
          element={
            <PublicRouteProviders>
              <ParticipantViewPage />
            </PublicRouteProviders>
          }
        />

        <Route
          path="*"
          element={
            <AppProviders>
              <AppLayout>
                <Workbench />
              </AppLayout>
            </AppProviders>
          }
        />
      </Routes>
    </Suspense>
  );
}
