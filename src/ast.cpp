#include "ast.h"

void IdentifierExpr::accept(ASTVisitor& visitor)   const { visitor.visit(*this); }
void StringLiteralExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void NumberExpr::accept(ASTVisitor& visitor)        const { visitor.visit(*this); }
void BoolLiteralExpr::accept(ASTVisitor& visitor)   const { visitor.visit(*this); }
void HoleExpr::accept(ASTVisitor& visitor)          const { visitor.visit(*this); }
void BinaryExpr::accept(ASTVisitor& visitor)        const { visitor.visit(*this); }
void UnaryExpr::accept(ASTVisitor& visitor)         const { visitor.visit(*this); }
void JsonObjectExpr::accept(ASTVisitor& visitor)    const { visitor.visit(*this); }

void IntentDecl::accept(ASTVisitor& visitor)  const { visitor.visit(*this); }
void LetDecl::accept(ASTVisitor& visitor)     const { visitor.visit(*this); }
void SetDecl::accept(ASTVisitor& visitor)     const { visitor.visit(*this); }
void PrintDecl::accept(ASTVisitor& visitor)    const { visitor.visit(*this); }
void ExprStatement::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void RepeatDecl::accept(ASTVisitor& visitor)   const { visitor.visit(*this); }

void StepNode::accept(ASTVisitor& visitor)    const { visitor.visit(*this); }
void PipeDecl::accept(ASTVisitor& visitor)    const { visitor.visit(*this); }

void MatchCaseNode::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void MatchDecl::accept(ASTVisitor& visitor)     const { visitor.visit(*this); }

void ProgramNode::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
