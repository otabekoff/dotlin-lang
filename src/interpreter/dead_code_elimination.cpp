#include "dotlin/visitors.h"
#include <iostream>
#include <vector>

using namespace dotlin;

Statement::Ptr DeadCodeEliminationVisitor::eliminate(Statement::Ptr stmt)
{
    if (!stmt)
    {
        return nullptr;
    }

    resultStmt = nullptr;
    hasReturn = false;
    hasUnreachable = false;

    stmt->accept(*this);

    if (resultStmt)
    {
        Statement::Ptr res = resultStmt;
        resultStmt = nullptr;
        return res;
    }

    return stmt;
}

void DeadCodeEliminationVisitor::visit(BlockStmt &node)
{
    std::vector<Statement::Ptr> newStatements;

    for (auto &stmt : node.statements)
    {
        if (hasReturn)
        {
            // Skip all statements after a return (unreachable code)
            hasUnreachable = true;
            continue;
        }

        auto eliminatedStmt = eliminate(stmt);

        if (eliminatedStmt)
        {
            newStatements.push_back(eliminatedStmt);

            // If this statement caused a return, we don't need to process more
            if (dynamic_cast<ReturnStmt *>(eliminatedStmt.get()))
            {
                hasReturn = true;
            }
        }
    }

    // If the block ends with a return statement, we can set hasReturn for outer context
    if (!newStatements.empty())
    {
        if (dynamic_cast<ReturnStmt *>(newStatements.back().get()))
        {
            hasReturn = true;
        }
    }

    // Create a new block with cleaned statements
    resultStmt = std::make_shared<BlockStmt>(std::move(newStatements), node.line, node.column);
}

void DeadCodeEliminationVisitor::visit(IfStmt &node)
{
    // Eliminate dead code in branches
    auto newThenBranch = eliminate(node.thenBranch);
    auto newElseBranch = node.elseBranch.has_value() ? eliminate(node.elseBranch.value()) : nullptr;

    // Create new if statement with cleaned branches
    auto newIfStmt = std::make_shared<IfStmt>(
        std::move(node.condition),
        std::move(newThenBranch),
        std::move(newElseBranch),
        node.line,
        node.column);

    resultStmt = newIfStmt;

    // For if statements, we can only say there's a return if BOTH branches return
    // (in which case the code after the if is unreachable)
    // Or if the condition is constant and evaluates to true/false and the corresponding branch returns
}

void DeadCodeEliminationVisitor::visit(WhileStmt &node)
{
    // While loops don't typically cause returns, but we should eliminate dead code inside
    auto newBody = eliminate(node.body);

    auto newWhileStmt = std::make_shared<WhileStmt>(
        std::move(node.condition),
        std::move(newBody),
        node.line,
        node.column);

    resultStmt = newWhileStmt;
}

void DeadCodeEliminationVisitor::visit(ReturnStmt &node)
{
    // Process the return value if it exists
    if (node.value)
    {
        // Note: we can't eliminate expressions in return values through DCE
        // but we can process them for other optimizations if needed
    }

    hasReturn = true;
    resultStmt = std::make_shared<ReturnStmt>(std::move(node.value), node.line, node.column);
}

void DeadCodeEliminationVisitor::visit(ExpressionStmt &node)
{
    if (hasReturn)
    {
        // This statement is unreachable, eliminate it
        resultStmt = nullptr;
        hasUnreachable = true;
        return;
    }

    // Expression statements don't affect control flow
    resultStmt = std::make_shared<ExpressionStmt>(std::move(node.expression), node.line, node.column);
}

void DeadCodeEliminationVisitor::visit(VariableDeclStmt &node)
{
    if (hasReturn)
    {
        // This statement is unreachable, eliminate it
        resultStmt = nullptr;
        hasUnreachable = true;
        return;
    }

    // Variable declarations don't affect control flow
    resultStmt = std::make_shared<VariableDeclStmt>(
        node.isVal, // Add the missing isVal parameter
        node.name,
        node.typeAnnotation,
        std::move(node.initializer),
        node.line,
        node.column);
}

void DeadCodeEliminationVisitor::visit(FunctionDeclStmt &node)
{
    // Function declarations are processed at global level, not eliminated as dead code
    // But we can eliminate dead code inside the function body
    if (node.body)
    {
        auto newBody = eliminate(node.body);
        resultStmt = std::make_shared<FunctionDeclStmt>(
            node.name,
            node.parameters,
            std::move(newBody),
            node.returnType,
            node.line,
            node.column);
    }
    else
    {
        resultStmt = std::make_shared<FunctionDeclStmt>(
            node.name,
            node.parameters,
            std::move(node.body),
            node.returnType,
            node.line,
            node.column);
    }
}

