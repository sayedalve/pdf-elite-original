\# AGENTS.md

\# PDF Elite Native Engineering Rules



Version: 2.0

Status: Binding

Target: Windows 10/11 x64

Language: C++20

Compiler: MSVC

Build: CMake + Ninja



This document defines the binding engineering rules for all AI coding agents working on the native PDF Elite rewrite.



============================================================

1\. PROJECT MISSION

============================================================



PDF Elite is being completely rebuilt as a professional native Windows PDF application.



The existing PDF Elite application is the functional reference.



The existing implementation is NOT the architecture to preserve.



The new application must be:



• Native Windows

• C++20

• Win32

• Direct2D

• DirectWrite

• WIC

• PDFium

• Fast

• Stable

• Secure

• Maintainable

• Responsive

• Lightweight



The following technologies must NOT be part of the production architecture:



• Tauri

• Electron

• WebView

• Java

• JRE

• Stirling PDF

• Spring Boot

• PDFBox

• Node.js runtime

• React runtime



The goal is to preserve product functionality while replacing the implementation architecture.



============================================================

2\. AUTHORITATIVE DOCUMENTATION

============================================================



The repository contains a directory named:



directions/



This directory contains the engineering documentation generated from a complete audit of the existing PDF Elite repository.



The directions/ directory is the authoritative architectural specification unless a document is explicitly marked as outdated or superseded.



Before making architectural changes:



1\. Read the relevant files in directions/.

2\. Read the actual source code involved.

3\. Compare the documentation with the source.

4\. Do not blindly trust assumptions.

5\. Record contradictions instead of silently choosing one.



Important documents include:



• directions/ARCHITECTURE.md

• directions/CURRENT\_ARCHITECTURE.md

• directions/FEATURE\_SPEC.md

• directions/FEATURE\_INVENTORY.md

• directions/PDF\_ENGINE.md

• directions/PDFIUM\_ANALYSIS.md

• directions/RENDERING\_ARCHITECTURE.md

• directions/UI\_DESIGN.md

• directions/BUILD\_SYSTEM.md

• directions/DEVELOPMENT\_WORKFLOW.md

• directions/HOT\_RELOAD.md

• directions/MIGRATION\_PLAN.md

• directions/TESTING.md

• directions/PERFORMANCE.md

• directions/SECURITY.md

• directions/DEFINITION\_OF\_DONE.md



If another document conflicts with this AGENTS.md, stop and identify the conflict before implementation.



============================================================

3\. COMPLETE ARCHITECTURE REPLACEMENT

============================================================



This is NOT an incremental modification of the current Tauri application.



The native application is a new implementation.



Do not:



• Convert React components into C++ one by one.

• Port Rust files mechanically.

• Recreate the Java backend in C++ line by line.

• Preserve Tauri concepts unnecessarily.

• Preserve Stirling PDF architecture unnecessarily.



Instead:



Analyze existing behavior.



Then implement the behavior using the new native architecture.



The current application remains the behavioral reference until feature parity is proven.



============================================================

4\. TARGET TECHNOLOGY

============================================================



Required:



C++20

Win32

Direct2D

DirectWrite

WIC

PDFium

CMake

Ninja

MSVC

x64



Direct2D is the primary 2D rendering API.



DirectWrite is the primary text rendering API.



WIC is the preferred Windows image decoding and conversion API where appropriate.



Win32 provides application windows, input, window management, menus, dialogs, clipboard integration, file dialogs, printing integration and other Windows platform functionality.



PDFium is the PDF engine.



Do NOT use GDI or GDI+ as the primary rendering architecture.



GDI/GDI+ may only be used where a specific Windows compatibility requirement makes them genuinely necessary, and such use must be documented.



Do not introduce another UI framework merely for convenience.



============================================================

5\. HARDWARE CONSTRAINT

============================================================



The developer machine has:



• Intel Core i7 8th generation

• 16 GB RAM

• Windows 11

