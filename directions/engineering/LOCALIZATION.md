# Localization

> Engineering doc for PDF Elite native C++/Win32/PDFium rebuild
> Target: File 29 of 31 in engineering documentation set

---

## 1. Current State

### 1.1 Frontend Localization

**FACT:** The web app stores translations as JSON files in `frontend/editor/public/locales/`.

```
frontend/editor/public/locales/
├── en-US/
│   └── translation.json
├── de/
│   └── translation.json
├── fr/
│   └── translation.json
├── es/
│   └── translation.json
├── it/
│   └── translation.json
└── ... (additional languages)
```

### 1.2 Backend Localization (Stirling PDF)

**FACT:** The Java backend uses standard `messages_*.properties` files (Spring Boot resource bundles):

```
src/main/resources/i18n/
├── messages.properties          (English default)
├── messages_de.properties      (German)
├── messages_fr.properties      (French)
├── messages_es.properties      (Spanish)
└── ...
```

### 1.3 Conversion Workflow

**FACT:** A conversion script exists at `scripts/convert_properties_to_json.py` that converts Java `.properties` files to JSON format for the frontend.

```
Java .properties → convert_properties_to_json.py → JSON translation.json
```

### 1.4 Translation Process

**FACT:** The translation rule is: **update `en-US` only**. Other languages are handled separately (likely by translation service or contributors), not by developers.

---

## 2. Proposed C++ Localization Strategy

### 2.1 Approach Comparison

| Approach | Pros | Cons | Verdict |
|----------|------|------|---------|
| **Windows .rc string tables** | Native, tools support (Visual Studio), standard | Requires recompile for new translations, poor workflow for translators | ❌ Not recommended |
| **Runtime-loadable JSON** | No recompile, easy translator workflow, familiar from current app | Slightly more code to load/parse | ✅ **Recommended** |
| **GNU gettext (.po/.mo)** | Industry standard, tools (poedit, transifex) | Extra dependency (libintl), less familiar on Windows | ⚠️ Alternative |
| **Embedded string table (.txt→.h)** | Simple, no parsing at runtime | Requires recompile, manual string IDs | ❌ Not recommended |

**RECOMMENDATION:** Use **runtime-loadable JSON files**, mirroring the current `translation.json` approach. This preserves the existing translation workflow and allows updates without recompilation.

---

## 3. JSON Localization Design

### 3.1 File Structure

```
resources/
└── locales/
    ├── en-US.json          (primary — maintained by developers)
    ├── de.json
    ├── fr.json
    ├── es.json
    ├── it.json
    ├── pt-BR.json
    ├── ja.json
    ├── ko.json
    ├── zh-CN.json
    ├── zh-TW.json
    ├── ar.json             (RTL)
    └── he.json             (RTL)
```

### 3.2 JSON Format

```json
{
  "app": {
    "name": "PDF Elite",
    "version": "1.0.0"
  },
  "menu": {
    "file": "File",
    "edit": "Edit",
    "view": "View",
    "tools": "Tools",
    "help": "Help"
  },
  "menu.file": {
    "open": "Open...",
    "save": "Save",
    "saveAs": "Save As...",
    "print": "Print...",
    "exit": "Exit"
  },
  "toolbar": {
    "undo": "Undo",
    "redo": "Redo",
    "zoomIn": "Zoom In",
    "zoomOut": "Zoom Out",
    "fitPage": "Fit to Page",
    "fitWidth": "Fit to Width"
  },
  "search": {
    "title": "Find in Document",
    "placeholder": "Search...",
    "next": "Next",
    "previous": "Previous",
    "matchCount": "{current} of {total} matches",
    "noResults": "No results found"
  },
  "annotations": {
    "highlight": "Highlight",
    "underline": "Underline",
    "strikeout": "Strikethrough",
    "freeText": "Text",
    "ink": "Freehand Draw",
    "stamp": "Stamp",
    "note": "Note"
  },
  "errors": {
    "fileNotFound": "File not found: {path}",
    "corruptPdf": "The PDF file is corrupted and cannot be opened.",
    "wrongPassword": "The password is incorrect.",
    "saveFailed": "Failed to save the file. {error}",
    "outOfMemory": "Not enough memory to complete the operation."
  },
  "plurals": {
    "pages": {
      "one": "{count} page",
      "other": "{count} pages"
    },
    "annotations": {
      "one": "{count} annotation",
      "other": "{count} annotations"
    }
  }
}
```

### 3.3 String ID Convention

Dot-separated hierarchical keys matching the UI structure:

```
{section}.{subsection}.{element}
```

Examples: `menu.file.open`, `toolbar.undo`, `search.matchCount`, `errors.fileNotFound`

---

## 4. C++ Localization API

### 4.1 LocalizationManager

