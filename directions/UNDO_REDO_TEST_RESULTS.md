# Undo/Redo Test Results

This document tracks the results of the Undo/Redo Command operations regression tests run inside `RegressionSuite.exe`.

## Automated Test Coverage

| Test Area | C++ Test Name | Status |
| --- | --- | --- |
| **Basic Undo/Redo Execution** | `Command_BasicUndoRedo` | PASS |
| **Redo Invalidation** | `Command_RedoInvalidation` | PASS |

## Validation Notes
- **Basic Undo/Redo Execution**: Instantiated a `CommandStack` coupled with a loaded native `PdfDocument`. Verified state transitions using `InsertBlankPageCommand`. Assertions successfully tracked that `IsDirty` flags matched `m_savePosition`.
- **Redo Invalidation**: Proved that executing a new command after issuing several undos purges the entire dormant redo stack (`m_redoStack.clear()`), leaving `CanRedo()` returning false and correctly capping document structural mutations.

## Memory Strategy Validations
- Operations like moving, adding, or rotating avoid duplicating bulk PDF assets. Total memory growth remains O(1) in relation to standard operation parameters. 
- Only heavily destructive operations such as `DeletePageCommand` allocate discrete temporary `FPDF_DOCUMENT` blocks. These clean up correctly upon destruction within the command stack depth cap purging.

## Limitations
- UI menu synchronization relies on subsequent manual querying. Observers have been integrated, but `PdfViewer` does not implement dynamic dirty state tracking over the taskbar.
- Undo tracking limits cap at `maxDepth` configuration (defaulting to 100), shedding oldest interactions successfully but irreversibly.