• No planned hardware upgrade



The project must therefore prioritize fast incremental development.



PDFium must NOT be rebuilt during normal application development.



Large third party dependencies must be prebuilt whenever practical.



Release builds and expensive PDFium builds should be performed through CI or another capable build environment when practical.



============================================================

6\. BUILD PERFORMANCE

============================================================



Use:



• CMake

• Ninja

• MSVC

• Precompiled headers where beneficial

• Incremental compilation

• Parallel compilation

• CMake presets

• Separate targets

• Prebuilt PDFium during normal development



Provide:



Debug

Release

RelWithDebInfo



The normal developer workflow must NOT require:



• Rebuilding PDFium

• Building the installer

• Rebuilding every project component

• Performing a full clean build



unless the developer explicitly requests it.



Do not unnecessarily create header dependencies that cause large portions of the project to rebuild.



============================================================

7\. DEVELOPMENT HOT RELOAD

============================================================



PDF Elite must provide a native development mode.



Recommended invocation:



PDFElite.exe --dev



The development mode should support hot reload of safe external resources such as:



• Themes

• Colors

• Layout definitions

• Toolbar configuration

• Sidebar configuration

• Icons

• Images

• UI text

• Other approved visual resources



Recommended resource structure:



ui/

themes/

layouts/

icons/

assets/



When an approved resource changes:



File changes

→ watcher detects change

→ resource reloads

→ native UI updates

→ Direct2D redraws



The developer should not need to rebuild the EXE for purely visual resource changes.



Do not create a browser based production UI merely to obtain hot reload.



Native development experience is required.



============================================================

8\. C++ HOT RELOAD

============================================================



Native C++ hot reload is optional and must not compromise architecture.



A reloadable UI DLL may be considered if it provides meaningful development benefits.



Possible structure:



PDFElite.exe

PDFEliteCore

PDFEliteUI

PDFium



However:



Do not create DLL boundaries solely for theoretical hot reload.



Evaluate:



• Build speed

• ABI stability

• Ownership

• Debugging

• Reliability

• Complexity



Prefer a simple and stable architecture over an elaborate hot reload system.



============================================================

9\. APPLICATION SIZE

============================================================



Release installer requirements:



Preferred:



80 to 100 MB



Absolute maximum:



120 MB



If a release installer exceeds 120 MB:



THE RELEASE BUILD MUST FAIL.



The exact static versus dynamic linking strategy for PDFium must be chosen based on:



• Size

• Performance

• Reliability

• Updateability

• Development speed

• Security



Do not choose static or DLL linking merely because it sounds architecturally cleaner.



Measure the final result.



Do not sacrifice correctness or security merely to save a few megabytes.



============================================================

10\. DEPENDENCY POLICY

============================================================



Minimize third party dependencies.



Every dependency must have a documented reason.



Before adding a dependency, determine:



• What problem does it solve?

• Can Windows API solve it?

• Can the standard library solve it?

• Can PDFium solve it?

• What is its binary size?

• What is its maintenance burden?

• What is its license?

• What is its security impact?



Do not add libraries for trivial functionality.



Do not introduce large frameworks that duplicate Windows capabilities.



============================================================

11\. PDFIUM ISOLATION

============================================================



This is a HARD architectural rule.



PDFium types MUST NOT leak outside the PDF engine implementation.



These types must never appear in UI or application level APIs:



• FPDF\_DOCUMENT

• FPDF\_PAGE

• FPDF\_PAGEOBJECT

• FPDF\_ANNOTATION

• FPDF\_TEXTPAGE

• Other FPDF\_\* handles



The architecture must look like:



UI

↓

Application

↓

Document Model

↓

PDF Engine Interface

↓

PDFium Adapter

↓

PDFium



The UI must not include PDFium implementation headers.



Only the PDF engine implementation may directly call PDFium APIs.



Violations are blocking architectural errors.



============================================================

12\. OWNERSHIP

