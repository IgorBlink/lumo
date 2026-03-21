# Prompt: Lumo Language Design Optimization for LLMs

## Role
You are a Senior Programming Language Engineer. 

## Task
Your task is to redesign and refine the Lumo programming language based on recent LLM benchmarking feedback. You must improve the language so it becomes maximally readable, consistent, and effective for LLMs to parse, reason about, and generate reliably.

## Objective
Identify weaknesses from the provided feedback, map them to exact issues in the current language design, and implement targeted syntax and semantic improvements. Your ultimate goal is to increase LLM performance (understanding, reasoning, exact-generation) by eliminating ambiguity and reinforcing parser-faithful structures.

---

## 1. Prioritized List of Changes to the Language

Based on the feedback analysis, implement the following prioritized language design changes:

1. **Make Block Structure Explicit (Replace Dash-Prefixes)**
2. **Convert Multi-Word Operators to Single Tokens**
3. **Remove Greedy Nested-Match Rule & Add Explicit If/Else**
4. **Reserve All Grammar Words Globally (Eliminate Context-Sensitivity)**
5. **Enforce Total Control Flow Forms (No Silent No-Ops)**
6. **Separate Statements from Expressions Completely**
7. **Add User-Defined Functions and Basic Data Structures**
8. **Publish a Normative Machine-Checkable Grammar & Style Spec**

---

## 2. Detailed Change Specifications

### Change 1: Make Block Structure Explicit
* **What to modify:** Replace the cosmetic indentation and dash-prefixed block nesting (`-`) with either strictly semantic indentation (like Python) or explicit block terminators (e.g., `end`).
* **Which feedback issue it solves:** "Indentation is cosmetic, so visual structure does not guarantee semantic structure." / "Dash-prefixed nesting in repeat, match, and pipe can be mis-scoped by models."
* **Why it improves LLM performance:** LLMs rely heavily on visual layout (whitespace/indentation) matching semantic scope. When visual structure diverges from parse structure, LLMs hallucinate scopes. Making structure explicit removes the largest source of parsing ambiguity.

### Change 2: Convert Multi-Word Operators to Single Tokens
* **What to modify:** Replace operators that span multiple words (`divided by`, `greater than`, `less than`) with single-word canonical alternatives (e.g., `divby`, `above`, `below`).
* **Which feedback issue it solves:** "Multiword operators increase boundary-resolution burden and fragmentation."
* **Why it improves LLM performance:** Multi-word operators break tokenization patterns and increase the fragility of partial completions. Single tokens provide stable semantic boundaries, greatly improving generation precision.

### Change 3: Remove Greedy Nested-Match Rule & Add Explicit If/Else
* **What to modify:** Remove the documented greedy nested-match behavior. Introduce a dedicated, plain-English `if`/`else` construct for standard boolean conditions to replace the reliance on `match` for simple branching. 
* **Which feedback issue it solves:** "Documented greedy nested-match behavior is a major reasoning and generation hazard."
* **Why it improves LLM performance:** LLMs carry a strong prior mental model of `if/else` from pretraining. Forcing them to use `match` with `catch error` for standard branching violates this intuition, leading to structural hallucinations and complex cascade workarounds.

### Change 4: Reserve All Grammar Words Globally
* **What to modify:** Forbid the use of special keywords like `value` and `result` as standard identifiers anywhere in the language. Make them globally reserved words.
* **Which feedback issue it solves:** "Special keywords like value and result have context-sensitive meaning." / "Potential for keyword collisions."
* **Why it improves LLM performance:** Context-sensitive keywords hurt exact decoding. Reserving all grammar tokens unconditionally removes token-role confusion and simplifies the inference load for the model.

### Change 5: Enforce Total Control Flow Forms
* **What to modify:** Require all branching constructs to be exhaustive. `match` without a `catch error` (which currently results in a silent no-op) must be disallowed.
* **Which feedback issue it solves:** "Match uses catch error as a default branch label... optional fallback branches in control flow."
* **Why it improves LLM performance:** Silent no-op branches increase reasoning uncertainty. Forcing explicit handling of all paths improves semantic predictability and strictness.

### Change 6: Separate Statements from Expressions Completely
* **What to modify:** Ensure zero overlap between statements and expressions. Expressions can no longer be evaluated as standalone statements without an explicit side-effect wrapper (e.g., assignment or print).
* **Which feedback issue it solves:** "Expression-as-statement support can blur intent and make program state transitions less explicit."
* **Why it improves LLM performance:** A strict statement/expression boundary forces the LLM to make state mutations obvious, reducing generation branching complexity and clarifying intent.

### Change 7: Add User-Defined Functions and Basic Data Structures
* **What to modify:** Introduce simple, English-syntax user-defined functions (e.g., `define task ...`) and working list/map data structures with plain-English accessors. Make `pipe` map and filter fully operational.
* **Which feedback issue it solves:** "No user-defined functions forces verbose repeated patterns... No data structures beyond primitives forces LLMs to work around limitations."
* **Why it improves LLM performance:** Without standard abstractions, LLMs will inevitably try to hallucinate them or abstract logic incorrectly. Providing explicit, native constructs gives the LLM safe patterns to draw from, dramatically reducing hallucinations.

### Change 8: Publish a Normative Machine-Checkable Grammar
* **What to modify:** Replace advisory formatting rules with a machine-readable schema and an automated formatter/validator that forces canonical output.
* **Which feedback issue it solves:** "English-like syntax can tempt a model into writing invalid near-English instead of exact grammar."
* **Why it improves LLM performance:** Advisory rules fail because LLMs drift into natural English. A strict, single-canonical-form validator turns theoretical readability gains into concrete, zero-drift generation guarantees.

---

## 3. Updated Core Language Rules

To achieve maximum LLM reliability, the core language rules are now updated as follows:

1. **One Token, One Meaning:** Every operator and keyword must be a single, globally reserved plain English word. No multi-word operators. No context-sensitive keywords. No symbolic logic operators.
2. **Visual Equals Semantic:** The visual structure of the code (indentation or explicit terminators) must exactly match the parser's semantic tree. No cosmetic indentation.
3. **One Canonical Form:** Every construct has exactly one valid syntactic form. Synonymy and natural-language flexibility are strictly forbidden. 
4. **Total Control Flow:** All branches must be explicit and total. No optional fallbacks or silent no-ops.
5. **Strict Segregation:** Variables must be declared (`let`) before mutation (`set`). Statements and expressions must have zero syntactic overlap.
6. **Native Abstraction:** Repetitive logic must be abstracted via native user-defined functions to prevent hallucinated design patterns.
7. **Validator Domination:** The normative machine-readable grammar dominates prose. All generated code must be capable of round-tripping through the canonical formatter without alteration.