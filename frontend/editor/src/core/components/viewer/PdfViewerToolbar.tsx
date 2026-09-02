import { useState, useEffect } from "react";
import { Paper, NumberInput } from "@mantine/core";
import { useTranslation } from "react-i18next";
import { useViewer } from "@app/contexts/ViewerContext";
import { useIsPhone } from "@app/hooks/useIsMobile";
import { ActionIcon } from "@app/ui/ActionIcon";
import "@app/components/viewer/PdfViewerToolbar.css";
import FirstPageIcon from "@mui/icons-material/FirstPage";
import ArrowBackIosIcon from "@mui/icons-material/ArrowBackIos";
import ArrowForwardIosIcon from "@mui/icons-material/ArrowForwardIos";
import LastPageIcon from "@mui/icons-material/LastPage";

// Sizing constants for the page number input
const MIN_PAGE_DIGITS = 2;
const MIN_INPUT_WIDTH_PX = 48;
const BASE_INPUT_WIDTH_PX = 32;
const PX_PER_DIGIT = 8;

interface PdfViewerToolbarProps {
  // Page navigation props (placeholders for now)
  currentPage?: number;
  totalPages?: number;
  onPageChange?: (page: number) => void;
}

export function PdfViewerToolbar({
  currentPage = 1,
  onPageChange,
}: PdfViewerToolbarProps) {
  const { t } = useTranslation();
  const isPhone = useIsPhone();
  const buttonMinWidth = isPhone ? "3rem" : "2.5rem";
  const buttonSize = isPhone ? "lg" : "md";
  const { getScrollState, scrollActions, registerImmediateScrollUpdate } =
    useViewer();

  const scrollState = getScrollState();
  const [pageInput, setPageInput] = useState(
    scrollState.currentPage || currentPage,
  );

  // Register for immediate scroll updates and sync with actual scroll state
  useEffect(() => {
    const unregister = registerImmediateScrollUpdate(
      (currentPage, _totalPages) => {
        setPageInput(currentPage);
      },
    );
    setPageInput(scrollState.currentPage);
    return () => {
      unregister?.();
    };
  }, [registerImmediateScrollUpdate, scrollState.currentPage]);

  const handlePageNavigation = (page: number) => {
    scrollActions.scrollToPage(page);
    if (onPageChange) {
      onPageChange(page);
    }
    setPageInput(page);
  };

  const handleFirstPage = () => {
    scrollActions.scrollToFirstPage();
  };

  const handlePreviousPage = () => {
    const { currentPage: cur } = getScrollState();
    if (cur > 1) scrollActions.scrollToPage(cur - 1);
  };

  const handleNextPage = () => {
    const { currentPage: cur, totalPages: tot } = getScrollState();
    if (cur < tot) scrollActions.scrollToPage(cur + 1);
  };

  const handleLastPage = () => {
    scrollActions.scrollToLastPage();
  };

  const totalPagesDigits = Math.max(
    MIN_PAGE_DIGITS,
    (scrollState.totalPages || 1).toString().length,
  );
  const inputWidth = Math.max(
    MIN_INPUT_WIDTH_PX,
    BASE_INPUT_WIDTH_PX + totalPagesDigits * PX_PER_DIGIT,
  );

  return (
    <Paper
      className="pdf-viewer-toolbar"
      p={8}
      pb={8}
      style={{
        display: "flex",
        alignItems: "center",
        flexWrap: "nowrap",
        gap: 8,
        justifyContent: "center",
        pointerEvents: "auto",
        borderRadius: 8,
      }}
    >
      {/* First Page Button */}
      {!isPhone && (
        <ActionIcon
          variant="tertiary"
          size={buttonSize}
          onClick={handleFirstPage}
          disabled={scrollState.currentPage === 1}
          style={{ minWidth: buttonMinWidth }}
          title={t("viewer.firstPage", "First Page")}
          aria-label={t("viewer.firstPage", "First Page")}
        >
          <FirstPageIcon fontSize="small" />
        </ActionIcon>
      )}

      {/* Previous Page Button */}
      <ActionIcon
        variant="tertiary"
        size={buttonSize}
        onClick={handlePreviousPage}
        disabled={scrollState.currentPage === 1}
        style={{ minWidth: buttonMinWidth }}
        title={t("viewer.previousPage", "Previous Page")}
        aria-label={t("viewer.previousPage", "Previous Page")}
      >
        <ArrowBackIosIcon fontSize="small" />
      </ActionIcon>

      {/* Page Input */}
      <NumberInput
        value={pageInput}
        onChange={(value) => {
          const page = Number(value);
          setPageInput(page);
          if (!isNaN(page) && page >= 1 && page <= scrollState.totalPages) {
            handlePageNavigation(page);
          }
        }}
        min={1}
        max={scrollState.totalPages}
        hideControls
        styles={{
          input: {
            width: inputWidth,
            textAlign: "center",
            fontWeight: 500,
            fontSize: 14,
            paddingLeft: 4,
            paddingRight: 4,
            boxSizing: "border-box",
            height: "1.75rem",
            minHeight: "1.75rem",
          },
        }}
      />

      <span
        style={{ fontWeight: 500, fontSize: 14, color: "var(--c-text-subtle)" }}
      >
        / {scrollState.totalPages}
      </span>

      {/* Next Page Button */}
      <ActionIcon
        variant="tertiary"
        size={buttonSize}
        onClick={handleNextPage}
        disabled={scrollState.currentPage === scrollState.totalPages}
        style={{ minWidth: buttonMinWidth }}
        title={t("viewer.nextPage", "Next Page")}
        aria-label={t("viewer.nextPage", "Next Page")}
      >
        <ArrowForwardIosIcon fontSize="small" />
      </ActionIcon>

      {/* Last Page Button */}
      {!isPhone && (
        <ActionIcon
          variant="tertiary"
          size={buttonSize}
          onClick={handleLastPage}
          disabled={scrollState.currentPage === scrollState.totalPages}
          style={{ minWidth: buttonMinWidth }}
          title={t("viewer.lastPage", "Last Page")}
          aria-label={t("viewer.lastPage", "Last Page")}
        >
          <LastPageIcon fontSize="small" />
        </ActionIcon>
      )}
    </Paper>
  );
}