============================================================



Use RAII throughout the application.



Preferred:



• std::unique\_ptr

• std::shared\_ptr only when genuinely required

• RAII wrappers for Windows handles

• RAII wrappers for PDFium resources

• Explicit ownership



Never:



• Raw owning pointers

• Manual new/delete

• Global ownership

• Hidden ownership

• Unsafe lifetime assumptions



Every resource must have a clear owner.



============================================================

13\. ERROR HANDLING

============================================================



Use a project-wide Result or equivalent expected style.



Prefer:



Result<T>



over exceptions for normal application errors.



Errors must be:



• Explicit

• Propagated

• Logged where useful

• Converted into user friendly UI messages where appropriate



Never silently ignore a failure that could affect PDF integrity.



Never crash because a user opened a malformed PDF.



Never overwrite an original document after an unsuccessful save.



============================================================

14\. THREADING

============================================================



The UI thread must remain responsive.



Never perform expensive operations directly in the UI thread.



Background work includes:



• PDF rendering

• Thumbnail generation

• Text extraction

• Search

• Large document loading

• Image processing

• Saving

• Export



Every asynchronous operation must have:



• Clear ownership

• Cancellation behavior where appropriate

• Safe shutdown

• Defined lifetime



Do not assume PDFium is globally thread safe.



Follow the verified PDFium thread safety rules documented in directions/PDF\_ENGINE.md and directions/PDFIUM\_ANALYSIS.md.



============================================================

15\. DOCUMENT MODEL

============================================================



The document model must be separate from the UI.



UI state is not the document.



PDFium state is not the UI.



The architecture should conceptually separate:



Document

Page

Object

Selection

Annotation

History

View State

UI State



Do not store application state in global variables.



============================================================

16\. UNDO / REDO

============================================================



Document mutations must use a command based system or another documented transactional history mechanism.



Undo/redo must be designed early.



Do not bolt undo onto the project after implementing all editing functionality.



Commands should have:



• Execute

• Undo

• Redo where necessary

• Clear ownership

• Safe failure behavior



============================================================

17\. RENDERING

============================================================



Primary rendering stack:



PDFium

\+

Direct2D

\+

DirectWrite

\+

WIC



The rendering system should support:



• Zoom

• Pan

• Smooth scrolling

• High DPI

• Tile rendering

• Render caching

• Background rendering

• Cancellation

• Large PDFs



Do not use GDI+ as the primary PDF rendering surface.



Do not block the UI thread.



============================================================

18\. UI ARCHITECTURE

============================================================



The UI must be a professional modern Windows application.



It must NOT resemble a basic legacy Win32 utility.



Use Win32 for:



• Windows

• Input

• Menus

• Dialogs

• Window management

• OS integration



Use Direct2D and DirectWrite for custom high quality surfaces.



Maintain clear separation between:



• UI

• Document state

• PDF engine

• Application state



============================================================

19\. UI RESOURCE HOT RELOAD

============================================================



Externalize visual information where it improves development speed.



Suitable candidates:



• Theme

• Colors

• Typography

• Layout

• Toolbar configuration

• Icons

• Visual states

• UI text



Do not externalize complex behavior merely to avoid compilation.



The resource format must be:



• Lightweight

• Validatable

• Versionable

• Fast to load

• Easy to debug



============================================================

20\. FILE SAFETY

============================================================



Saving a PDF is a critical operation.



Do not directly overwrite the original file during an unsafe save process.



Prefer:



Temporary file

→ flush

→ validate

→ atomic replacement



Preserve the original file if saving fails.



============================================================

21\. SECURITY

============================================================



Treat PDFs as untrusted input.



Consider:



• Malformed PDFs

• Malicious PDFs

• Embedded files

• JavaScript

• External URLs

• File path attacks

• Temporary file attacks

• Resource exhaustion

• PDFium vulnerabilities



Do not execute arbitrary external content.



