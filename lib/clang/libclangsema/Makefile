# $FreeBSD$

.include <bsd.own.mk>

LIB=	clangsema

SRCDIR=	tools/clang/lib/Sema
SRCS=	AnalysisBasedWarnings.cpp \
	AttributeList.cpp \
	CodeCompleteConsumer.cpp \
	DeclSpec.cpp \
	DelayedDiagnostic.cpp \
	IdentifierResolver.cpp \
	JumpDiagnostics.cpp \
	MultiplexExternalSemaSource.cpp \
	Scope.cpp \
	ScopeInfo.cpp \
	Sema.cpp \
	SemaAccess.cpp \
	SemaAttr.cpp \
	SemaCXXScopeSpec.cpp \
	SemaCast.cpp \
	SemaChecking.cpp \
	SemaCodeComplete.cpp \
	SemaConsumer.cpp \
	SemaCUDA.cpp \
	SemaDecl.cpp \
	SemaDeclAttr.cpp \
	SemaDeclCXX.cpp \
	SemaDeclObjC.cpp \
	SemaExceptionSpec.cpp \
	SemaExpr.cpp \
	SemaExprCXX.cpp \
	SemaExprMember.cpp \
	SemaExprObjC.cpp \
	SemaFixItUtils.cpp \
	SemaInit.cpp \
	SemaLambda.cpp \
	SemaLookup.cpp \
	SemaObjCProperty.cpp \
	SemaOpenMP.cpp \
	SemaOverload.cpp \
	SemaPseudoObject.cpp \
	SemaStmt.cpp \
	SemaStmtAsm.cpp \
	SemaStmtAttr.cpp \
	SemaTemplate.cpp \
	SemaTemplateDeduction.cpp \
	SemaTemplateInstantiate.cpp \
	SemaTemplateInstantiateDecl.cpp \
	SemaTemplateVariadic.cpp \
	SemaType.cpp \
	TypeLocBuilder.cpp

TGHDRS=	AttrList \
	AttrParsedAttrKinds \
	AttrParsedAttrList \
	AttrParsedAttrImpl \
	AttrSpellingListIndex \
	AttrTemplateInstantiate \
	AttrVisitor \
	Attrs \
	CommentCommandList \
	CommentNodes \
	DeclNodes \
	DiagnosticASTKinds \
	DiagnosticCommentKinds \
	DiagnosticCommonKinds \
	DiagnosticParseKinds \
	DiagnosticSemaKinds \
	StmtNodes \
	arm_neon

# FIXME: Workaround for ARM. clang process
#	 gets too big on this file.
CFLAGS+=${CFLAGS_$*}
.if ${MACHINE_ARCH} == "arm"
CFLAGS_SemaDeclObjC+=-O0
.endif

.include "../clang.lib.mk"
