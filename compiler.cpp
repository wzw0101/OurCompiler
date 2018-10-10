#include "llvm/ADT/STLExtras.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
	//文件结束符
	tok_eof = -1,

	// commands
	//函数定义符
	tok_func = -2,
	//函数申明符
	tok_extern = -3,

	// primary
	//标识符
	tok_identifier = -4,
	//数字
	tok_number = -5,
	//return
	tok_return = -6,
	//print
	
	tok_print = -7,
	//在这里补充VSL的关键字FUNC等....
	tok_continue = -8,
	tok_if = -9,
	tok_then = -10,
	tok_else = -11,
	tok_fi = -12,
	tok_while = -13,
	tok_do = -14,
	tok_done = -15,
	tok_var = -16
};

static std::string IdentifierStr; // Filled in if tok_identifier
static double NumVal;             // Filled in if tok_number

/// gettok - Return the next token from standard input.
/// 请修改gettok代码，在读到合适的单词后，准确分析它的语法属性，返回词性。
static int gettok() {
	static int LastChar = ' ';

	// Skip any whitespace.
	while (isspace(LastChar))
		LastChar = getchar();

	if (isalpha(LastChar)) { //字母
		IdentifierStr = LastChar;
		while (isalnum((LastChar = getchar())))//字母或数字
			IdentifierStr += LastChar;
		//以下考虑strcmp()函数的使用
		if (IdentifierStr == "FUNC")
			return tok_func;
		if (IdentifierStr == "extern")
			return tok_extern;
		if (IdentifierStr == "PRINT")
			return tok_print;
		if (IdentifierStr == "RETURN")
			return tok_return;
		if (IdentifierStr == "CONTINUE")
			return tok_continue;
		if (IdentifierStr == "IF")
			return tok_if;
		if (IdentifierStr == "THEN")
			return tok_then;
		if (IdentifierStr == "ELSE")
			return tok_else;
		if (IdentifierStr == "FI")
			return tok_fi;
		if (IdentifierStr == "WHILE")
			return tok_while;
		if (IdentifierStr == "DO")
			return tok_do;
		if (IdentifierStr == "DONE")
			return tok_done;
		if (IdentifierStr == "VAR")
			return tok_var;
		return tok_identifier;
	}

	//此处应修改，避免出现1.2.3仍能通过的情况，修改后请删去本行
	if (isdigit(LastChar) || LastChar == '.') { // 数字
		std::string NumStr;
		int num_point=0;
			do {
				if (LastChar == '.')
					num_point++;
				NumStr += LastChar;
				LastChar = getchar();
			} while (isdigit(LastChar) || LastChar == '.');
			if (num_point < 2) {
				NumVal = strtod(NumStr.c_str(), nullptr);
				return tok_number;
			}
			else {
				fprintf(stderr,"Invalid demical");
				return 0;
			}
	}

	//此处修改对注释的处理，不再是'#'，而是"//"
	//对注释的处理已修改为"//"
	if (LastChar == '/') {
		//如果是除法，返回该除法符号
		if ((LastChar = getchar()) != '/') 
			return '/';

		// Comment until end of line.
		do
			LastChar = getchar();
		while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

		//此处考虑/r/n的情况
		if (LastChar != EOF)
			return gettok();
	}

	// Check for end of file.  Don't eat the EOF.
	if (LastChar == EOF)
		return tok_eof;

	// Otherwise, just return the character as its ascii value.
	int ThisChar = LastChar;
	LastChar = getchar();
	return ThisChar;
}

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//

namespace {
	/// ExprAST - Base class for all expression nodes.
	class ExprAST {
	public:
		virtual ~ExprAST() = default;
		virtual void printAST() {};
	};

	/// NumberExprAST - Expression class for numeric literals like "1.0".
	class NumberExprAST : public ExprAST {
		double Val;

	public:
		NumberExprAST(double Val) : Val(Val) {}
		virtual void printAST() {
			//输出数字常量结点
		};
	};

	///StringAST
	class StringAST : public ExprAST {
		std::string str;

	public:
		StringAST(std::string str) : str(str) {}
		virtual void printAST() {
			//输出字符串结点
		};
	};

	/// VariableExprAST - Expression class for referencing a variable, like "a".
	class VariableExprAST : public ExprAST {
		std::string Name;

	public:
		VariableExprAST(const std::string &Name) : Name(Name) {}
		virtual void printAST() {
			//输出变量结点
		}
	};

