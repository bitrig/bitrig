" Copyright 2004, 2005, 2006, 2009
" Free Software Foundation, Inc.
"
" Copying and distribution of this file, with or without modification,
" are permitted in any medium without royalty provided the copyright
" notice and this notice are preserved.

" Vim syntax file
" Language:    mom
" Maintainer:  Christian V. J. Brüssow <cvjb@cvjb.de>
" Last Change: So 06 Mär 2005 17:28:13 CET
" Filenames:   *.mom
" URL:         http://www.cvjb.de/comp/vim/mom.vim
" Note:        Remove or overwrite troff syntax for *.mom-files with filetype/filedetect.
" Version:     0.1
"
" Mom: Macro set for easy typesetting with troff/nroff/groff.

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
	syntax clear
elseif exists("b:current_syntax")
	finish
endif

" Mom is case sensitive
syn case match

" Synchronization, I know it is a huge number, but normal texts can be
" _very_ long ;-)
syn sync lines=1000

" Characters allowed in keywords
if version >= 600
	setlocal iskeyword=@,#,$,%,48-57,.,@-@,_,192-255
else
	set iskeyword=@,#,$,%,48-57,.,@-@,_,192-255
endif

" Some special keywords
syn keyword momTodo contained TODO FIXME
syn keyword momDefine .de .. .ALIAS .ALIASN

" Preprocessor keywords
syn keyword momPreprocessor .EQ .EN .GS .GE .GF .PS .PE .R1 .R2 .TS .TE .TH
syn keyword momPreprocessor .G1 .G2 .IS .IE .cstart .cend

" Number Registers
syn match momNumberReg '\.#[A-Za-z][_0-9A-Za-z]*'

" String Registers
syn match momStringReg '\.\$[A-Za-z][_0-9A-Za-z]*'

" Strings
syn region momString start='"' end='"' contains=momNoLineBreak,momGreek,momInteger,momFloatEN,momFloatDE,momBracketRegion,momBracketError,momSpecialMove

" Special characters
syn match momSpecialChar '\\([-+A-Za-z0-9*<>=~!]\+'

" Greek symbols
syn match momGreek '\\(\*[A-Za-z]\+'

" Hyphenation marks
syn match momHyphenation '\\%'

" Masking of line breaks
syn match momNoLineBreak '\\\s*$'

" Numbers (with optional units)
syn match momInteger '[-+]\=[0-9]\+[iPpv]\='
syn match momFloatEN '[-+]\=[0-9]*\.[0-9]\+[iPpv]\='
syn match momFloatDE '[-+]\=[0-9]\+,[0-9]\+'

" Mom Macros
syn match momKeyword '\(^\|\s\+\)\.[A-Za-z][_0-9A-Za-z]*'
syn match momKeywordParam '\(^\|\s\+\)\.[A-Za-z][_0-9A-Za-z]*\s\+[^-\\"]\+' contains=momInteger,momFloatEN,momString,momSpecialParam
syn keyword momSpecialParam contained ON OFF T H C R I B L J N QUAD CLEAR NAMED DRAFT FINAL DEFAULT TYPESET TYPEWRITE CHAPTER BLOCK

" Brackets
syn match momBrackets '[[]]'
syn match momBracketError '\]'
syn region momBracketRegion transparent matchgroup=Delimiter start='\[' matchgroup=Delimiter end='\]' contains=ALLBUT,momBracketError

" Special movements, e.g. \*[BU<#>] or \*[BP<#>]
syn region momSpecialMove matchgroup=Delimiter start='\\\*\[' matchgroup=Delimiter end='\]' contains=ALLBUT,momBracketError

" Quotes
syn region momQuote matchgroup=momKeyword start='\<\.QUOTE\>' matchgroup=momKeyword end='\<\.QUOTE\s\+OFF\>' skip='$' contains=ALL
syn region momBlockQuote matchgroup=momKeyword start='\<\.BLOCKQUOTE\>' matchgroup=momKeyword end='\<\.BLOCKQUOTE\s\+OFF\>' skip='$' contains=ALL
syn keyword momBreakQuote .BREAK_QUOTE'

" Footnotes
syn region momFootnote matchgroup=momKeyword start='\<\.FOOTNOTE\>' matchgroup=momKeyword end='\<\.FOOTNOTE\s\+OFF\>' skip='$' contains=ALL

" Comments
syn region momCommentLine start='\(\\!\)\|\(\\"\)\|\(\\#\)' end='$' contains=momTodo
syn region momCommentRegion matchgroup=momKeyword start='\<\.\(COMMENT\)\|\(SILENT\)\>' matchgroup=momKeyword end='\<\.\(COMMENT\s\+OFF\)\|\(SILENT\s\+OFF\)\>' skip='$'

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_mom_syn_inits")
	if version < 508
		let did_mom_syn_inits = 1
		command -nargs=+ HiLink hi link <args>
	else
		command -nargs=+ HiLink hi def link <args>
	endif

	" The default methods for highlighting. Can be overrriden later.
	HiLink momTodo Todo
	HiLink momDefine Define
	HiLink momPreprocessor PreProc
	HiLink momNumberReg Special
	HiLink momStringReg Special
	HiLink momCommentLine Comment
	HiLink momCommentRegion Comment
	HiLink momInteger Number
	HiLink momFloatEN Number
	HiLink momFloatDE Number
	HiLink momString String
	HiLink momHyphenation Tag
	HiLink momNoLineBreak Special
	HiLink momKeyword Keyword
	HiLink momSpecialParam Special
	HiLink momKeywordParam Keyword

	HiLink momBracketError Error
	HiLink momBrackets Delimiter

	hi momNormal term=none cterm=none gui=none
	hi momItalic term=italic cterm=italic gui=italic
	hi momBoldItalic term=bold,italic cterm=bold,italic gui=bold,italic
	HiLink momGreek momBoldItalic
	HiLink momSpecialChar momItalic
	HiLink momSpecialMove momBoldItalic
	
	HiLink momQuote momBoldItalic
	HiLink momBlockQuote momBoldItalic
	HiLink momBreakQuote momNormal
	
	HiLink momFootnote momItalic
	
	delcommand HiLink
endif

let b:current_syntax = "mom"

" vim:ts=8:sw=4:nocindent:smartindent:
