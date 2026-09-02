# Undo/Redo System

> Engineering doc for rebuilding the PDF Elite undo/redo subsystem as a native C++/Win32 application.

## Current Architecture (Tauri/Web)

### Undo/Redo Scope

| Layer | Implementation | Scope | Limitation |
|-------|---------------|-------|------------|
| HistoryPlugin | EmbedPDF (JavaScript) | Annotation operations only | Not for text edits or page ops |
| Tool operations | `consumeFiles` / `undoConsumeFiles` | File-level operations | Restores original from IndexedDB |
| Document edits | — | — | **NOT IMPLEMENTED** |

> **FACT**: HistoryPlugin in EmbedPDF provides annotation-level undo/redo only.

> **FACT**: Tool operations use `consumeFiles` / `undoConsumeFiles` for file-level undo that restores the original document from IndexedDB.

> **FACT**: There is NO document-level undo — editing operations like text changes, page operations, and image manipulations are not undoable.

### Current Limitations

```
UNDOABLE (current):
  ├── Annotation: add, move, resize, delete annotations
  └── Tool ops: file-level restore from IndexedDB

NOT UNDOABLE (current):
  ├── Text edits (content stream modifications)
  ├── Page insertions, deletions, rotations
  ├── Image replacements
  ├── Form field modifications
  ├── Page reordering
  └── Any content stream changes
```

---

## Proposed Native Architecture (C++/Win32)

### Design Goals

> **RECOMMENDATION**: Implement a full Command pattern undo/redo system that covers ALL operations: text edits, annotations, page operations, image operations, and form modifications.

### Command Pattern Architecture

```
ICommand (abstract interface)
├── TextEditCommand         (text content modifications)
├── AnnotationAddCommand    (add annotation)
├── AnnotationModifyCommand (move/resize/change annotation)
├── AnnotationDeleteCommand (delete annotation)
├── PageInsertCommand       (insert new page)
├── PageDeleteCommand       (remove page)
├── PageRotateCommand       (rotate page)
├── PageReorderCommand      (move page to new position)
├── ImageInsertCommand      (add/replace image)
├── ImageDeleteCommand      (remove image)
├── ImageTransformCommand   (resize/rotate/move image)
├── FormFieldChangeCommand  (modify form field value)
└── CompoundCommand         (group of commands executed atomically)
```

### Core Interface

```cpp
// command.h
class ICommand {
public:
    virtual ~ICommand() = default;

    virtual std::wstring GetName() const = 0;
    virtual HRESULT Execute() = 0;
    virtual HRESULT Undo() = 0;

    virtual bool CanMergeWith(const ICommand* other) const { return false; }
    virtual void MergeWith(std::unique_ptr<ICommand> other) {}

    virtual bool IsSignificant() const { return true; }
};

class CCommandStack {
public:
    void Clear();
    bool CanUndo() const;
    bool CanRedo() const;

    void Push(std::unique_ptr<ICommand> cmd);
    HRESULT Undo();
    HRESULT Redo();

    std::wstring GetUndoDescription() const;
    std::wstring GetRedoDescription() const;

    void SetMaxSize(size_t maxSize);
    size_t GetUndoCount() const { return m_undoStack.size(); }
    size_t GetRedoCount() const { return m_redoStack.size(); }

    // Coalescing
    void EnableCoalescing(bool enable);
    void FlushCoalesce();  // Force-push current coalesced command

private:
    std::vector<std::unique_ptr<ICommand>> m_undoStack;
    std::vector<std::unique_ptr<ICommand>> m_redoStack;
    size_t m_maxSize = 100;

    // Coalescing support
    bool m_coalescing = false;
    std::unique_ptr<ICommand> m_coalesceTarget;
};
```

### Command Examples

#### Text Edit Command

```cpp
class TextEditCommand : public ICommand {
public:
    TextEditCommand(CDocument* doc, int pageIndex,
                    const std::wstring& objectId,
                    const std::wstring& oldText,
                    const std::wstring& newText);

    std::wstring GetName() const override { return L"Text Edit"; }

    HRESULT Execute() override {
        // Apply newText to the content stream object
        return m_doc->SetTextContent(m_pageIndex, m_objectId, m_newText);
    }

    HRESULT Undo() override {
        // Restore oldText
        return m_doc->SetTextContent(m_pageIndex, m_objectId, m_oldText);
    }

    bool CanMergeWith(const ICommand* other) const override {
        if (other->GetName() != L"Text Edit") return false;
        auto* otherEdit = static_cast<const TextEditCommand*>(other);
        return otherEdit->m_pageIndex == m_pageIndex &&
               otherEdit->m_objectId == m_objectId &&
               otherEdit->m_newText == m_oldText;  // Continuous typing
    }

    void MergeWith(std::unique_ptr<ICommand> other) override {
        auto* otherEdit = static_cast<TextEditCommand*>(other.get());
        m_newText = otherEdit->m_newText;
    }

private:
    CDocument* m_doc;
    int m_pageIndex;
    std::wstring m_objectId;
    std::wstring m_oldText;
    std::wstring m_newText;
};
```