	/// DeclareExprAST - Expression like 'VAR x,y,z'.
	class DeclareExprAST : public ExprAST {
		std::vector<std::string> Names;
	public:
		DeclareExprAST(const std::vector<std::string> &Names) : Names(Names) {}
		virtual void printAST() {
			//输出声明表达式结点
		}
	};

	/// AssignExpr - 负责处理赋值表达式
	class AssignExpr : public ExprAST {
		std::string Ident;
		std::unique_ptr<ExprAST> Expr;
	public:
		AssignExpr(std::string Ident, std::unique_ptr<ExprAST> Expr)
		:Ident(Ident),Expr(std::move(Expr)){}
		virtual void printAST() {
			//输出赋值表达式结点
		}
	};

	/// BinaryExprAST - Expression class for a binary operator.
	class BinaryExprAST : public ExprAST {
		char Op;
		std::unique_ptr<ExprAST> LHS, RHS;

	public:
		BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
			std::unique_ptr<ExprAST> RHS)
			: Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
		virtual void printAST() {
			//输出运算表达式结点
		}
	};

	/// CallExprAST - Expression class for function calls.
	class CallExprAST : public ExprAST {
		std::string Callee;
		std::vector<std::unique_ptr<ExprAST>> Args;

	public:
		CallExprAST(const std::string &Callee,
			std::vector<std::unique_ptr<ExprAST>> Args)
			: Callee(Callee), Args(std::move(Args)) {}
		virtual void printAST() {
			//输出函数调用表达式结点
		}
	};
	
	/// PrototypeAST - This class represents the "prototype" for a function,
	/// which captures its name, and its argument names (thus implicitly the number
	/// of arguments the function takes).
	class PrototypeAST {
		std::string Name;
		std::vector<std::string> Args;

	public:
		PrototypeAST(const std::string &Name, std::vector<std::string> Args)
			: Name(Name), Args(std::move(Args)) {}

		const std::string &getName() const { return Name; }
		virtual void printAST() {
			//输出函数签名结点
		}
	};

	/// FunctionAST - This class represents a function definition itself.
	class FunctionAST : public ExprAST {
		std::unique_ptr<PrototypeAST> Proto;
		// std::unique_ptr<ExprAST> Body; 
		// 函数的定义被修改为“签名” + “语句块”的形式
		std::unique_ptr<ExprAST> Body;
	public:
		FunctionAST(std::unique_ptr<PrototypeAST> Proto,
			std::unique_ptr<ExprAST> Body)
			: Proto(std::move(Proto)), Body(std::move(Body)) {}
		virtual void printAST() {
			//输出函数定义结点
		}
	};
	
	///ExprsAST - 语句块表达式结点
	class ExprsAST : public ExprAST {
		std::vector<std::unique_ptr<ExprAST>> Stats; //多句表达式向量
	public:
		ExprsAST(std::vector<std::unique_ptr<ExprAST>> Stats)
			:Stats(std::move(Stats)) {}
		virtual void printAST() {
			//输出语句块结点
		}
	};
	
	///RetStatAST - 返回语句结点
	class RetStatAST : public ExprAST {
		std::unique_ptr<ExprAST> Expr; // 返回语句后面的表达式
	public:
		RetStatAST(std::unique_ptr<ExprAST> Expr) : Expr(std::move(Expr)) {}
		virtual void printAST() {
			//输出返回表达式结点
		}
	};

	/// PrtStatAST - 打印语句结点
	/// 打印语句后面的多个待输出表达式或字符串： PRINT print_item1, print_item2...
	class PrtStatAST : public ExprAST {
		std::vector<std::unique_ptr<ExprAST>> Args;
	public:
		PrtStatAST(std::vector<std::unique_ptr<ExprAST>> Args) : Args(std::move(Args)) {}
		virtual void printAST() {
			//输出打印表达式结点
		}
	};

	/// NullStatAST - 空语句结点
	class NullStatAST : public ExprAST {
	public:
		NullStatAST() {}
		virtual void printAST() {
			//输出Continue语句结点
		}
	};

	/// IfStatAST - 条件语句结点
	class IfStatAST : public ExprAST {
		std::unique_ptr<ExprAST> Cond; // 条件表达式
		std::unique_ptr<ExprAST> ThenStat; // Cond 为True后的执行语句块

		std::unique_ptr<ExprAST> ElseStat; // Cond 为False后的执行语句块
	public:
		IfStatAST(std::unique_ptr<ExprAST> Cond,
			std::unique_ptr<ExprAST> ThenStat,
			std::unique_ptr<ExprAST> ElseStat)
			:Cond(std::move(Cond)),
			ThenStat(std::move(ThenStat)),
			ElseStat(std::move(ElseStat)) {}
		virtual void printAST() {
			//输出条件语句结点
		}
	};

	/// WhileStatAST - 当循环语句结点
	class WhileStatAST : public ExprAST {
		std::unique_ptr<ExprAST> Cond; // 条件表达式
		std::unique_ptr<ExprAST> Stat; // Cond 为True后的执行语句块
	public:
		WhileStatAST(std::unique_ptr<ExprAST> Cond,
			std::unique_ptr<ExprAST> Stat)
			:Cond(std::move(Cond)), Stat(std::move(Stat)) {}
		virtual void printAST() {
			//输出当循环语句结点
		}
	};

} // end anonymous namespace

  //===----------------------------------------------------------------------===//
  // Parser
  //===----------------------------------------------------------------------===//

  /// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
  /// token the parser is looking at.  getNextToken reads another token from the
  /// lexer and updates CurTok with its results.