void DeadCodeEliminationVisitor::visit(ClassDeclStmt &node)
{
    // For class declarations, we can process members for dead code elimination
    std::vector<Statement::Ptr> newMembers;
    for (auto &member : node.members)
    {
        auto eliminatedMember = eliminate(member);
        if (eliminatedMember)
        {
            newMembers.push_back(eliminatedMember);
        }
    }

    resultStmt = std::make_shared<ClassDeclStmt>(
        node.name,
        std::move(newMembers),
        node.superClass,
        node.line,
        node.column);
}

void DeadCodeEliminationVisitor::visit(ForStmt &node)
{
    // For loops don't typically cause returns, but we should eliminate dead code inside
    auto newBody = eliminate(node.body);

    auto newForStmt = std::make_shared<ForStmt>(
        node.variable,
        std::move(node.iterable),
        std::move(newBody),
        node.line,
        node.column);

    resultStmt = newForStmt;
}

void DeadCodeEliminationVisitor::visit(WhenStmt &node)
{
    // Process all branches of the when statement
    std::vector<std::pair<Expression::Ptr, Statement::Ptr>> newBranches;
    for (auto &branch : node.branches)
    {
        auto newBranchBody = eliminate(branch.second);
        newBranches.emplace_back(std::move(branch.first), std::move(newBranchBody));
    }

    Statement::Ptr newElseBranch = nullptr;
    if (node.elseBranch)
    {
        newElseBranch = eliminate(node.elseBranch.value());
    }

    auto newWhenStmt = std::make_shared<WhenStmt>(
        std::move(node.subject),
        std::move(newBranches),
        std::move(newElseBranch),
        node.line,
        node.column);

    resultStmt = newWhenStmt;
}

void DeadCodeEliminationVisitor::visit(TryStmt &node)
{
    // Process the try block, catch block, and finally block
    auto newTryBlock = eliminate(node.tryBlock);

    auto newCatchBlock = eliminate(node.catchBlock);

    Statement::Ptr newFinallyBlock = nullptr;
    if (node.finallyBlock)
    {
        newFinallyBlock = eliminate(node.finallyBlock.value());
    }

    auto newTryStmt = std::make_shared<TryStmt>(
        std::move(newTryBlock),
        node.exceptionVar,
        std::move(newCatchBlock),
        std::move(newFinallyBlock),
        node.line,
        node.column);

    resultStmt = newTryStmt;
}

void DeadCodeEliminationVisitor::visit(ConstructorDeclStmt &node)
{
    // Process constructor body for dead code elimination
    Statement::Ptr newBody = nullptr;
    if (node.body)
    {
        newBody = eliminate(node.body);
    }

    resultStmt = std::make_shared<ConstructorDeclStmt>(
        node.parameters,
        std::move(newBody),
        node.line,
        node.column);
}

// Expression visits (needed for completeness but don't eliminate expressions themselves)
void DeadCodeEliminationVisitor::visit(LiteralExpr &node)
{
    (void)node;
    resultStmt = nullptr;
}

void DeadCodeEliminationVisitor::visit(StringInterpolationExpr &node)
{
    (void)node;
    resultStmt = nullptr;
}

void DeadCodeEliminationVisitor::visit(IdentifierExpr &node)
{
    (void)node;
    resultStmt = nullptr;
}

void DeadCodeEliminationVisitor::visit(LambdaExpr &node)
{
    (void)node;
    resultStmt = nullptr;
}

void DeadCodeEliminationVisitor::visit(BinaryExpr &node)
{
    (void)node;
    resultStmt = nullptr;
}

void DeadCodeEliminationVisitor::visit(UnaryExpr &node)
{
    (void)node;
    resultStmt = nullptr;
}

void DeadCodeEliminationVisitor::visit(CallExpr &node)
{
    (void)node;
    resultStmt = nullptr;
}

void DeadCodeEliminationVisitor::visit(MemberAccessExpr &node)
{
    (void)node;
    resultStmt = nullptr;
}

void DeadCodeEliminationVisitor::visit(ArrayLiteralExpr &node)
{
    (void)node;
    resultStmt = nullptr;
}

void DeadCodeEliminationVisitor::visit(ArrayAccessExpr &node)
{
    (void)node;
    resultStmt = nullptr;
}