#### Annotation Command

```cpp
class AnnotationModifyCommand : public ICommand {
public:
    AnnotationModifyCommand(CDocument* doc, int pageIndex,
                           const std::string& annotId,
                           const AnnotationState& oldState,
                           const AnnotationState& newState)
        : m_doc(doc), m_pageIndex(pageIndex), m_annotId(annotId),
          m_oldState(oldState), m_newState(newState) {}

    std::wstring GetName() const override { return L"Modify Annotation"; }

    HRESULT Execute() override {
        return m_doc->SetAnnotationState(m_pageIndex, m_annotId, m_newState);
    }

    HRESULT Undo() override {
        return m_doc->SetAnnotationState(m_pageIndex, m_annotId, m_oldState);
    }

private:
    CDocument* m_doc;
    int m_pageIndex;
    std::string m_annotId;
    AnnotationState m_oldState;
    AnnotationState m_newState;
};
```

#### Compound Command (Atomic Group)

```cpp
class CompoundCommand : public ICommand {
public:
    CompoundCommand(const std::wstring& name) : m_name(name) {}

    void AddCommand(std::unique_ptr<ICommand> cmd) {
        m_commands.push_back(std::move(cmd));
    }

    std::wstring GetName() const override { return m_name; }

    HRESULT Execute() override {
        for (auto& cmd : m_commands) {
            HRESULT hr = cmd->Execute();
            if (FAILED(hr)) {
                // Rollback already-executed commands
                for (auto it = m_commands.rbegin(); it != m_commands.rend(); ++it) {
                    if (it->get() != cmd.get()) {
                        (*it)->Undo();
                    }
                }
                return hr;
            }
        }
        return S_OK;
    }

    HRESULT Undo() override {
        // Undo in reverse order
        for (auto it = m_commands.rbegin(); it != m_commands.rend(); ++it) {
            HRESULT hr = (*it)->Undo();
            if (FAILED(hr)) return hr;
        }
        return S_OK;
    }

private:
    std::wstring m_name;
    std::vector<std::unique_ptr<ICommand>> m_commands;
};
```

### Per-Document Command Stack

> **RECOMMENDATION**: Each open document maintains its own independent command stack. This is critical for multi-tab support.

```cpp
class CDocument {
    // ...
    CCommandStack m_undoStack;
};
```

```
Tab 1 (report.pdf)
  └── CCommandStack
        ├── Undo: [TextEdit, AnnotationMove, PageInsert]
        └── Redo: [] (empty after new operation)

Tab 2 (invoice.pdf)
  └── CCommandStack
        ├── Undo: [ImageInsert]
        └── Redo: [AnnotationDelete]
```

### Undo/Redo Integration

```cpp
class CUndoManager {
public:
    static CUndoManager& Instance();

    // Execute and track
    HRESULT ExecuteCommand(std::unique_ptr<ICommand> cmd, CDocument* doc);

    // Undo/Redo active document
    HRESULT Undo();
    HRESULT Redo();

    // State queries
    bool CanUndo() const;
    bool CanRedo() const;
    std::wstring GetUndoDescription() const;
    std::wstring GetRedoDescription() const;

    // UI update notification
    using StateCallback = std::function<void()>;
    void RegisterStateCallback(StateCallback cb);

private:
    CUndoManager() = default;
    CDocument* GetActiveDocument() const;
    std::vector<StateCallback> m_stateCallbacks;
};
```

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+Z` | Undo last operation |
| `Ctrl+Y` | Redo |
| `Ctrl+Shift+Z` | Redo (alternative) |
| `Ctrl+Alt+Z` | Redo (alternative) |

### Undo/Redo Menu Integration

```
Edit Menu:
  ├── Undo        Ctrl+Z      "Undo: Move Annotation"
  ├── Redo        Ctrl+Y      "Redo: Delete Page"
  ├── ─────────────────────
  ├── Cut         Ctrl+X
  ├── Copy        Ctrl+C
  └── Paste       Ctrl+V
```

> **RECOMMENDATION**: The Undo/Redo menu items should dynamically show the description of the next undoable/redoable operation (e.g., "Undo: Move Annotation").

### Command Coalescing

> **RECOMMENDATION**: Coalesce (merge) consecutive commands of the same type to avoid polluting the undo stack with trivial operations like individual character keystrokes.

```
Without coalescing (bad):
  Undo Stack: [Type 'H', Type 'e', Type 'l', Type 'l', Type 'o', ...]

With coalescing (good):
  Undo Stack: [Type "Hello"]