static int CurTok;
static int getNextToken() { return CurTok = gettok(); }

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
static std::map<char, int> BinopPrecedence;

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
static int GetTokPrecedence() {
	if (!isascii(CurTok))
		return -1;

	// Make sure it's a declared binop.
	int TokPrec = BinopPrecedence[CurTok];
	if (TokPrec <= 0)
		return -1;
	return TokPrec;
}

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(const char *Str) {
	fprintf(stderr, "Error: %s\n", Str);
	return nullptr;
}
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
	LogError(Str);
	return nullptr;
}

static std::unique_ptr<ExprAST> ParseNullExpr();
static std::unique_ptr<ExprAST> ParseNumberExpr();
static std::unique_ptr<ExprAST> ParseParenExpr();
static std::unique_ptr<ExprAST> ParseIdentifierExpr();
static std::unique_ptr<ExprAST> ParseBinOpRHS(int, std::unique_ptr<ExprAST>);
static std::unique_ptr<ExprAST> ParseExpression();
static std::unique_ptr<ExprAST> ParseReturnExpr();
static std::unique_ptr<ExprAST> ParsePrintExpr();
static std::unique_ptr<ExprAST> ParseWhileExpr();
static std::unique_ptr<ExprAST> ParseIfExpr();
static std::unique_ptr<ExprAST> ParseDclrExpr();
static std::unique_ptr<ExprAST> ParsePrimary();
static std::unique_ptr<PrototypeAST> ParsePrototype();
static std::unique_ptr<FunctionAST> ParseTopLevelExpr();
static std::unique_ptr<PrototypeAST> ParseExtern();
static std::unique_ptr<ExprsAST> ParseStats();
static std::unique_ptr<ExprAST> ParseStat();
static std::unique_ptr<ExprAST> ParseString();

///print语句中字符串节点
static std::unique_ptr<ExprAST> ParseString()
{
	std::string str = "";
	char c;
	while ((c = getchar()) != '"') 
	{
		str += c;
	}
    auto Result = llvm::make_unique<StringAST>(str);
	return std::move(Result);
}

/// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr() {
	auto Result = llvm::make_unique<NumberExprAST>(NumVal);
	getNextToken(); // consume the number
	return std::move(Result);
}

/// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr() {
	getNextToken(); // eat (.
	auto V = ParseExpression();
	if (!V)
		return nullptr;

	if (CurTok != ')')
		return LogError("expected ')'");
	getNextToken(); // eat ).
	return V;
}

/// NullExpr ::= CONTINUE;
static std::unique_ptr<ExprAST> ParseNullExpr() {
	getNextToken();
	return llvm::make_unique<NullStatAST>();
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
///   ::= identifier ':= ' expression
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
	std::string IdName = IdentifierStr;

	getNextToken(); // eat identifier.

	if (CurTok != '(') // Simple variable ref.
		return llvm::make_unique<VariableExprAST>(IdName);

	//如果标志符后为赋值符号
	if (CurTok == ':')
	{
		getNextToken();
		//未考虑异常情况！修改后删除本行注释
		if (CurTok == '=')
		{
			//滤掉'='
			getNextToken();
			std::unique_ptr<ExprAST> RHS = ParseExpression();
			if (!RHS) {
				return llvm::make_unique<AssignExpr>(IdName, std::move(RHS));
			}				
		}
	}
	// Call.
	getNextToken(); // eat (
	std::vector<std::unique_ptr<ExprAST>> Args;
	//该if语句可以优化
	if (CurTok != ')') {
		while (true) {
			if (auto Arg = ParseExpression())
				Args.push_back(std::move(Arg));
			else
				return nullptr;

			if (CurTok == ')')
				break;

			if (CurTok != ',')
				return LogError("Expected ')' or ',' in argument list");
			getNextToken();
		}
	}

	// Eat the ')'.
	getNextToken();

	return llvm::make_unique<CallExprAST>(IdName, std::move(Args));
}

