package pdfelite.software.SPDF.controller.api;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.pdfbox.pdmodel.PDDocument;
import org.apache.pdfbox.pdmodel.PDDocumentCatalog;
import org.apache.pdfbox.pdmodel.PDPage;
import org.apache.pdfbox.pdmodel.PDPageTree;
import org.apache.pdfbox.pdmodel.interactive.documentnavigation.destination.PDPageFitDestination;
import org.apache.pdfbox.pdmodel.interactive.documentnavigation.outline.PDDocumentOutline;
import org.apache.pdfbox.pdmodel.interactive.documentnavigation.outline.PDOutlineItem;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.core.io.FileSystemResource;
import org.springframework.core.io.Resource;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.multipart.MultipartFile;

import lombok.extern.slf4j.Slf4j;

import pdfelite.software.SPDF.model.api.EditTableOfContentsRequest;
import pdfelite.software.common.service.CustomPDFDocumentFactory;
import pdfelite.software.common.util.TempFile;
import pdfelite.software.common.util.TempFileManager;
import tools.jackson.core.type.TypeReference;
import tools.jackson.databind.ObjectMapper;

/**
 * Provides extract and edit operations for PDF table-of-contents (bookmark outline).
 *
 * <ul>
 *   <li>{@link #extractBookmarks(MultipartFile)} – returns the current outline as a JSON-friendly
 *       list of maps.
 *   <li>{@link #editTableOfContents(EditTableOfContentsRequest)} – replaces or appends bookmarks
 *       and returns the modified PDF.
 * </ul>
 */
@Slf4j
public class EditTableOfContentsController {

    /**
     * DTO for a single bookmark entry. Uses explicit getters/setters so that both Jackson
     * deserialisation and Mockito {@code @InjectMocks} work without Lombok on the test classpath.
     */
    public static class BookmarkItem {
        private String title;
        private int pageNumber;
        private List<BookmarkItem> children;

        public BookmarkItem() {
            this.children = new ArrayList<>();
        }

        public String getTitle() {
            return title;
        }

        public void setTitle(String title) {
            this.title = title;
        }

        public int getPageNumber() {
            return pageNumber;
        }

        public void setPageNumber(int pageNumber) {
            this.pageNumber = pageNumber;
        }

        public List<BookmarkItem> getChildren() {
            return children;
        }

        public void setChildren(List<BookmarkItem> children) {
            this.children = children == null ? new ArrayList<>() : children;
        }
    }

    private final CustomPDFDocumentFactory pdfDocumentFactory;
    private final ObjectMapper objectMapper;
    private final TempFileManager tempFileManager;

    @Autowired
    public EditTableOfContentsController(
            CustomPDFDocumentFactory pdfDocumentFactory,
            ObjectMapper objectMapper,
            TempFileManager tempFileManager) {
        this.pdfDocumentFactory = pdfDocumentFactory;
        this.objectMapper = objectMapper;
        this.tempFileManager = tempFileManager;
    }

    // ─── extract ─────────────────────────────────────────────────────────────

    /**
     * Reads the PDF's document outline and returns a flat list of top-level bookmarks, each
     * carrying its title, 1-based page number, and a (possibly empty) list of child bookmarks.
     *
     * @param file the uploaded PDF
     * @return HTTP 200 with the bookmark list; empty list when the PDF has no outline
     */
    public ResponseEntity<List<Map<String, Object>>> extractBookmarks(MultipartFile file)
            throws IOException {
        PDDocument doc = null;
        try {
            doc = pdfDocumentFactory.load(file);
            PDDocumentCatalog catalog = doc.getDocumentCatalog();
            PDDocumentOutline outline = catalog == null ? null : catalog.getDocumentOutline();
            if (outline == null) {
                return ResponseEntity.ok(new ArrayList<>());
            }

            List<Map<String, Object>> result = collectOutlineItems(doc, outline.getFirstChild());
            return ResponseEntity.ok(result);
        } finally {
            closeQuietly(doc);
        }
    }

