import React, { useState, useEffect } from "react";
import { Stack, Card, Badge, Text, Group } from "@mantine/core";
import { useTranslation } from "react-i18next";
import {
  connectionModeService,
  ConnectionConfig,
} from "@app/services/connectionModeService";
import { authService, UserInfo } from "@app/services/authService";

export const ConnectionSettings: React.FC = () => {
  const { t } = useTranslation();
  const [config, setConfig] = useState<ConnectionConfig | null>(null);
  const [userInfo, setUserInfo] = useState<UserInfo | null>(null);

  // Load current config on mount
  useEffect(() => {
    const loadConfig = async () => {
      const currentConfig = await connectionModeService.getCurrentConfig();
      setConfig(currentConfig);

      if (
        currentConfig.mode === "saas" ||
        currentConfig.mode === "selfhosted"
      ) {
        const user = await authService.getUserInfo();
        setUserInfo(user);
      }
    };

    loadConfig();

    const unsubscribe =
      connectionModeService.subscribeToModeChanges(loadConfig);
    return unsubscribe;
  }, []);

  if (!config) {
    return <Text>{t("common.loading", "Loading...")}</Text>;
  }

  return (
    <>
      <Card shadow="sm" padding="lg" radius="md" withBorder>
        <Stack gap="md">
          <Group justify="space-between">
            <Text fw={600}>
              {t("settings.connection.title", "Connection Mode")}
            </Text>
            <Badge
              color={
                config.mode === "saas"
                  ? "blue"
                  : config.mode === "local"
                    ? "white"
                    : "green"
              }
              variant="light"
            >
              {config.mode === "saas"
                ? t("settings.connection.mode.saas", "PDFElite Cloud")
                : config.mode === "local"
                  ? t("settings.connection.mode.local", "Local Only")
                  : t("settings.connection.mode.selfhosted", "Self-Hosted")}
            </Badge>
          </Group>

          {config.mode === "local" && (
            <Text size="sm" c="dimmed">
              {t(
                "settings.connection.localDescription",
                "You are using the local backend without an account. Some tools requiring cloud processing or a self-hosted server are unavailable.",
              )}
            </Text>
          )}

          {(config.mode === "saas" || config.mode === "selfhosted") &&
            config.server_config && (
              <>
                <div>
                  <Text size="sm" fw={500}>
                    {t("settings.connection.server", "Server")}
                  </Text>
                  <Text size="sm" c="dimmed">
                    {config.mode === "saas"
                      ? "PDFElite.com"
                      : config.server_config.url}
                  </Text>
                </div>

                {userInfo && (
                  <div>
                    <Text size="sm" fw={500}>
                      {t("settings.connection.user", "Logged in as")}
                    </Text>
                    <Text size="sm" c="dimmed">
                      {userInfo.username}
                      {userInfo.email && ` (${userInfo.email})`}
                    </Text>
                  </div>
                )}
              </>
            )}
        </Stack>
      </Card>
    </>
  );
};