//ParseReturnExpr - 实现返回语句
static std::unique_ptr<ExprAST> ParseReturnExpr()
{
	if (CurTok == tok_return)
	{
		getNextToken();
		std::unique_ptr<ExprAST> Expr = ParseExpression();
		if (Expr)
		{
			//auto Result = new RetStatAST(std::move(Expr));
			return llvm::make_unique<RetStatAST>(std::move(Expr));
		}
	}
}

//ParsePrintExpr - 实现打印语句
static std::unique_ptr<ExprAST> ParsePrintExpr()
{
	std::vector <std::unique_ptr<ExprAST>> Args;
	if (CurTok == tok_print)
	{
		//滤去PRINT
		getNextToken();
		while (getNextToken() != '#')
		{
			if (CurTok == '"')
			{
				Args.push_back(ParseString());
			}
			if (CurTok == tok_number)
			{
				Args.push_back(ParseNumberExpr());
			}
			if (CurTok == tok_identifier)
			{
				Args.push_back(ParsePrimary());
			
			}
		}
		if (CurTok == '#')
		{
			return llvm::make_unique<PrtStatAST>(std::move(Args));
		}
	}
}
//@铁男 代码逻辑问题
//ParseWhileExpr - 实现While循环
static std::unique_ptr<ExprAST> ParseWhileExpr() {
	std::unique_ptr<ExprAST> Cond, Stat;
	std::unique_ptr<WhileStatAST> WhilePtr = llvm::make_unique<WhileStatAST>(std::move(Cond), std::move(Stat));
	ParseParenExpr();
	CurTok = getNextToken();
	if (CurTok == tok_do) {
		CurTok = getNextToken();
		switch (CurTok) {
		case tok_identifier:
		case tok_if:
		case tok_while:ParseStat(); break;
		case '{':ParseStats(); break;
		default:return LogError("Not A WHile Parser!"); break;
		}

	}
		else return LogError("Expect 'then'!");
		CurTok = getNextToken();
		if (CurTok == tok_done) return WhilePtr;
		else return LogError("Expect 'FI'!");//读到done可以安全退出
}
//@铁男 代码逻辑问题
//ParseIfExpr - 实现If判断
static std::unique_ptr<ExprAST> ParseIfExpr() {	
	std::unique_ptr<ExprAST> Cond, ThenStat, ElseStat;
	std::unique_ptr<IfStatAST> IfPtr = llvm::make_unique<IfStatAST>(std::move(Cond), std::move(ThenStat), std::move(ElseStat));
	ParseParenExpr(); //分析if后面的条件
	getNextToken();
	if (CurTok == tok_then ) {
		getNextToken();
		switch (CurTok) {
		case tok_identifier:
		case tok_if:
		case tok_while:ParseStat(); break;
		case '{':ParseStats(); break;
		default:return LogError("Not A If Parser!"); break;
		}
	}
	else return LogError("Expect 'DO'!");
		CurTok = getNextToken();
		if (CurTok == tok_fi) return IfPtr;
		else return LogError("Expect 'DONE'!");//读到fi可以安全退出
}

//ParseDclrExpr - 实现变量声明语句解析
static std::unique_ptr<ExprAST> ParseDclrExpr() {
	getNextToken();
	std::vector<std::string> Names;
	while (CurTok == tok_identifier) {
		Names.push_back(IdentifierStr);
		getNextToken();
		if (CurTok == ',') getNextToken();//eat ','
	}
	if (Names.empty()) {
		LogError("Need var names");
		return nullptr;
	}
	else
		return llvm::make_unique<DeclareExprAST>(std::move(Names));
}

/// primary 是一个表达式中的基本单元，包括identifierexpr（变量， 函数调用， 赋值表达式）, numberexpr, parenexpr
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary() {
	switch (CurTok) {
	default:
		return LogError("unknown token when expecting an expression");
	case tok_identifier:
		return ParseIdentifierExpr();
	case tok_number:
		return ParseNumberExpr();
	case '(':
		return ParseParenExpr();
	}
}

