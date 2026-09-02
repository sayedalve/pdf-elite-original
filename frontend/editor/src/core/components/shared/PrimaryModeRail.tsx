/* eslint-disable */
import React from "react";
import { Box, Stack, Tooltip, UnstyledButton, Text } from "@mantine/core";
import { useNavigate } from "react-router-dom";
import { useTranslation } from "react-i18next";
import { useWorkbenchBar } from "@app/contexts/WorkbenchBarContext";
import {
  useNavigationActions,
  useNavigationState,
} from "@app/contexts/NavigationContext";
import HomeRoundedIcon from "@mui/icons-material/HomeRounded";
import VisibilityRoundedIcon from "@mui/icons-material/VisibilityRounded";
import CreateRoundedIcon from "@mui/icons-material/CreateRounded";
import EditRoundedIcon from "@mui/icons-material/EditRounded";
import AutoAwesomeMosaicRoundedIcon from "@mui/icons-material/AutoAwesomeMosaicRounded";

export function PrimaryModeRail() {
  const { t } = useTranslation();
  const navigate = useNavigate();
  const { activeMode, setActiveMode } = useWorkbenchBar();
  const { actions: navActions } = useNavigationActions();
  const { workbench } = useNavigationState();

  const handleModeClick = React.useCallback(
    (mode: string) => {
      setActiveMode(mode);
      if (mode === "organize") {
        navActions.setWorkbench("pageEditor");
      } else if (workbench === "pageEditor") {
        navActions.setWorkbench("viewer");
      }
    },
    [navActions, setActiveMode, workbench],
  );

  React.useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key !== "Escape" || e.defaultPrevented) return;

      const el = document.activeElement;
      if (
        el &&
        (el.tagName === "INPUT" ||
          el.tagName === "TEXTAREA" ||
          (el as HTMLElement).isContentEditable)
      ) {
        return;
      }

      // Check if a modal/dropdown/popover is open.
      if (
        document.querySelector(
          '[role="dialog"], [role="menu"], [data-radix-popper-content-wrapper], .mantine-Popover-dropdown, .mantine-Modal-content',
        )
      ) {
        return;
      }

      if (activeMode !== "view" && activeMode !== "home") {
        e.preventDefault();
        handleModeClick("view");
      }
    };

    window.addEventListener("keydown", handleKeyDown);
    return () => window.removeEventListener("keydown", handleKeyDown);
  }, [activeMode, handleModeClick]);

  const modes = [
    {
      id: "home",
      icon: <HomeRoundedIcon sx={{ fontSize: "1.5rem" }} />,
      label: t("navigation.home", "Home"),
      onClick: () => navigate("/"),
    },
    {
      id: "view",
      icon: <VisibilityRoundedIcon sx={{ fontSize: "1.5rem" }} />,
      label: t("navigation.view", "View"),
      onClick: () => handleModeClick("view"),
    },
    {
      id: "annotate",
      icon: <CreateRoundedIcon sx={{ fontSize: "1.5rem" }} />,
      label: t("navigation.annotate", "Comment"),
      onClick: () => handleModeClick("annotate"),
    },
    {
      id: "edit",
      icon: <EditRoundedIcon sx={{ fontSize: "1.5rem" }} />,
      label: t("navigation.edit", "Edit"),
      onClick: () => handleModeClick("edit"),
    },
    {
      id: "organize",
      icon: <AutoAwesomeMosaicRoundedIcon sx={{ fontSize: "1.5rem" }} />,
      label: t("navigation.organize", "Organize"),
      onClick: () => handleModeClick("organize"),
    },
  ];

  return (
    <Box
      w={64}
      h="100%"
      style={{
        backgroundColor: "var(--c-bg-raised, var(--p-doc-surface-chrome))",
        borderRight:
          "1px solid var(--c-border, var(--p-doc-surface-hover-selected))",
        display: "flex",
        flexDirection: "column",
        alignItems: "center",
        paddingTop: "0.75rem",
        paddingBottom: "0.75rem",
        zIndex: 100,
        flexShrink: 0,
      }}
    >
      <Stack gap="0.25rem" align="center" style={{ width: "100%" }}>
        {modes.map((m) => {
          const isActive = activeMode === m.id;
          return (
            <Tooltip
              key={m.id}
              label={m.label}
              position="right"
              withArrow
              offset={8}
            >
              <UnstyledButton
                onClick={m.onClick}
                style={{
                  display: "flex",
                  flexDirection: "column",
                  alignItems: "center",
                  justifyContent: "center",
                  width: 52,
                  height: 52,
                  borderRadius: "10px",
                  color: isActive
                    ? "var(--c-accent, var(--p-doc-accent-primary))"
                    : "var(--c-text-muted, var(--p-doc-text-secondary))",
                  backgroundColor: isActive
                    ? "color-mix(in srgb, var(--c-accent, var(--p-doc-accent-primary)) 14%, transparent)"
                    : "transparent",
                  transition: "background-color 0.15s ease, color 0.15s ease",
                  position: "relative",
                }}
                onMouseEnter={(e) => {
                  if (!isActive) {
                    e.currentTarget.style.backgroundColor =
                      "var(--c-hover, var(--p-doc-surface-hover-selected))";
                    e.currentTarget.style.color =
                      "var(--c-text, var(--p-doc-text-primary))";
                  }
                }}
                onMouseLeave={(e) => {
                  if (!isActive) {
                    e.currentTarget.style.backgroundColor = "transparent";
                    e.currentTarget.style.color =
                      "var(--c-text-muted, var(--p-doc-text-secondary))";
                  }
                }}
              >
                {/* Active left-edge indicator */}
                {isActive && (
                  <span
                    style={{
                      position: "absolute",
                      left: -6,
                      top: "50%",
                      transform: "translateY(-50%)",
                      width: 3,
                      height: 20,
                      borderRadius: "0 2px 2px 0",
                      backgroundColor:
                        "var(--c-accent, var(--p-doc-accent-primary))",
                    }}
                  />
                )}
                {m.icon}
                <Text
                  size="0.6rem"
                  fw={isActive ? 600 : 500}
                  mt={3}
                  style={{ userSelect: "none", lineHeight: 1 }}
                >
                  {m.label}
                </Text>
              </UnstyledButton>
            </Tooltip>
          );
        })}
      </Stack>
    </Box>
  );
}
