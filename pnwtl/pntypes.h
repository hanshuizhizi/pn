/**
 * @file pntypes.h
 * @brief Define structs etc. used throughout pn2.
 * @author Simon Steele
 * @note Copyright (c) 2002 Simon Steele <s.steele@pnotepad.org>
 *
 * Programmers Notepad 2 : The license file (license.[txt|html]) describes 
 * the conditions under which this source may be modified / distributed.
 */

#ifndef pntypes_h__included
#define pntypes_h__included

typedef struct tagFindOptions
{
	CString FindText;
	bool MatchWholeWord;
	bool MatchCase;
	bool UseRegExp;
	bool SearchAll;
	bool Direction;		// true is down.
	bool Loop;
	bool Found;
	bool UseSlashes;
} SFindOptions;

typedef struct tagReplaceOptions : tagFindOptions
{
	CString ReplaceText;
	bool	InSelection;
} SReplaceOptions;

#endif