/// binoprhs
///   ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
	std::unique_ptr<ExprAST> LHS) {
	// If this is a binop, find its precedence.
	while (true) {
		int TokPrec = GetTokPrecedence();

		// If this is a binop that binds at least as tightly as the current binop,
		// consume it, otherwise we are done.
		if (TokPrec < ExprPrec)
			return LHS;

		// Okay, we know this is a binop.
		int BinOp = CurTok;
		getNextToken(); // eat binop

						// Parse the primary expression after the binary operator.
		auto RHS = ParsePrimary();
		if (!RHS)
			return nullptr;

		// If BinOp binds less tightly with RHS than the operator after RHS, let
		// the pending operator take RHS as its LHS.
		int NextPrec = GetTokPrecedence();
		if (TokPrec < NextPrec) {
			RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
			if (!RHS)
				return nullptr;
		}

		// Merge LHS/RHS.
		LHS = llvm::make_unique<BinaryExprAST>(BinOp, std::move(LHS),
			std::move(RHS));
	}
}

/// expression
///   ::= primary binoprhs
///
static std::unique_ptr<ExprAST> ParseExpression() {
	//这里如果读到LHS为空，检查后面是否为'-'
	std::unique_ptr<ExprAST> LHS = llvm::make_unique<ExprAST>();
	if (CurTok != '-') {
		LHS = ParsePrimary();
		if (!LHS)
			return nullptr;
	}

	return ParseBinOpRHS(0, std::move(LHS));
}

/// prototype
///   ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype() {
	if (CurTok != tok_identifier)
		return LogErrorP("Expected function name in prototype");

	std::string FnName = IdentifierStr;
	getNextToken();

	if (CurTok != '(')
		return LogErrorP("Expected '(' in prototype");
	//VSL语言参数以','间隔，此处修改
	//已修改 段
	std::vector<std::string> ArgNames;
	getNextToken();//eat '('
	while (CurTok == tok_identifier) {
		ArgNames.push_back(IdentifierStr);
		if (getNextToken() == ',') getNextToken();//eat ','
	}
	if (CurTok != ')')
		return LogErrorP("Expected ')' in prototype");

	// success.
	getNextToken(); // eat ')'.

	return llvm::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

/// definition ::= 'FUNC' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition() {
	getNextToken(); // eat FUNC.
	auto Proto = ParsePrototype();
	if (!Proto)
		return nullptr;
	
	//此处改为分析函数体的语句
	if (auto E = ParseStats())
		return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(E));
	return nullptr;
}

/// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
	if (auto E = ParseExpression()) {
		// Make an anonymous proto.
		auto Proto = llvm::make_unique<PrototypeAST>("__anon_expr",
			std::vector<std::string>());
		return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(E));
	}
	return nullptr;
}

/// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern() {
	getNextToken(); // eat extern.
	return ParsePrototype();
}

/// ParseStats - 分析语句块的函数
static std::unique_ptr<ExprsAST> ParseStats() {
	std::vector<std::unique_ptr<ExprAST>> Stats;
	if (CurTok != '{') {
		Stats.push_back(std::move(ParseStat()));
		return llvm::make_unique<ExprsAST>(std::move(Stats));
	}
	else {
		//仅考虑正常语法情况，需增加异常处理
		while (getNextToken() != '}') {
			Stats.push_back(std::move(ParseStat()));
		}
		//消耗掉'}'
		getNextToken();
		return llvm::make_unique<ExprsAST>(std::move(Stats));
	}
}

/// ParseStat - 分析单条语句的函数
static std::unique_ptr<ExprAST> ParseStat() {
	switch (CurTok)
	{
		//VAR
	case tok_var:
		return ParseDclrExpr();
		//IF
	case tok_if:
		return ParseIfExpr();

		//WHILE
	case tok_while:
		return ParseWhileExpr();

		//IDENTI
	case tok_identifier:
		return ParseIdentifierExpr();

		//RETURN
	case tok_return:
		return ParseReturnExpr();

		//PRINT
	case tok_print:
		return ParsePrintExpr();

	default:
		//违背语法，报错
		LogError("Unknown Statement!");
		return nullptr;
	}
}


//===----------------------------------------------------------------------===//
// Top-Level parsing
//===----------------------------------------------------------------------===//