```

```cpp
void CCommandStack::Push(std::unique_ptr<ICommand> cmd) {
    if (m_coalescing && !m_undoStack.empty() && cmd->CanMergeWith(m_undoStack.back().get())) {
        // Merge into last command
        m_undoStack.back()->MergeWith(std::move(cmd));
    } else {
        // New entry
        m_undoStack.push_back(std::move(cmd));
        m_redoStack.clear();  // New operation clears redo stack
        TrimToMaxSize();
    }
    NotifyStateChanged();
}
```

### Coalescing Strategy Table

| Command Type | Coalesce? | Merge Rule |
|-------------|-----------|------------|
| TextEdit | Yes | Same page, same object, consecutive |
| AnnotationModify | Yes | Same annotation, same property, within 2 seconds |
| AnnotationAdd | No | Each add is separate |
| AnnotationDelete | No | Each delete is separate |
| PageRotate | Yes | Same page, consecutive rotations |
| PageInsert | No | Each insert is separate |
| PageDelete | No | Each delete is separate |
| ImageTransform | Yes | Same image, within 2 seconds |

### Memory Management

> **RECOMMENDATION**: Command stacks are memory-only. They are cleared when the document is closed. This avoids serialization complexity and limits memory usage.

| Setting | Value | Notes |
|---------|-------|-------|
| Max undo levels | 100 (configurable) | Trims oldest commands when exceeded |
| Stack memory limit | 256 MB (configurable) | Safety net for memory-heavy commands |
| Persistence | None | Cleared on document close |
| Auto-save interaction | Auto-save commits; undo stack preserved until close |

### Memory-Heavy Command Optimization

```cpp
class PageDeleteCommand : public ICommand {
    // Instead of storing full page data (potentially MB),
    // store only the page index and re-extract from PDFium on undo
public:
    PageDeleteCommand(CDocument* doc, int pageIndex);

    HRESULT Execute() override {
        m_deletedPageData = m_doc->ExtractPageData(m_pageIndex);
        return m_doc->DeletePage(m_pageIndex);
    }

    HRESULT Undo() override {
        return m_doc->InsertPage(m_pageIndex, m_deletedPageData);
    }

private:
    int m_pageIndex;
    std::vector<uint8_t> m_deletedPageData;  // Freed when command is trimmed
};
```

### Undo Stack State Diagram

```
                     Push(cmd)
          ┌─────────────────────────┐
          │                         ▼
    ┌──────────┐            ┌──────────┐
    │  Undo    │            │  Undo    │
    │  Stack   │            │  Stack   │
    │  [A,B,C] │            │  [A,B,C,D]│
    └──────────┘            └──────────┘
          ▲                         │
          │    Undo()               │
          │  ┌─────────────────┐    │
          │  │                 ▼    │
          │  │            ┌──────────┐
          │  │            │  Undo    │
          │  │            │  Stack   │
          │  │            │  [A,B,C] │
          │  │            └──────────┘
          │  │                     │
          │  │    Redo()           │
          │  │  ┌─────────────────┐│
          │  │  │                 ▼│
          │  │  │  ┌──────────┐   ││
          │  │  │  │ Redo     │   ││
          │  │  │  │ Stack    │   ││
          │  │  │  │ [D]      │   ││
          │  │  │  └──────────┘   ││
          │  │  │                  ││
          │  │  └──────────────────┘│
          │  │                       │
          └──┘◄──────────────────────┘
```

### Performance Considerations

| Scenario | Strategy |
|----------|----------|
| Typing command coalescing | Flush coalesce buffer after 1s of inactivity |
| Large page data in commands | Use deferred extraction — extract on Undo, not on Execute |
| Annotation state | Lightweight struct copy (~100 bytes per annotation) |
| Image commands | Store original image bytes only, reference PDFium handle |
| Memory pressure | Drop oldest commands when undo stack exceeds 256 MB |

---

## Implementation Checklist

- [ ] Define `ICommand` interface with `Execute()`, `Undo()`, `GetName()`
- [ ] Implement `CCommandStack` with push, undo, redo, trim
- [ ] Implement command coalescing with merge rules per command type
- [ ] Implement `TextEditCommand` with typing coalescing
- [ ] Implement annotation commands (add, modify, delete)
- [ ] Implement page operation commands (insert, delete, rotate, reorder)
- [ ] Implement image operation commands (insert, delete, transform)
- [ ] Implement `CompoundCommand` for atomic grouped operations
- [ ] Integrate per-document command stacks into `CDocument`
- [ ] Implement `CUndoManager` with keyboard shortcuts (Ctrl+Z/Y)
- [ ] Add undo/redo menu items with dynamic descriptions
- [ ] Implement undo stack memory limit (256 MB) with trimming
- [ ] Test with 100+ sequential operations for stack performance