    /** Recursively collect outline items starting from {@code item}. */
    private List<Map<String, Object>> collectOutlineItems(PDDocument doc, PDOutlineItem item)
            throws IOException {
        List<Map<String, Object>> list = new ArrayList<>();
        while (item != null) {
            Map<String, Object> entry = new HashMap<>();
            String title = item.getTitle();
            entry.put("title", title == null ? "" : title);

            PDPage destPage = item.findDestinationPage(doc);
            int pageIndex = 0;
            if (destPage != null) {
                PDPageTree pages = doc.getPages();
                pageIndex = pages.indexOf(destPage);
                if (pageIndex < 0) pageIndex = 0;
            }
            entry.put("pageNumber", pageIndex + 1);

            // Recurse into children
            List<Map<String, Object>> children = collectOutlineItems(doc, item.getFirstChild());
            entry.put("children", children);

            list.add(entry);
            item = item.getNextSibling();
        }
        return list;
    }

    // ─── edit ─────────────────────────────────────────────────────────────────

    /**
     * Replaces or appends bookmarks in the PDF according to the supplied JSON bookmark data.
     *
     * @param request contains the PDF, the JSON bookmark tree, and a replaceExisting flag
     * @return HTTP 200 with the modified PDF as an octet-stream
     */
    public ResponseEntity<Resource> editTableOfContents(EditTableOfContentsRequest request)
            throws Exception {
        PDDocument doc = null;
        TempFile tempFile = null;
        try {
            doc = pdfDocumentFactory.load(request.getFileInput());

            List<BookmarkItem> bookmarks =
                    objectMapper.readValue(
                            request.getBookmarkData(), new TypeReference<List<BookmarkItem>>() {});

            PDDocumentCatalog catalog = doc.getDocumentCatalog();

            PDDocumentOutline outline;
            boolean replace = request.getReplaceExisting() == null || request.getReplaceExisting();
            if (replace || catalog.getDocumentOutline() == null) {
                outline = new PDDocumentOutline();
            } else {
                outline = catalog.getDocumentOutline();
            }

            for (BookmarkItem bookmark : bookmarks) {
                PDOutlineItem outlineItem = createOutlineItem(doc, bookmark);
                outline.addLast(outlineItem);
            }

            catalog.setDocumentOutline(outline);

            tempFile = tempFileManager.createManagedTempFile(".pdf");
            File outputFile = tempFile.getFile();
            doc.save(outputFile);

            Resource resource = new FileSystemResource(outputFile);
            return ResponseEntity.ok()
                    .contentType(MediaType.APPLICATION_OCTET_STREAM)
                    .header("Content-Disposition", "attachment; filename=\"modified.pdf\"")
                    .body(resource);
        } finally {
            closeQuietly(doc);
        }
    }

    /**
     * Creates a {@link PDOutlineItem} for {@code bookmark} and recursively adds its children. Page
     * numbers are clamped to [1, numberOfPages].
     */
    private PDOutlineItem createOutlineItem(PDDocument doc, BookmarkItem bookmark) {
        PDOutlineItem item = new PDOutlineItem();
        item.setTitle(bookmark.getTitle() == null ? "" : bookmark.getTitle());

        int numPages = doc.getNumberOfPages();
        int pageIndex = Math.max(0, Math.min(bookmark.getPageNumber() - 1, numPages - 1));
        PDPage page = doc.getPage(pageIndex);
        PDPageFitDestination dest = new PDPageFitDestination();
        dest.setPage(page);
        item.setDestination(dest);

        if (bookmark.getChildren() != null) {
            for (BookmarkItem child : bookmark.getChildren()) {
                item.addLast(createOutlineItem(doc, child));
            }
        }
        return item;
    }

    private void closeQuietly(PDDocument doc) {
        if (doc != null) {
            try {
                doc.close();
            } catch (Exception ignored) {
            }
        }
    }
}
