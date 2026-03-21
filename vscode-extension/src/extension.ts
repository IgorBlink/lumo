import * as vscode from 'vscode';
import * as cp from 'child_process';
import * as path from 'path';
import * as fs from 'fs';

// Parse error output from the lumo --validate command.
// Example line: "Parse error at line 2, col 16 [NEWLINE]: Expected expression"
const ERROR_PATTERN = /Parse error at line (\d+), col (\d+) \[([^\]]*)\]: (.*)/;

let diagnosticCollection: vscode.DiagnosticCollection;
let debounceTimers = new Map<string, ReturnType<typeof setTimeout>>();

export function activate(context: vscode.ExtensionContext) {
    diagnosticCollection = vscode.languages.createDiagnosticCollection('lumo');
    context.subscriptions.push(diagnosticCollection);

    // Validate any already-open Lumo documents.
    vscode.workspace.textDocuments.forEach((doc) => {
        if (doc.languageId === 'lumo') {
            scheduleValidation(doc);
        }
    });

    context.subscriptions.push(
        vscode.workspace.onDidOpenTextDocument((doc) => {
            if (doc.languageId === 'lumo') {
                scheduleValidation(doc);
            }
        }),

        vscode.workspace.onDidChangeTextDocument((event) => {
            if (event.document.languageId !== 'lumo') { return; }
            const cfg = vscode.workspace.getConfiguration('lumo');
            if (cfg.get<boolean>('validateOnType', true)) {
                scheduleValidation(event.document);
            }
        }),

        vscode.workspace.onDidSaveTextDocument((doc) => {
            if (doc.languageId === 'lumo') {
                scheduleValidation(doc);
            }
        }),

        vscode.workspace.onDidCloseTextDocument((doc) => {
            diagnosticCollection.delete(doc.uri);
            const key = doc.uri.toString();
            const t = debounceTimers.get(key);
            if (t !== undefined) {
                clearTimeout(t);
                debounceTimers.delete(key);
            }
        })
    );
}

function scheduleValidation(document: vscode.TextDocument) {
    const key = document.uri.toString();
    const existing = debounceTimers.get(key);
    if (existing !== undefined) {
        clearTimeout(existing);
    }
    const timer = setTimeout(() => {
        debounceTimers.delete(key);
        validateDocument(document);
    }, 500);
    debounceTimers.set(key, timer);
}

function resolveExecutable(): string {
    const cfg = vscode.workspace.getConfiguration('lumo');
    const configured = cfg.get<string>('executablePath', '').trim();
    if (configured !== '') {
        return configured;
    }

    // Auto-detect: try a `lumo` binary at the workspace root first.
    const folders = vscode.workspace.workspaceFolders;
    if (folders && folders.length > 0) {
        const candidate = path.join(folders[0].uri.fsPath, 'lumo');
        if (fs.existsSync(candidate)) {
            return candidate;
        }
    }

    // Fall back to whatever is on PATH.
    return 'lumo';
}

function validateDocument(document: vscode.TextDocument) {
    const executable = resolveExecutable();
    const source = document.getText();

    let stderr = '';
    let stdout = '';

    let proc: cp.ChildProcess;
    try {
        proc = cp.spawn(executable, ['--validate'], { stdio: ['pipe', 'pipe', 'pipe'] });
    } catch (err) {
        // Binary not found – clear stale diagnostics and warn once.
        diagnosticCollection.set(document.uri, []);
        vscode.window.showWarningMessage(
            `Lumo: Could not launch validator. Set "lumo.executablePath" in your settings. (${err})`
        );
        return;
    }

    proc.stdout?.on('data', (chunk: Buffer) => { stdout += chunk.toString(); });
    proc.stderr?.on('data', (chunk: Buffer) => { stderr += chunk.toString(); });

    proc.stdin?.write(source);
    proc.stdin?.end();

    proc.on('error', (err) => {
        diagnosticCollection.set(document.uri, []);
        vscode.window.showWarningMessage(
            `Lumo: Validator process error – ${err.message}. Check "lumo.executablePath".`
        );
    });

    proc.on('close', (code) => {
        if (code === 0) {
            // Clean parse.
            diagnosticCollection.set(document.uri, []);
            return;
        }

        const diagnostics: vscode.Diagnostic[] = [];
        const combined = stdout + '\n' + stderr;

        for (const line of combined.split('\n')) {
            const match = ERROR_PATTERN.exec(line);
            if (!match) { continue; }

            const lineNo = Math.max(0, parseInt(match[1], 10) - 1);
            const colNo  = Math.max(0, parseInt(match[2], 10) - 1);
            const message = `[${match[3]}] ${match[4]}`;

            const docLine = document.lineAt(Math.min(lineNo, document.lineCount - 1));
            const range = new vscode.Range(
                lineNo,
                colNo,
                lineNo,
                Math.max(colNo + 1, docLine.text.length)
            );

            const diag = new vscode.Diagnostic(range, message, vscode.DiagnosticSeverity.Error);
            diag.source = 'lumo';
            diagnostics.push(diag);

            // Only report the first fatal error; subsequent lines are noise.
            break;
        }

        diagnosticCollection.set(document.uri, diagnostics);
    });
}

export function deactivate() {
    diagnosticCollection.dispose();
    for (const t of debounceTimers.values()) {
        clearTimeout(t);
    }
    debounceTimers.clear();
}
