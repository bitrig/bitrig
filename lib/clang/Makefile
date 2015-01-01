# $FreeBSD$

.include <bsd.own.mk>

.if !make(install)
.if !defined(EARLY_BUILD) && defined(MK_CLANG_FULL) && ${MK_CLANG_FULL:L} != "no"
_libclangstaticanalyzer= \
	libclangstaticanalyzercheckers \
	libclangstaticanalyzercore \
	libclangstaticanalyzerfrontend
_libclangarcmigrate= \
	libclangarcmigrate
.endif # !EARLY_BUILD && MK_CLANG_FULL

SUBDIR=	libclanganalysis \
	${_libclangarcmigrate} \
	libclangast \
	libclangbasic \
	libclangcodegen \
	libclangdriver \
	libclangedit \
	libclangfrontend \
	libclangfrontendtool \
	libclanglex \
	libclangparse \
	libclangrewrite \
	libclangrewritefrontend \
	libclangsema \
	libclangserialization \
	${_libclangstaticanalyzer} \
	\
	libllvmanalysis \
	libllvmasmparser \
	libllvmasmprinter \
	libllvmbitreader \
	libllvmbitwriter \
	libllvmcodegen \
	libllvmcore \
	libllvminstcombine \
	libllvminstrumentation \
	libllvmipa \
	libllvmipo \
	libllvmirreader \
	libllvmlinker \
	libllvmmc \
	libllvmmcparser \
	libllvmobjcarcopts \
	libllvmobject \
	libllvmoption \
	libllvmprofiledata \
	libllvmscalaropts \
	libllvmselectiondag \
	libllvmsupport \
	libllvmtablegen \
	libllvmtarget \
	libllvmtransformutils \
	libllvmvectorize \
	\
	libllvmaarch64asmparser \
	libllvmaarch64codegen \
	libllvmaarch64desc \
	libllvmaarch64disassembler \
	libllvmaarch64info \
	libllvmaarch64instprinter \
	libllvmaarch64utils \
	\
	libllvmarmasmparser \
	libllvmarmcodegen \
	libllvmarmdesc \
	libllvmarmdisassembler \
	libllvmarminfo \
	libllvmarminstprinter \
	libllvmmipsasmparser \
	libllvmmipscodegen \
	libllvmmipsdesc \
	libllvmmipsdisassembler \
	libllvmmipsinfo \
	libllvmmipsinstprinter \
	libllvmpowerpccodegen \
	libllvmpowerpcdisassembler \
	libllvmpowerpcdesc \
	libllvmpowerpcinfo \
	libllvmpowerpcinstprinter \
	libllvmx86asmparser \
	libllvmx86codegen \
	libllvmx86desc \
	libllvmx86disassembler \
	libllvmx86info \
	libllvmx86instprinter \
	libllvmx86utils

.if ${MK_CLANG_EXTRAS:L} != "no"
SUBDIR+=libllvmdebuginfo \
	libllvmexecutionengine \
	libllvminterpreter \
	libllvmmcdisassembler \
	libllvmmcjit \
	libllvmruntimedyld
.endif # MK_CLANG_EXTRAS
.endif # !make(install)

SUBDIR+= include

.include <bsd.subdir.mk>