```cpp
// common/public/localization.h
class LocalizationManager {
public:
    static LocalizationManager& Instance();

    // Initialization
    Result<void> LoadLanguage(std::string_view locale_code);
    Result<void> SetFallbackLanguage(std::string_view locale_code);

    // String access
    std::string Tr(std::string_view key) const;
    std::string Tr(std::string_view key,
                   const std::unordered_map<std::string, std::string>& args) const;

    // Pluralization
    std::string TrPlural(std::string_view key, int count) const;

    // Queries
    std::string GetCurrentLanguage() const;
    std::vector<std::string> GetAvailableLanguages() const;
    bool IsRTL() const;

    // Change notification
    using LanguageChangedCallback = std::function<void(std::string_view new_lang)>;
    void OnLanguageChanged(LanguageChangedCallback callback);

private:
    LocalizationManager() = default;
    nlohmann::json strings_;
    nlohmann::json fallback_strings_;
    std::string current_language_;
    std::string fallback_language_ = "en-US";
    std::vector<LanguageChangedCallback> callbacks_;
};

// Convenience macro
#define TR(key) LocalizationManager::Instance().Tr(key)
#define TR_FMT(key, ...) LocalizationManager::Instance().Tr(key, __VA_ARGS__)
```

---

## 5. Language Detection

### 5.1 Priority Order

| Priority | Source | Method |
|----------|--------|--------|
| 1 | User preference (saved in settings) | `GetPrivateProfileString` or JSON settings |
| 2 | Windows user locale | `GetUserDefaultUILanguage()` |
| 3 | Fallback | `en-US` |

### 5.2 Implementation

```cpp
std::string DetectSystemLanguage() {
    LANGID langid = GetUserDefaultUILanguage();
    WCHAR locale_name[LOCALE_NAME_MAX_LENGTH];
    if (GetLocaleInfoW(langid, LOCALE_SNAME, locale_name, LOCALE_NAME_MAX_LENGTH)) {
        // Convert "de-DE" to "de", "zh-Hans" to "zh-CN", etc.
        return NormalizeLocaleCode(WideToUtf8(locale_name));
    }
    return "en-US";
}
```

---

## 6. RTL Support

**FACT:** The current web app has no RTL support. The native app should plan for it.

### 6.1 RTL Languages
| Language | Code | Direction |
|----------|------|-----------|
| Arabic | `ar` | RTL |
| Hebrew | `he` | RTL |
| Persian | `fa` | RTL |
| Urdu | `ur` | RTL |

### 6.2 RTL Implementation

| Component | RTL Handling |
|-----------|-------------|
| Win32 dialogs | `SetProcessDefaultLayout(LAYOUT_RTL)` or per-window `SetWindowLongPtr(GWL_EXSTYLE, WS_EX_LAYOUTRTL)` |
| Menus | Automatic with `WS_EX_LAYOUTRTL` |
| Toolbar | Automatic with `WS_EX_LAYOUTRTL` |
| PDF canvas | No change — PDF content direction is per-document |
| Custom drawn text | Use `DrawText` with `DT_RTLREADING` flag |

### 6.3 RTL Layout Rules

```
LTR:  [Icon] [Text] →→→ [Control]
RTL:  [Control] ←←← [Text] [Icon]
```

---

## 7. Pluralization

**RECOMMENDATION:** Use CLDR plural rules for the `en-US` locale initially, with a simple `one`/`other` distinction. Extend to full CLDR rules when additional languages require it.

```cpp
std::string TrPlural(std::string_view key, int count) const {
    // English rules (covers most cases for v1)
    std::string form_key;
    if (count == 1) {
        form_key = fmt::format("{}.one", key);
    } else {
        form_key = fmt::format("{}.other", key);
    }
    return Tr(form_key, {{"count", std::to_string(count)}});
}
```

---

## 8. Translation Workflow

### 8.1 Developer Workflow

```
1. Add/modify strings in en-US.json (only)
2. Run build — app works with English strings
3. Submit PR with en-US.json changes
4. Translation service picks up changes
5. Translated JSON files are committed separately
```

### 8.2 String Validation

Add a build-time check that all language files contain the same keys as `en-US.json`:

```python
# scripts/validate_translations.py
# Loads en-US.json as reference
# Checks all other .json files for missing or extra keys
# Reports mismatches as warnings (not errors — missing keys fall back to en-US)
```

### 8.3 Hot Reload (Debug Builds)

In debug builds, watch the locale file for changes and reload automatically:

```cpp
#ifdef _DEBUG
void LocalizationManager::StartFileWatcher() {
    // Watch resources/locales/ directory
    // On change, reload current language file
    // Notify UI via OnLanguageChanged callbacks
}
#endif
```

---

## 9. Migration from Current System

| Step | Action |
|------|--------|
| 1 | Extract all string keys from `frontend/editor/public/locales/en-US/translation.json` |
| 2 | Map to native string IDs (flatten dot-notation) |
| 3 | Create `en-US.json` in new format |
| 4 | Replace hardcoded strings in C++ code with `TR()` calls |
| 5 | Port other language JSONs with same key structure |
| 6 | Remove dependency on web translation infrastructure |

---

*Document 29 of 31 — Localization*