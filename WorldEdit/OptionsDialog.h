//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Options dialog
//////////////////////////////////////////////////////////////////////////////////

#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include "wx_inc.h"
#include "EditorHeader.h"

class COptionsDialog : public wxDialog
{
public:
	COptionsDialog();

	void LoadOptions();
	void SaveOptions();

protected:


};

#endif // OPTIONSDIALOG_H