static void HandleContinue() {
	if (ParseNullExpr()) {
		fprintf(stderr, "Parsed a null expression.\n");
	}
	else {
		// Skip token for error recovery.
		getNextToken();
	}
}

static void HandleDeclaration() {
	if (ParseDclrExpr()) {
		fprintf(stderr, "Parsed a declaration statement.\n");
	}
	else {
		// Skip token for error recovery.
		getNextToken();
	}
}

static void HandleDefinition() {
	if (ParseDefinition()) {
		fprintf(stderr, "Parsed a function definition.\n");
	}
	else {
		// Skip token for error recovery.
		getNextToken();
	}
}

static void HandleExtern() {
	if (ParseExtern()) {
		fprintf(stderr, "Parsed an extern\n");
	}
	else {
		// Skip token for error recovery.
		getNextToken();
	}
}

static void HandleTopLevelExpression() {
	// Evaluate a top-level expression into an anonymous function.
	if (ParseTopLevelExpr()) {
		fprintf(stderr, "Parsed a top-level expr\n");
	}
	else {
		// Skip token for error recovery.
		getNextToken();
	}
}

static void HandleIf() {
	if (ParseIfExpr()) {
		fprintf(stderr, "Parse a if statement\n");
	}
	else {
		//Skip token for error recovery.
		getNextToken();
	}
}

static void HandleWhile() {
	if (ParseWhileExpr()) {
		fprintf(stderr, "Parse a while statement\n");
	}
	else {
		//Skip token for error recovery.
		getNextToken();
	}
}

static void HandlePrint() {
	if (ParsePrintExpr()) {
		fprintf(stderr, "Parse a print statement\n");
	}
	else {
		//Skip token for error recovery.
		getNextToken();
	}
}

static void HandleReturn() {
	if (ParseReturnExpr()) {
		fprintf(stderr, "Parse a return statement\n");
	}
	else {
		//Skip token for error recovery.
		getNextToken();
	}
}

/// top ::= definition | external | expression | ';'
/*
static void MainLoop() {
	while (true) {
		switch (CurTok) {
		case tok_eof:
			return;
		case ';': // ignore top-level semicolons.
			getNextToken();
			break;
		case tok_func:
			HandleDefinition();
			break;
		case tok_extern:
			HandleExtern();
			break;
		case tok_if:
			HandleIf();
			break;
		case tok_while:
			HandleWhile();
			break;
		case tok_print:
			HandlePrint();
			break;
		case tok_return:
			HandleReturn();
			break;
		case tok_continue:
			HandleContinue();
			break;
		case tok_var:
			HandleDeclaration();
			break;
		case '{':
			ParseStats();
			break;
		default:
			HandleTopLevelExpression();
			break;
		}
	}
	fprintf(stderr, "ready> ");
}
*/
static void MainLoop() {
	while (true) {
		switch (CurTok) {
		case tok_func:
			HandleDefinition();
			break;
		case tok_identifier:
			ParseIdentifierExpr();
			break;
		case tok_return:
			HandleReturn();
			break;
		case tok_print:
			HandlePrint();
			break;
		case tok_if:
			HandleIf();
			break;
		case tok_while:
			HandleWhile();
			break;
		case tok_var:
			HandleDeclaration();
			break;
		case tok_eof:
			return;
		case ('#'):
			return;
		//非函数体报错
		default:
			LogError("Error!Expected a function definition");
			break;
		}
	}
	fprintf(stderr, "ready> ");
}
//===----------------------------------------------------------------------===//
// Program Parse Code
//===----------------------------------------------------------------------===//

static std::unique_ptr<ExprAST> ParseProgram() {
	
	//存储程序中的所有函数或其它语句
	std::vector<std::unique_ptr<ExprAST>> func_list;
	while (true) {
		switch (CurTok)
		{
		case tok_func:
		{
			auto E = ParseDefinition();
			func_list.push_back(std::move(E));
		}
		case '#'://分析结束，返回程序语法树
			return llvm::make_unique<ExprsAST>(std::move(func_list));
		}
	}
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main() {
	// Install standard binary operators.
	// 1 is lowest precedence.
	BinopPrecedence['+'] = 20;
	BinopPrecedence['-'] = 20;
	BinopPrecedence['*'] = 40;
	BinopPrecedence['/'] = 40;// highest.
	//这里增加运算符的优先级


	// Prime the first token.
	fprintf(stderr, "ready> ");
	getNextToken();

	// Run the main "interpreter loop" now.
	MainLoop();
	//ParseProgram();

	return 0;
}