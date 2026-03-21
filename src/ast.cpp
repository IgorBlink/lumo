#include "ast.h"

// ─── Expressions ─────────────────────────────────────────────────────────────
void IdentifierExpr::accept(ASTVisitor& visitor)   const { visitor.visit(*this); }
void StringLiteralExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void NumberExpr::accept(ASTVisitor& visitor)        const { visitor.visit(*this); }
void BoolLiteralExpr::accept(ASTVisitor& visitor)   const { visitor.visit(*this); }
void HoleExpr::accept(ASTVisitor& visitor)          const { visitor.visit(*this); }
void BinaryExpr::accept(ASTVisitor& visitor)        const { visitor.visit(*this); }
void UnaryExpr::accept(ASTVisitor& visitor)         const { visitor.visit(*this); }
void JsonObjectExpr::accept(ASTVisitor& visitor)    const { visitor.visit(*this); }
void ListExpr::accept(ASTVisitor& visitor)          const { visitor.visit(*this); }
void GetExpr::accept(ASTVisitor& visitor)           const { visitor.visit(*this); }
void CallExpr::accept(ASTVisitor& visitor)          const { visitor.visit(*this); }

// ─── Statements ──────────────────────────────────────────────────────────────
void IntentDecl::accept(ASTVisitor& visitor)        const { visitor.visit(*this); }
void LetDecl::accept(ASTVisitor& visitor)           const { visitor.visit(*this); }
void SetDecl::accept(ASTVisitor& visitor)           const { visitor.visit(*this); }
void PrintDecl::accept(ASTVisitor& visitor)         const { visitor.visit(*this); }
void SkipDecl::accept(ASTVisitor& visitor)          const { visitor.visit(*this); }
void ReadDecl::accept(ASTVisitor& visitor)          const { visitor.visit(*this); }
void RepeatDecl::accept(ASTVisitor& visitor)        const { visitor.visit(*this); }
void RepeatTimesDecl::accept(ASTVisitor& visitor)   const { visitor.visit(*this); }
void ForEachDecl::accept(ASTVisitor& visitor)       const { visitor.visit(*this); }
void IfDecl::accept(ASTVisitor& visitor)            const { visitor.visit(*this); }
void FunctionDecl::accept(ASTVisitor& visitor)      const { visitor.visit(*this); }
void ReturnStmt::accept(ASTVisitor& visitor)        const { visitor.visit(*this); }
void CallStmt::accept(ASTVisitor& visitor)          const { visitor.visit(*this); }
void PutDecl::accept(ASTVisitor& visitor)           const { visitor.visit(*this); }

// ─── Pipe ────────────────────────────────────────────────────────────────────
void StepNode::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void PipeDecl::accept(ASTVisitor& visitor) const { visitor.visit(*this); }

// ─── Match ───────────────────────────────────────────────────────────────────
void MatchCaseNode::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void MatchDecl::accept(ASTVisitor& visitor)     const { visitor.visit(*this); }

// ─── Program ─────────────────────────────────────────────────────────────────
void ProgramNode::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