Do not invoke shell commands with unsanitized user controlled input.



============================================================

22\. NO PREMATURE PLUGIN SYSTEM

============================================================



Do not introduce a plugin system unless a later architecture review explicitly approves it.



PDF Elite should remain a cohesive desktop application.



Do not build an enterprise plugin framework for simple internal features.



============================================================

23\. NO PREMATURE ABSTRACTION

============================================================



Avoid:



• Excessive interfaces

• Deep factory hierarchies

• Unnecessary dependency injection

• Generic frameworks

• Abstract abstractions with no concrete purpose



Use interfaces when they enforce important boundaries.



In particular:



PDF engine abstraction is required.



Everything else should justify its abstraction.



============================================================

24\. TESTING

============================================================



Important functionality must have tests.



At minimum test:



• PDF opening

• Rendering

• Saving

• Reopening

• Text extraction

• Search

• Selection

• Annotations

• Page operations

• Clipboard

• Undo/redo

• Large PDFs

• Malformed PDFs



Create a regression corpus.



Never assume that a feature is correct because the UI appears to work.



Validate the resulting PDF.



============================================================

25\. DOCUMENTATION

============================================================



The directions/ directory is the architecture documentation source of truth.



When architecture changes:



Update the appropriate document.



When a new major subsystem is added:



Update:



directions/ARCHITECTURE.md



When feature behavior changes:



Update:



directions/FEATURE\_SPEC.md



When a technical decision changes:



Update:



directions/ARCHITECTURE\_DECISIONS.md



When the development workflow changes:



Update:



directions/DEVELOPMENT\_WORKFLOW.md



============================================================

26\. CODE STYLE

============================================================



Use modern C++20.



Prefer:



• Strong types

• RAII

• const correctness

• constexpr

• std::span

• std::filesystem

• std::string/std::wstring appropriately

• Standard library containers

• Clear namespaces



Use Unicode Windows APIs.



Prefer:



CreateWindowExW

SendMessageW

SetWindowTextW



Do not use ANSI APIs.



Use consistent naming and formatting.



The project must contain a `.clang-format`.



============================================================

27\. CODE REVIEW RULES

============================================================



Before accepting a change, verify:



• Does it respect architecture boundaries?

• Does it unnecessarily introduce dependencies?

• Does it block the UI thread?

• Does it expose PDFium types?

• Does it create unclear ownership?

• Does it increase build time unnecessarily?

• Does it affect PDF integrity?

• Does it increase release size?

• Does it require documentation updates?

• Does it require regression tests?



============================================================

28\. AI WORKFLOW

============================================================



Every AI coding agent must follow this sequence:



1\. Read directions/ documentation relevant to the task.

2\. Inspect the actual current source code.

3\. Identify dependencies and call flows.

4\. State important assumptions.

5\. Make the smallest correct architectural change.

6\. Build the relevant targets.

7\. Run relevant tests.

8\. Inspect the resulting behavior.

9\. Update documentation if architecture changed.



Do NOT modify large amounts of code without first understanding the surrounding architecture.



Do NOT rewrite files merely to make them stylistically cleaner.



Do NOT silently delete old functionality.



============================================================

29\. CURRENT APPLICATION AS REFERENCE

============================================================



The existing application is the functional reference until the native version reaches parity.



Use it to determine:



• Existing behavior

• Existing UI workflows

• Existing feature behavior

• Edge cases

• Expected outputs



Do not copy its architecture.



============================================================

30\. FINAL PRINCIPLE

============================================================



The goal is not:



"Convert the old application to C++."



The goal is:



"Build a world class native Windows PDF editor using C++20, Win32, Direct2D, DirectWrite, WIC and PDFium while preserving the useful functionality of the existing PDF Elite application."



Correctness comes before size.



Stability comes before speed.



Security comes before convenience.



Maintainability comes before cleverness.



Development speed matters.



The developer's limited hardware must be respected.



The final installer must never exceed 120 MB.



